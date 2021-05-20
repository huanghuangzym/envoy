#pragma once

#include "envoy/extensions/filters/network/dubbo_proxy/router/v3/router.pb.h"
#include "envoy/extensions/filters/network/dubbo_proxy/router/v3/router.pb.validate.h"

#include "extensions/filters/network/dubbo_proxy/filters/factory_base.h"
#include "extensions/filters/network/dubbo_proxy/filters/well_known_names.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace DubboProxy {
namespace Testhl {

class RouterFilterConfig
    : public DubboFilters::FactoryBase<
          envoy::extensions::filters::network::dubbo_proxy::router::v3::Router> {
public:
  RouterFilterConfig() : FactoryBase(DubboFilters::DubboFilterNames::get().TESTHL) {}

private:
  DubboFilters::FilterFactoryCb createFilterFactoryFromProtoTyped(
      const envoy::extensions::filters::network::dubbo_proxy::router::v3::Router& proto_config,
      const std::string& stat_prefix, Server::Configuration::FactoryContext& context) override;
};

} // namespace Testhl
} // namespace DubboProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
