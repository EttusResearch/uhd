//
// Auto-generated C++ client implementation
// Generated from protobuf definition
//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// clang-format off

#include "mpm_client.hpp"
#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <stdexcept>
#include <array>
#include <atomic>
#include <chrono>
#include <optional>
#include <mutex>
#include <thread>
#include <grpcpp/grpcpp.h>
#include "mpm_server.pb.h"
#include "mpm_server.grpc.pb.h"


// GRPC_ARG_USE_LOCAL_SUBCHANNEL_POOL is not declared by older gRPC releases
// (e.g. the version shipped with Ubuntu 20.04), but the underlying channel-arg
// key string is stable across versions.
#ifndef GRPC_ARG_USE_LOCAL_SUBCHANNEL_POOL
#define GRPC_ARG_USE_LOCAL_SUBCHANNEL_POOL "grpc.use_local_subchannel_pool"
#endif

using uhd::rpc_exception;

<%
def build_param_log_strings(parameters):
    """Helper function to build parameter logging strings"""
    param_strs = []
    for param in parameters:
        parts = param.strip().split()
        param_name = parts[-1]
        param_type = ' '.join(parts[:-1])
        # Show only a 4-char prefix for correlation in large setups; never log the full token
        if param_name == 'token':
            param_strs.append(f'"token=" << {param_name}.substr(0, 4) << "****"')
            continue
        # Handle complex types that can't be streamed
        if 'std::vector<std::map' in param_type:
            param_strs.append(f'"{param_name}=[vector<map> size=" << {param_name}.size() << "]"')
        elif 'std::vector<std::vector' in param_type:
            param_strs.append(f'"{param_name}=[vector<vector> size=" << {param_name}.size() << "]"')
        elif 'std::vector<std::string>' in param_type:
            param_strs.append(f'"{param_name}=[vector<string> size=" << {param_name}.size() << "]"')
        elif 'std::vector<int' in param_type:
            param_strs.append(f'"{param_name}=[vector<int> size=" << {param_name}.size() << "]"')
        elif 'std::vector<double>' in param_type or 'std::vector<float>' in param_type:
            param_strs.append(f'"{param_name}=[vector<double> size=" << {param_name}.size() << "]"')
        elif 'std::map<std::string, std::vector' in param_type:
            param_strs.append(f'"{param_name}=[map<string,bytes> size=" << {param_name}.size() << "]"')
        elif 'std::map<std::string, std::string>' in param_type:
            param_strs.append(f'"{param_name}=[map size=" << {param_name}.size() << "]"')
        else:
            param_strs.append(f'"{param_name}=" << {param_name}')
    return param_strs
%>

#include <uhdlib/usrp/common/mpmd_timeouts.hpp>

constexpr int MAX_GRPC_MESSAGE_SIZE = 128 * 1024 * 1024;  // 128 MB in bytes

class rpc_client_impl : public uhd::rpc_client, public std::enable_shared_from_this<rpc_client_impl>
{
public:
    class dboard_iface_impl;

private:
    static constexpr size_t MAX_DBOARDS = 2;  // 0, 1 (two daughterboards)

    std::unique_ptr<${package_name}::${service_name}::Stub> stub_;
    std::string _token;
    std::chrono::milliseconds _timeout;
    std::optional<std::chrono::milliseconds> _scoped_timeout;
    mutable std::recursive_mutex _rpc_call_mutex;
    std::array<std::unique_ptr<dboard_iface_impl>, MAX_DBOARDS> dboard_instances_;

    // Diagnostics for tracking gRPC channel/transport lifecycle. Each client
    // gets a unique id so its create/RPC/destroy events can be correlated in
    // the logs (e.g. to see whether tearing down a stale session's channel
    // drops the active session's connection).
    static std::atomic<uint64_t> _client_id_counter;
    uint64_t _client_id;
    std::string _server_address;
    std::shared_ptr<grpc::Channel> _channel;

    // Human-readable name for a gRPC connectivity state.
    static const char* connectivity_state_str(grpc_connectivity_state s)
    {
        switch (s) {
            case GRPC_CHANNEL_IDLE: return "IDLE";
            case GRPC_CHANNEL_CONNECTING: return "CONNECTING";
            case GRPC_CHANNEL_READY: return "READY";
            case GRPC_CHANNEL_TRANSIENT_FAILURE: return "TRANSIENT_FAILURE";
            case GRPC_CHANNEL_SHUTDOWN: return "SHUTDOWN";
            default: return "UNKNOWN";
        }
    }

    // RAII guard that applies a scoped timeout while serializing concurrent RPCs.
    class timeout_scope_impl : public uhd::rpc_client::timeout_scope
    {
    public:
        timeout_scope_impl(rpc_client_impl& parent, std::chrono::milliseconds timeout)
            : _parent(parent), _lock(parent._rpc_call_mutex)
        {
            _parent._scoped_timeout = timeout;
        }

        ~timeout_scope_impl() override
        {
            _parent._scoped_timeout.reset();
        }

    private:
        rpc_client_impl& _parent;
        std::unique_lock<std::recursive_mutex> _lock;
    };

    // Helper method to convert StringMap to std::map
    std::map<std::string, std::string> stringmap_to_stdmap(const ${package_name}::StringMap& sm) {
        std::map<std::string, std::string> result;
        for (const auto& pair : sm.data()) {
            result[pair.first] = pair.second;
        }
        return result;
    }

    // Helper method to convert BytesMap to eeprom_map_t
    std::map<std::string, std::vector<uint8_t>> bytesmap_to_eeprom_map(const ${package_name}::BytesMap& bm) {
        std::map<std::string, std::vector<uint8_t>> result;
        for (const auto& pair : bm.data()) {
            result[pair.first] = std::vector<uint8_t>(pair.second.begin(), pair.second.end());
        }
        return result;
    }

    // Helper method to convert SensorValueMap to std::map
    std::map<std::string, std::string> sensorvaluemap_to_stdmap(const ${package_name}::SensorValueMap& svm) {
        std::map<std::string, std::string> result;
        for (const auto& pair : svm.data()) {
            result[pair.first] = pair.second;
        }
        return result;
    }

    // Convert std::map -> StringMap
    void stdmap_to_stringmap(const std::map<std::string, std::string>& stdmap, ${package_name}::StringMap* sm) {
        auto* data = sm->mutable_data();
        for (const auto& pair : stdmap) {
            (*data)[pair.first] = pair.second;
        }
    }

    // Convert eeprom_map_t -> BytesMap
    void eeprom_map_to_bytesmap(const std::map<std::string, std::vector<uint8_t>>& eeprom_map, ${package_name}::BytesMap* bm) {
        auto* data = bm->mutable_data();
        for (const auto& pair : eeprom_map) {
            (*data)[pair.first] = std::string(pair.second.begin(), pair.second.end());
        }
    }

    // Convert vector of maps to repeated StringMap
    void vector_to_repeated_stringmap(const std::vector<std::map<std::string, std::string>>& vec,
                                     google::protobuf::RepeatedPtrField<${package_name}::StringMap>* repeated) {
        for (const auto& map : vec) {
            ${package_name}::StringMap* sm = repeated->Add();
            stdmap_to_stringmap(map, sm);
        }
    }

    // Convert repeated StringMap to vector of maps
    std::vector<std::map<std::string, std::string>> repeated_stringmap_to_vector(
        const google::protobuf::RepeatedPtrField<${package_name}::StringMap>& repeated) {
        std::vector<std::map<std::string, std::string>> result;
        for (const auto& sm : repeated) {
            result.push_back(stringmap_to_stdmap(sm));
        }
        return result;
    }

    // Convert MethodInfo to std::map (avoid leaking protobuf types)
    std::map<std::string, std::string> methodinfo_to_stdmap(const ${package_name}::MethodInfo& mi) {
        std::map<std::string, std::string> result;
        result["method_name"] = mi.method_name();
        result["docstring"] = mi.docstring();
        result["requires_claim"] = mi.requires_claim() ? "true" : "false";
        return result;
    }

public:
    rpc_client_impl(const std::string& server_name, uint16_t port, uint64_t timeout_ms)
        : _timeout(std::chrono::milliseconds(timeout_ms))
        , _client_id(_client_id_counter++)
    {
        std::string server_address = server_name + ":" + std::to_string(port);
        _server_address = server_address;

        // Set channel arguments with MAX_GRPC_MESSAGE_SIZE limit (in bytes).
        grpc::ChannelArguments args;
        args.SetInt(GRPC_ARG_MAX_RECEIVE_MESSAGE_LENGTH, MAX_GRPC_MESSAGE_SIZE);
        args.SetInt(GRPC_ARG_MAX_SEND_MESSAGE_LENGTH, MAX_GRPC_MESSAGE_SIZE);

        // Give every channel its own subchannel (and thus its own HTTP/2
        // transport) instead of sharing gRPC's global subchannel pool. All
        // clients here target the same MPM address with identical channel
        // args, so with the default global pool they would collapse onto a
        // single shared TCP/HTTP2 connection. When a stale session's channel
        // is destroyed (which, with the phase-coherence tests, happens tens of
        // seconds after that session was unclaimed), tearing down the shared
        // transport surfaces on the *active* session as
        // "<RPC> RPC failed: Socket closed" even though the MPM is healthy.
        // A local subchannel pool isolates each channel so destroying an old
        // session cannot drop the active session's connection.
        args.SetInt(GRPC_ARG_USE_LOCAL_SUBCHANNEL_POOL, 1);

        auto channel = grpc::CreateCustomChannel(server_address, grpc::InsecureChannelCredentials(), args);
        _channel = channel;
        stub_ = ${package_name}::${service_name}::NewStub(channel);

        // Eagerly establish the TCP connection so subsequent RPC calls don't
        // also pay the connection-setup cost within their per-call deadline.
        // WaitForConnected() returns false on timeout; we let the first RPC
        // call report the failure naturally in that case.
        channel->WaitForConnected(std::chrono::system_clock::now() + _timeout);

        UHD_LOGGER_TRACE("MPM_CLIENT")
            << "rpc_client #" << _client_id << " CREATED -> " << _server_address
            << " state=" << connectivity_state_str(_channel->GetState(false));

        // Pre-allocate all daughterboard instances
        for (size_t i = 0; i < MAX_DBOARDS; ++i) {
            dboard_instances_[i] = std::make_unique<dboard_iface_impl>(this, i);
        }
    }

    ~rpc_client_impl() override
    {
        // This destruction is the event of interest: with the phase-coherence
        // tests, a stale session's client is destroyed tens of seconds after
        // it was unclaimed. Log it (with the last-known channel state) so it
        // can be correlated with any "Socket closed" seen on the active session.
        UHD_LOGGER_TRACE("MPM_CLIENT")
            << "rpc_client #" << _client_id << " DESTROYED -> " << _server_address
            << " state="
            << (_channel ? connectivity_state_str(_channel->GetState(false)) : "n/a");
    }

    void set_token(const std::string& token) override {
        _token = token;
    }

    // Set the default timeout used for RPCs without a scoped override.
    void set_timeout(uint64_t timeout_ms) override {
        std::lock_guard<std::recursive_mutex> rpc_call_lock(_rpc_call_mutex);
        _timeout = std::chrono::milliseconds(timeout_ms);
    }

    // Return an RAII guard that applies a timeout override for its lifetime.
    uhd::rpc_client::timeout_scope_uptr set_scope_timeout(uint64_t timeout_ms) override {
        return std::make_unique<timeout_scope_impl>(
            *this, std::chrono::milliseconds(timeout_ms));
    }

    uhd::rpc_client::sptr get_raw_rpc_client() override {
        return shared_from_this();
    }

    uint64_t get_client_id() const override {
        return _client_id;
    }

private:
    // Helper method to get the appropriate deadline for a call. Scoped
    // timeout has precedence while the returned timeout_scope is alive.
    std::chrono::system_clock::time_point get_deadline_for_call(
        std::chrono::milliseconds default_timeout) {
        if (_scoped_timeout.has_value()) {
            return std::chrono::system_clock::now() + _scoped_timeout.value();
        }
        return std::chrono::system_clock::now() + default_timeout;
    }

public:
    // Main domain method implementations
% for method in main_methods:
    ${method['return_type']} ${method['name']}(${', '.join(method['parameters'])}) override {
        <% param_strs = build_param_log_strings(method['parameters']) %>
% if param_strs:
        UHD_LOG_TRACE("RPC", ">>> ${method['camel_name']}(" << ${'<< ", " << '.join(param_strs)} << ")");
% else:
        UHD_LOG_TRACE("RPC", ">>> ${method['camel_name']}()");
% endif
        ${package_name}::${method['request_type']} request;
        ${package_name}::${method['response_type']} response;
        grpc::ClientContext context;
        std::lock_guard<std::recursive_mutex> rpc_call_lock(_rpc_call_mutex);

        // Set timeout
        % if method.get('timeout'):
            % if method['timeout'].isdigit():
        context.set_deadline(get_deadline_for_call(std::chrono::milliseconds(${method['timeout']})));
            % else:
        context.set_deadline(get_deadline_for_call(std::chrono::milliseconds(${method['timeout']})));
            % endif
        % else:
        context.set_deadline(get_deadline_for_call(_timeout));
        % endif

        % if method['requires_token']:
        request.set_token(_token); // Set token
        % endif
        // Set parameters
        % for param in method['parameters']:
            <%
                param_parts = param.strip().split()
                param_name = param_parts[-1]
                param_type = ' '.join(param_parts[:-1])
            %>
            % if 'std::vector<std::map<std::string, std::string>>' in param_type:
        vector_to_repeated_stringmap(${param_name}, request.mutable_${param_name}());
            % elif 'std::vector<std::vector<uint8_t>>' in param_type or 'std::vector<std::vector<unsigned char>>' in param_type:
        // Convert vector of byte vectors to repeated bytes field
        for (const auto& data : ${param_name}) {
            request.add_${param_name}(data.data(), data.size());
        }
            % elif 'std::vector<std::string>' in param_type:
        // Convert vector of strings to repeated string field
        for (const auto& item : ${param_name}) {
            request.add_${param_name}(item);
        }
            % elif 'std::vector<int' in param_type:
        // Convert vector of integers to repeated int field
        for (const auto& item : ${param_name}) {
            request.add_${param_name}(item);
        }
            % elif 'std::vector<double>' in param_type or 'std::vector<float>' in param_type:
        // Convert vector of doubles/floats to repeated field
        for (const auto& item : ${param_name}) {
            request.add_${param_name}(item);
        }
            % elif 'std::map<std::string, std::vector<uint8_t>>' in param_type:
        eeprom_map_to_bytesmap(${param_name}, request.mutable_${param_name}());
            % elif 'std::map<std::string, std::string>' in param_type:
        stdmap_to_stringmap(${param_name}, request.mutable_${param_name}());
            % else:
        request.set_${param_name}(${param_name});
            % endif
        % endfor

        grpc::Status status = stub_->${method['camel_name']}(&context, request, &response);

        if (status.ok()) {
            // Return response
        % if method['return_type'] == 'void':
            UHD_LOG_TRACE("RPC", "<<< ${method['camel_name']} [void]");
            return;
        % elif method['response_field'] is None:
            UHD_LOG_TRACE("RPC", "<<< ${method['camel_name']} [none]");
            return;
        % elif method['return_type'] == 'double' or method['return_type'] == 'float':
            auto result = response.${method['response_field']['name']}();
            UHD_LOG_TRACE("RPC", "<<< ${method['camel_name']} = " << result);
            return result;
        % elif method['return_type'] == 'uint16_t':
            // Cast uint32 to uint16_t for get_proto_ver
            auto result = static_cast<uint16_t>(response.${method['response_field']['name']}());
            UHD_LOG_TRACE("RPC", "<<< ${method['camel_name']} = " << result);
            return result;
        % elif method['return_type'] == 'std::string':
            auto result = response.${method['response_field']['name']}();
            % if method['response_field']['name'] == 'token':
            UHD_LOG_TRACE("RPC", "<<< ${method['camel_name']} with token = " << result.substr(0, 4) << "****");
            % else:
            UHD_LOG_TRACE("RPC", "<<< ${method['camel_name']} = \"" << result << "\"");
            % endif
            return result;
        % elif method['return_type'] == 'std::map<std::string, std::vector<uint8_t>>':
            auto result = bytesmap_to_eeprom_map(response.${method['response_field']['name']}());
            UHD_LOG_TRACE("RPC", "<<< ${method['camel_name']} = map<string,bytes>[" << result.size() << " entries]");
            return result;
        % elif method['return_type'] == 'std::map<std::string, std::string>':
            % if 'map<' in method['response_field']['type']:
            std::map<std::string, std::string> result;
            for (const auto& pair : response.${method['response_field']['name']}()) {
                result[pair.first] = pair.second;
            }
            UHD_LOG_TRACE("RPC", "<<< ${method['camel_name']} = map[" << result.size() << " entries]");
            return result;
            % elif method['response_field']['type'] == 'SensorValueMap':
            auto result = sensorvaluemap_to_stdmap(response.${method['response_field']['name']}());
            UHD_LOG_TRACE("RPC", "<<< ${method['camel_name']} = sensor_map[" << result.size() << " entries]");
            return result;
            % else:
            auto result = stringmap_to_stdmap(response.${method['response_field']['name']}());
            UHD_LOG_TRACE("RPC", "<<< ${method['camel_name']} = map[" << result.size() << " entries]");
            return result;
            % endif
        % elif method['return_type'] == 'std::vector<std::map<std::string, std::string>>':
            % if 'MethodInfo' in method['response_field']['type']:
            // Convert protobuf MethodInfo to standard map to avoid leaking protobuf types
            std::vector<std::map<std::string, std::string>> result;
            for (const auto& item : response.${method['response_field']['name']}()) {
                result.push_back(methodinfo_to_stdmap(item));
            }
            UHD_LOG_TRACE("RPC", "<<< ${method['camel_name']} = vector<map>[" << result.size() << " items]");
            return result;
            % else:
            auto result = repeated_stringmap_to_vector(response.${method['response_field']['name']}());
            UHD_LOG_TRACE("RPC", "<<< ${method['camel_name']} = vector<map>[" << result.size() << " items]");
            return result;
            % endif
        % elif method['return_type'].startswith('std::pair<'):
            // Handle pair return type
            <%
                f0 = method['pair_field_names'][0]
                f1 = method['pair_field_names'][1]
                inner = method['return_type'][len('std::pair<'):-1]
                depth, split = 0, 0
                for _i, _c in enumerate(inner):
                    if _c == '<': depth += 1
                    elif _c == '>': depth -= 1
                    elif _c == ',' and depth == 0: split = _i; break
                f0_type = inner[:split].strip()
                f1_type = inner[split+1:].strip()
            %>
            ${f0_type} ${f0} = response.${f0}();
            % if f1_type.startswith('std::vector<'):
            ${f1_type} ${f1}(response.${f1}().begin(), response.${f1}().end());
            % else:
            ${f1_type} ${f1} = response.${f1}();
            % endif
            auto result = std::make_pair(${f0}, ${f1});
            % if f1_type.startswith('std::vector<'):
            UHD_LOG_TRACE("RPC", "<<< ${method['camel_name']} = pair(${f0}=" << result.first << ", ${f1}=[" << result.second.size() << " items])");
            % else:
            UHD_LOG_TRACE("RPC", "<<< ${method['camel_name']} = pair(${f0}=" << result.first << ", ${f1}=" << result.second << ")");
            % endif
            return result;
        % elif method['return_type'].startswith('std::vector<'):
            % if 'MethodInfo' in method['response_field']['type']:
            // Convert protobuf MethodInfo to standard map to avoid leaking protobuf types
            std::vector<std::map<std::string, std::string>> result;
            for (const auto& item : response.${method['response_field']['name']}()) {
                result.push_back(methodinfo_to_stdmap(item));
            }
            UHD_LOG_TRACE("RPC", "<<< ${method['camel_name']} = vector<map>[" << result.size() << " items]");
            return result;
            % else:
            // Convert protobuf repeated field to std::vector
            ${method['return_type']} result;
            for (const auto& item : response.${method['response_field']['name']}()) {
                result.push_back(item);
            }
            UHD_LOG_TRACE("RPC", "<<< ${method['camel_name']} = vector[" << result.size() << " items]");
            return result;
            % endif
        % else:
            auto result = response.${method['response_field']['name']}();
            UHD_LOG_TRACE("RPC", "<<< ${method['camel_name']} = " << result);
            return result;
        % endif
        } else if (status.error_code() == grpc::StatusCode::PERMISSION_DENIED) {
            UHD_LOG_TRACE("RPC", "!!! ${method['camel_name']} FAILED [" << status.error_code() << "]: Authentication error");
            throw rpc_exception("Authentication failed: " + status.error_message());
        } else {
            UHD_LOG_TRACE("RPC", "!!! ${method['camel_name']} FAILED [" << status.error_code() << "]: " << status.error_message());
            UHD_LOGGER_TRACE("MPM_CLIENT")
                << "rpc_client #" << _client_id << " ${method['camel_name']} FAILED ["
                << status.error_code() << "]: " << status.error_message()
                << " | channel state="
                << (_channel ? connectivity_state_str(_channel->GetState(false)) : "n/a")
                << " -> " << _server_address;
            throw rpc_exception("${method['camel_name']} RPC failed: " + status.error_message());
        }
    }

% endfor

    // Daughterboard implementation
    class dboard_iface_impl : public uhd::rpc_client::dboard_iface
    {
    private:
        rpc_client_impl* parent_;
        size_t db_idx_;

    public:
        dboard_iface_impl(rpc_client_impl* parent, size_t db_idx)
            : parent_(parent), db_idx_(db_idx) {}

% for method in dboard_methods:
        ${method['return_type']} ${method['name']}(${', '.join(method['parameters'])}) override {
            <% param_strs = build_param_log_strings(method['parameters']) %>
% if param_strs:
            UHD_LOG_TRACE("RPC", ">>> ${method['camel_name']}(db_idx=" << db_idx_ << ", " << ${'<< ", " << '.join(param_strs)} << ")");
% else:
            UHD_LOG_TRACE("RPC", ">>> ${method['camel_name']}(db_idx=" << db_idx_ << ")");
% endif
            ${package_name}::${method['request_type']} request;
            ${package_name}::${method['response_type']} response;
            grpc::ClientContext context;
            std::lock_guard<std::recursive_mutex> rpc_call_lock(parent_->_rpc_call_mutex);

            // Set timeout
            % if method.get('timeout'):
                % if method['timeout'].isdigit():
            context.set_deadline(parent_->get_deadline_for_call(std::chrono::milliseconds(${method['timeout']})));
                % else:
            context.set_deadline(parent_->get_deadline_for_call(std::chrono::milliseconds(${method['timeout']})));
                % endif
            % else:
            context.set_deadline(parent_->get_deadline_for_call(parent_->_timeout));
            % endif

            % if method['requires_token']:
            request.set_token(parent_->_token); // Set token
            % endif
            request.set_db_idx(static_cast<uint32_t>(db_idx_)); // Set db_idx

            // Set other parameters
            % for param in method['parameters']:
                <%
                    param_parts = param.strip().split()
                    param_name = param_parts[-1]
                    param_type = ' '.join(param_parts[:-1])
                %>
                % if 'std::vector<std::map<std::string, std::string>>' in param_type:
            parent_->vector_to_repeated_stringmap(${param_name}, request.mutable_${param_name}());
                % elif 'std::vector<int' in param_type:
            // Convert vector of integers to repeated int field
            for (const auto& item : ${param_name}) {
                request.add_${param_name}(item);
            }
                % elif 'std::vector<double>' in param_type or 'std::vector<float>' in param_type:
            // Convert vector of doubles/floats to repeated field
            for (const auto& item : ${param_name}) {
                request.add_${param_name}(item);
            }
                % elif 'std::map<std::string, std::vector<uint8_t>>' in param_type:
            parent_->eeprom_map_to_bytesmap(${param_name}, request.mutable_${param_name}());
                % elif 'std::map<std::string, std::string>' in param_type:
            parent_->stdmap_to_stringmap(${param_name}, request.mutable_${param_name}());
                % else:
            request.set_${param_name}(${param_name});
                % endif
            % endfor

            grpc::Status status = parent_->stub_->${method['camel_name']}(&context, request, &response);

            if (status.ok()) {
                // Return response
            % if method['return_type'] == 'void':
                UHD_LOG_TRACE("RPC", "<<< ${method['camel_name']}[db=" << db_idx_ << "] [void]");
                return;
            % elif method['response_field'] is None:
                UHD_LOG_TRACE("RPC", "<<< ${method['camel_name']}[db=" << db_idx_ << "] [none]");
                return;
            % elif method['return_type'] == 'double' or method['return_type'] == 'float':
                auto result = response.${method['response_field']['name']}();
                UHD_LOG_TRACE("RPC", "<<< ${method['camel_name']}[db=" << db_idx_ << "] = " << result);
                return result;
            % elif method['return_type'] == 'std::string':
                auto result = response.${method['response_field']['name']}();
                % if method['response_field']['name'] == 'token':
                UHD_LOG_TRACE("RPC", "<<< ${method['camel_name']}[db=" << db_idx_ << "] with token = " << result.substr(0, 4) << "****");
                % else:
                UHD_LOG_TRACE("RPC", "<<< ${method['camel_name']}[db=" << db_idx_ << "] = \"" << result << "\"");
                % endif
                return result;
            % elif method['return_type'] == 'std::map<std::string, std::vector<uint8_t>>':
                auto result = parent_->bytesmap_to_eeprom_map(response.${method['response_field']['name']}());
                UHD_LOG_TRACE("RPC", "<<< ${method['camel_name']}[db=" << db_idx_ << "] = map<string,bytes>[" << result.size() << " entries]");
                return result;
            % elif method['return_type'] == 'std::map<std::string, std::string>':
                % if 'map<' in method['response_field']['type']:
                std::map<std::string, std::string> result;
                for (const auto& pair : response.${method['response_field']['name']}()) {
                    result[pair.first] = pair.second;
                }
                UHD_LOG_TRACE("RPC", "<<< ${method['camel_name']}[db=" << db_idx_ << "] = map[" << result.size() << " entries]");
                return result;
                % elif method['response_field']['type'] == 'SensorValueMap':
                auto result = parent_->sensorvaluemap_to_stdmap(response.${method['response_field']['name']}());
                UHD_LOG_TRACE("RPC", "<<< ${method['camel_name']}[db=" << db_idx_ << "] = sensor_map[" << result.size() << " entries]");
                return result;
                % else:
                auto result = parent_->stringmap_to_stdmap(response.${method['response_field']['name']}());
                UHD_LOG_TRACE("RPC", "<<< ${method['camel_name']}[db=" << db_idx_ << "] = map[" << result.size() << " entries]");
                return result;
                % endif
            % elif method['return_type'] == 'std::vector<std::map<std::string, std::string>>':
                auto result = parent_->repeated_stringmap_to_vector(response.${method['response_field']['name']}());
                UHD_LOG_TRACE("RPC", "<<< ${method['camel_name']}[db=" << db_idx_ << "] = vector<map>[" << result.size() << " items]");
                return result;
            % elif method['return_type'].startswith('std::pair<'):
                // Handle pair return type
                <%
                    f0 = method['pair_field_names'][0]
                    f1 = method['pair_field_names'][1]
                    inner = method['return_type'][len('std::pair<'):-1]
                    depth, split = 0, 0
                    for _i, _c in enumerate(inner):
                        if _c == '<': depth += 1
                        elif _c == '>': depth -= 1
                        elif _c == ',' and depth == 0: split = _i; break
                    f0_type = inner[:split].strip()
                    f1_type = inner[split+1:].strip()
                %>
                ${f0_type} ${f0} = response.${f0}();
                % if f1_type.startswith('std::vector<'):
                ${f1_type} ${f1}(response.${f1}().begin(), response.${f1}().end());
                % else:
                ${f1_type} ${f1} = response.${f1}();
                % endif
                auto result = std::make_pair(${f0}, ${f1});
                % if f1_type.startswith('std::vector<'):
                UHD_LOG_TRACE("RPC", "<<< ${method['camel_name']}[db=" << db_idx_ << "] = pair(${f0}=" << result.first << ", ${f1}=[" << result.second.size() << " items])");
                % else:
                UHD_LOG_TRACE("RPC", "<<< ${method['camel_name']}[db=" << db_idx_ << "] = pair(${f0}=" << result.first << ", ${f1}=" << result.second << ")");
                % endif
                return result;
            % elif method['return_type'].startswith('std::vector<'):
                // Convert protobuf repeated field to std::vector
                ${method['return_type']} result;
                for (const auto& item : response.${method['response_field']['name']}()) {
                    result.push_back(item);
                }
                UHD_LOG_TRACE("RPC", "<<< ${method['camel_name']}[db=" << db_idx_ << "] = vector[" << result.size() << " items]");
                return result;
            % else:
                auto result = response.${method['response_field']['name']}();
                UHD_LOG_TRACE("RPC", "<<< ${method['camel_name']}[db=" << db_idx_ << "] = " << result);
                return result;
            % endif
            } else if (status.error_code() == grpc::StatusCode::PERMISSION_DENIED) {
                UHD_LOG_TRACE("RPC", "!!! ${method['camel_name']}[db=" << db_idx_ << "] FAILED [" << status.error_code() << "]: Authentication error");
                throw rpc_exception("Authentication failed: " + status.error_message());
            } else {
                UHD_LOG_TRACE("RPC", "!!! ${method['camel_name']}[db=" << db_idx_ << "] FAILED [" << status.error_code() << "]: " << status.error_message());
                throw rpc_exception("${method['camel_name']} RPC failed: " + status.error_message());
            }
        }

% endfor
    };

    uhd::rpc_client::dboard_iface& get_dboard(size_t db_idx) override {
        // Validate db_idx - supports 2 daughterboards (0, 1)
        if (db_idx >= MAX_DBOARDS) {
            throw rpc_exception("Invalid daughterboard index: " + std::to_string(db_idx) +
                              ". Valid range is 0-" + std::to_string(MAX_DBOARDS-1));
        }

        return *dboard_instances_[db_idx];
    }

    // Make helper methods accessible to dboard implementation
    friend class dboard_iface_impl;
};

std::atomic<uint64_t> rpc_client_impl::_client_id_counter{0};

// Factory implementation
uhd::rpc_client::sptr uhd::rpc_client::make(const std::string& server_name, uint16_t port, uint64_t timeout_ms) {
    return std::make_shared<rpc_client_impl>(server_name, port, timeout_ms);
}

// clang-format on