#include "source/common/network/transport_socket_options_impl.h"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "source/common/common/scalar_to_byte_vector.h"
#include "source/common/common/utility.h"
#include "source/common/network/application_protocol.h"
#include "source/common/network/downstream_target_port.h"
#include "source/common/network/proxy_protocol_filter_state.h"
#include "source/common/network/upstream_server_name.h"
#include "source/common/network/upstream_subject_alt_names.h"
#include "source/extensions/filters/common/expr/cel_state.h"

namespace Envoy {
namespace Network {
namespace {
void commonHashKey(const TransportSocketOptions& options, std::vector<std::uint8_t>& key,
                   const Network::TransportSocketFactory& factory) {
  const auto& server_name_overide = options.serverNameOverride();
  if (server_name_overide.has_value()) {
    pushScalarToByteVector(StringUtil::CaseInsensitiveHash()(server_name_overide.value()), key);
  }

  const auto& verify_san_list = options.verifySubjectAltNameListOverride();
  for (const auto& san : verify_san_list) {
    pushScalarToByteVector(StringUtil::CaseInsensitiveHash()(san), key);
  }

  const auto& alpn_list = options.applicationProtocolListOverride();
  for (const auto& protocol : alpn_list) {
    pushScalarToByteVector(StringUtil::CaseInsensitiveHash()(protocol), key);
  }

  const auto& alpn_fallback = options.applicationProtocolFallback();
  for (const auto& protocol : alpn_fallback) {
    pushScalarToByteVector(StringUtil::CaseInsensitiveHash()(protocol), key);
  }

  // Proxy protocol options should only be included in the hash if the upstream
  // socket intends to use them.
  const auto& proxy_protocol_options = options.proxyProtocolOptions();
  if (proxy_protocol_options.has_value() && factory.usesProxyProtocolOptions()) {
    pushScalarToByteVector(
        StringUtil::CaseInsensitiveHash()(proxy_protocol_options.value().asStringForHash()), key);
  }
}
} // namespace

void AlpnDecoratingTransportSocketOptions::hashKey(
    std::vector<uint8_t>& key, const Network::TransportSocketFactory& factory) const {
  commonHashKey(*this, key, factory);
}

void TransportSocketOptionsImpl::hashKey(std::vector<uint8_t>& key,
                                         const Network::TransportSocketFactory& factory) const {
  commonHashKey(*this, key, factory);
}

TransportSocketOptionsConstSharedPtr
TransportSocketOptionsUtility::fromFilterState(const StreamInfo::FilterState& filter_state) {
  absl::string_view server_name;
  absl::string_view downstream_port;
  std::vector<std::string> application_protocols;
  std::vector<std::string> subject_alt_names;
  std::vector<std::string> alpn_fallback;
  absl::optional<Network::ProxyProtocolData> proxy_protocol_options;

  std::cout << "creating a new transport socket options using filter state\n";

  if (filter_state.hasData<Extensions::Filters::Common::Expr::CelState>("wasm.envoy.network.upstream_server_name")) {
    std::cout << "found the upstream server name from wasm with prefix\n";
  }

 

  bool needs_transport_socket_options = false;


 if (filter_state.hasData<Extensions::Filters::Common::Expr::CelState>("wasm.env")) {
    std::cout << "found the upstream server name from wasm with env prefix\n";
    const auto& upstream_server_name =
        filter_state.getDataReadOnly<Extensions::Filters::Common::Expr::CelState>("wasm.env");
    server_name = upstream_server_name.value();
    needs_transport_socket_options = true;
  
  }

  if (filter_state.hasData<UpstreamServerName>(UpstreamServerName::key())) {
    const auto& upstream_server_name =
        filter_state.getDataReadOnly<UpstreamServerName>(UpstreamServerName::key());
    server_name = upstream_server_name.value();
    needs_transport_socket_options = true;
  }

  // server_name = "httpbin.org:443";
  // needs_transport_socket_options = true;

  if (filter_state.hasData<Network::ApplicationProtocols>(Network::ApplicationProtocols::key())) {
    const auto& alpn = filter_state.getDataReadOnly<Network::ApplicationProtocols>(
        Network::ApplicationProtocols::key());
    application_protocols = alpn.value();
    needs_transport_socket_options = true;
  }

  std::cout << "checking to see if the downstream port has been set in filter config\n";
  // TODO check the filter state for a sni port addition from the filter state and add it if needed
  if (filter_state.hasData<DownstreamTargetPort>(DownstreamTargetPort::key())) {
    const auto& port =
        filter_state.getDataReadOnly<DownstreamTargetPort>(DownstreamTargetPort::key());
    downstream_port = port.value();
    std::cout << "found downstream port ";
    std::cout << downstream_port;
    std::cout << ";\n";
    needs_transport_socket_options = true;
  }

  if (filter_state.hasData<UpstreamSubjectAltNames>(UpstreamSubjectAltNames::key())) {
    const auto& upstream_subject_alt_names =
        filter_state.getDataReadOnly<UpstreamSubjectAltNames>(UpstreamSubjectAltNames::key());
    subject_alt_names = upstream_subject_alt_names.value();
    needs_transport_socket_options = true;
  }

  if (filter_state.hasData<ProxyProtocolFilterState>(ProxyProtocolFilterState::key())) {
    const auto& proxy_protocol_filter_state =
        filter_state.getDataReadOnly<ProxyProtocolFilterState>(ProxyProtocolFilterState::key());
    proxy_protocol_options.emplace(proxy_protocol_filter_state.value());
    needs_transport_socket_options = true;
  }

  if (needs_transport_socket_options) {
    return std::make_shared<Network::TransportSocketOptionsImpl>(
        server_name, std::move(subject_alt_names), std::move(application_protocols),
        std::move(alpn_fallback), proxy_protocol_options, downstream_port);
  } else {
    return nullptr;
  }
}

} // namespace Network
} // namespace Envoy
