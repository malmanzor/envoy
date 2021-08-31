#pragma once

#include "envoy/stream_info/filter_state.h"

namespace Envoy {
namespace Network {

/**
 * Target port that a downstream is connecting to.
 */
class DownstreamTargetPort : public StreamInfo::FilterState::Object {
public:
  DownstreamTargetPort(absl::string_view target_port) : target_port_(target_port) {}
  const std::string& value() const { return target_port_; }
  static const std::string& key();

private:
  const std::string target_port_;
};

} // namespace Network
} // namespace Envoy
