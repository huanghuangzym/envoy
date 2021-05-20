#pragma once

#include <memory>
#include <string>

#include "envoy/router/router.h"

#include "extensions/filters/network/dubbo_proxy/metadata.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace DubboProxy {
namespace Testhl {


/**
 * Route holds the RouteEntry for a request.
 */
class Route {
public:
  virtual ~Route() = default;

};

using RouteConstSharedPtr = std::shared_ptr<const Route>;
using RouteSharedPtr = std::shared_ptr<Route>;


} // namespace Testhl
} // namespace DubboProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
