#include "source/common/network/downstream_target_port.h"

#include "source/common/common/macros.h"

namespace Envoy {
namespace Network {

const std::string& DownstreamTargetPort::key() {
  CONSTRUCT_ON_FIRST_USE(std::string, "envoy.network.downstream_target_port");
}
} // namespace Network
} // namespace Envoy
