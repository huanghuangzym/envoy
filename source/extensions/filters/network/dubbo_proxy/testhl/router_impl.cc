#include "extensions/filters/network/dubbo_proxy/testhl/router_impl.h"

#include "envoy/upstream/cluster_manager.h"
#include "envoy/upstream/thread_local_cluster.h"

#include "extensions/filters/network/dubbo_proxy/app_exception.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace DubboProxy {
namespace Testhl {

void Router::onDestroy() {
  if (upstream_request_) {
    upstream_request_->resetStream();
  }
  cleanup();
}

void Router::setDecoderFilterCallbacks(DubboFilters::DecoderFilterCallbacks& callbacks) {
  callbacks_ = &callbacks;
}

FilterStatus Router::onMessageDecoded(MessageMetadataSharedPtr metadata, ContextSharedPtr ctx) {
  ASSERT(metadata->hasInvocationInfo());

    ENVOY_LOG(debug, "onMessageDecoded  dubbo-testhl get hasInvocationInfo {} {} {}  org data is {}",
            metadata->invocationInfo().serviceName(),metadata->invocationInfo().methodName(),
           // metadata->invocationInfo().serviceVersion(),metadata->invocationInfo().serviceGroup(),
              metadata->requestId(),ctx->messageOriginData().toString());

  ENVOY_STREAM_LOG(debug, "onMessageDecoded dubbo-testhl router: decoding request  {}", *callbacks_,ctx->messageSize());


  return FilterStatus::Continue;
}

void Router::onUpstreamData(Buffer::Instance& data, bool end_stream) {
  ASSERT(!upstream_request_->response_complete_);

  ENVOY_STREAM_LOG(trace, "onUpstreamData dubbo-testhl: reading response: {} bytes {}  data is {}",
          *callbacks_, data.length(),end_stream,data.toString());

}

void Router::onEvent(Network::ConnectionEvent event) {

    ENVOY_LOG(debug, "onEvent dubbo-testhl get event {}",event);

  if (!upstream_request_ || upstream_request_->response_complete_) {
    // Client closed connection after completing response.
    ENVOY_LOG(debug, "onEvent dubbo-testhl upstream request: the upstream request had completed");
    return;
  }

  if (upstream_request_->stream_reset_ && event == Network::ConnectionEvent::LocalClose) {
    ENVOY_LOG(debug, "onEvent dubbo-testhl upstream request: the stream reset");
    return;
  }

}

const Network::Connection* Router::downstreamConnection() const {
  return callbacks_ != nullptr ? callbacks_->connection() : nullptr;
}

void Router::cleanup() {
  if (upstream_request_) {
    upstream_request_.reset();
  }
}

Router::UpstreamRequest::UpstreamRequest(Router& parent, Tcp::ConnectionPool::Instance& pool,
                                         MessageMetadataSharedPtr& metadata,
                                         SerializationType serialization_type,
                                         ProtocolType protocol_type)
    : parent_(parent), conn_pool_(pool), metadata_(metadata),
      protocol_(
          NamedProtocolConfigFactory::getFactory(protocol_type).createProtocol(serialization_type)),
      request_complete_(false), response_started_(false), response_complete_(false),
      stream_reset_(false) {}

Router::UpstreamRequest::~UpstreamRequest() = default;

FilterStatus Router::UpstreamRequest::start() {


  return FilterStatus::Continue;
}

void Router::UpstreamRequest::resetStream() {
  stream_reset_ = true;
    ENVOY_LOG(debug, "resetStream dubbo-testhl begit reset stream");

  if (conn_pool_handle_) {
    ASSERT(!conn_data_);
    conn_pool_handle_->cancel(Tcp::ConnectionPool::CancelPolicy::Default);
    conn_pool_handle_ = nullptr;
    ENVOY_LOG(debug, "dubbo-testhl upstream request: reset connection pool handler");
  }

  if (conn_data_) {
    ASSERT(!conn_pool_handle_);
    conn_data_->connection().close(Network::ConnectionCloseType::NoFlush);
    conn_data_.reset();
    ENVOY_LOG(debug, "dubbo-testhl upstream request: reset connection data");
  }
}

void Router::UpstreamRequest::encodeData(Buffer::Instance& data) {
  ASSERT(conn_data_);
  ASSERT(!conn_pool_handle_);

  ENVOY_STREAM_LOG(trace, "encodeData dubbo-testhl {} bytes data {}",
          *parent_.callbacks_, data.length(),data.toString());

}

void Router::UpstreamRequest::onPoolFailure(ConnectionPool::PoolFailureReason reason,
                                            Upstream::HostDescriptionConstSharedPtr host) {
    ENVOY_LOG(debug, "onPoolFailure dubbo-testhl upstream request:onPoolFailure {}  and host {}",reason,host);
}

void Router::UpstreamRequest::onPoolReady(Tcp::ConnectionPool::ConnectionDataPtr&& conn_data,
                                          Upstream::HostDescriptionConstSharedPtr host) {
    conn_data_ = std::move(conn_data);
  ENVOY_LOG(debug, "onPoolReady dubbo-testhl upstream request: tcp connection has ready  {} ",host);

}

void Router::UpstreamRequest::onRequestStart(bool continue_decoding) {
  ENVOY_LOG(debug, "dubbo-testhl upstream request: start sending data to the server {}  {}",
            upstream_host_->address()->asString(),continue_decoding);

}

void Router::UpstreamRequest::onRequestComplete() { request_complete_ = true; }

void Router::UpstreamRequest::onResponseComplete() {
}

void Router::UpstreamRequest::onUpstreamHostSelected(Upstream::HostDescriptionConstSharedPtr host) {
  ENVOY_LOG(debug, "onUpstreamHostSelected dubbo-testhl upstream request: selected upstream {}", host->address()->asString());
  upstream_host_ = host;
}

void Router::UpstreamRequest::onResetStream(ConnectionPool::PoolFailureReason reason) {
  if (metadata_->messageType() == MessageType::Oneway) {
    // For oneway requests, we should not attempt a response. Reset the downstream to signal
    // an error.
    ENVOY_LOG(debug, "dubbo-testhl upstream request: the request is oneway, reset downstream stream");
    parent_.callbacks_->resetStream();
    return;
  }

  // When the filter's callback does not end, the sendLocalReply function call
  // triggers the release of the current stream at the end of the filter's callback.
  switch (reason) {
  case ConnectionPool::PoolFailureReason::Overflow:
    parent_.callbacks_->sendLocalReply(
        AppException(ResponseStatus::ServerError,
                     fmt::format("dubbo-testhl upstream request: too many connections")),
        false);
    break;
  case ConnectionPool::PoolFailureReason::LocalConnectionFailure:
    // Should only happen if we closed the connection, due to an error condition, in which case
    // we've already handled any possible downstream response.
    parent_.callbacks_->sendLocalReply(
        AppException(ResponseStatus::ServerError,
                     fmt::format("dubbo-testhl upstream request: local connection failure '{}'",
                                 upstream_host_->address()->asString())),
        false);
    break;
  case ConnectionPool::PoolFailureReason::RemoteConnectionFailure:
    parent_.callbacks_->sendLocalReply(
        AppException(ResponseStatus::ServerError,
                     fmt::format("dubbo-testhl upstream request: remote connection failure '{}'",
                                 upstream_host_->address()->asString())),
        false);
    break;
  case ConnectionPool::PoolFailureReason::Timeout:
    parent_.callbacks_->sendLocalReply(
        AppException(ResponseStatus::ServerError,
                     fmt::format("dubbo-testhl upstream request: connection failure '{}' due to timeout",
                                 upstream_host_->address()->asString())),
        false);
    break;
  default:
    NOT_REACHED_GCOVR_EXCL_LINE;
  }

  if (parent_.filter_complete_ && !response_complete_) {
    // When the filter's callback has ended and the reply message has not been processed,
    // call resetStream to release the current stream.
    // the resetStream eventually triggers the onDestroy function call.
    parent_.callbacks_->resetStream();
  }
}

} // namespace Testhl
} // namespace DubboProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
