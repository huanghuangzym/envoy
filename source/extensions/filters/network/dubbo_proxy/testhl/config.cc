#include "extensions/filters/network/dubbo_proxy/testhl/config.h"

#include "envoy/extensions/filters/network/dubbo_proxy/router/v3/router.pb.h"
#include "envoy/extensions/filters/network/dubbo_proxy/router/v3/router.pb.validate.h"
#include "envoy/registry/registry.h"

#include "extensions/filters/network/dubbo_proxy/testhl/router_impl.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace DubboProxy {
namespace Testhl {

DubboFilters::FilterFactoryCb RouterFilterConfig::createFilterFactoryFromProtoTyped(
    const envoy::extensions::filters::network::dubbo_proxy::router::v3::Router&, const std::string&,
    Server::Configuration::FactoryContext& context) {
  return [&context](DubboFilters::FilterChainFactoryCallbacks& callbacks) -> void {
    callbacks.addDecoderFilter(std::make_shared<Router>(context.clusterManager()));
  };
}

/**
 * Static registration for the router filter. @see RegisterFactory.
 */
REGISTER_FACTORY(RouterFilterConfig, DubboFilters::NamedDubboFilterConfigFactory);

} // namespace Testhl
} // namespace DubboProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
