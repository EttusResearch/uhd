//
// Copyright 2017 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "mpmd_impl.hpp"
#include "mpmd_mb_iface.hpp"
#include <uhd/transport/udp_simple.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/safe_call.hpp>
#include <chrono>
#include <memory>
#include <thread>

namespace {
/*************************************************************************
 * Local constants
 ************************************************************************/
//! Timeout for pings (seconds).
constexpr double MPMD_PING_TIMEOUT = 0.1;
//! Default session ID (MPM will recognize a session by this name)
const std::string MPMD_DEFAULT_SESSION_ID = "UHD";
//! Key to initialize a ping/measurement latency test
const std::string MPMD_MEAS_LATENCY_KEY = "measure_rpc_latency";
//! Duration of a latency measurement test
constexpr size_t MPMD_MEAS_LATENCY_DURATION = 1000;

using log_buf_t = std::vector<std::map<std::string, std::string>>;


/*************************************************************************
 * Helper functions
 ************************************************************************/
/*! Return true if we can MPM ping a device via discovery service.
 */
bool is_pingable(const std::string& ip_addr, const std::string& udp_port)
{
    auto udp = uhd::transport::udp_simple::make_broadcast(ip_addr, udp_port);
    const std::string send_buf(uhd::mpmd::mpmd_impl::MPM_ECHO_CMD + " ping");
    std::vector<uint8_t> recv_buf;
    recv_buf.resize(send_buf.size(), ' ');
    udp->send(boost::asio::buffer(send_buf.c_str(), send_buf.size()));
    const size_t len = udp->recv(boost::asio::buffer(recv_buf), MPMD_PING_TIMEOUT);
    if (len == 0) {
        UHD_LOG_TRACE("MPMD", "Received no MPM ping, assuming device is unreachable.");
        return false;
    }
    if (len != send_buf.size()) {
        UHD_LOG_DEBUG("MPMD",
            "Received bad return packet on MPM ping. Assuming endpoint"
            " is not a valid MPM device.");
        return false;
    }
    if (std::memcmp((void*)&recv_buf[0], (void*)&send_buf[0], send_buf.size()) != 0) {
        UHD_LOG_DEBUG("MPMD",
            "Received invalid return packet on MPM ping. Assuming endpoint"
            " is not a valid MPM device.");
        return false;
    }
    return true;
}

/*! Call init() on an MPM device.
 */
void init_device(uhd::rpc_client::sptr rpc, const uhd::device_addr_t mb_args)
{
    auto init_status = rpc->request_with_token<std::vector<std::string>>(
        MPMD_DEFAULT_INIT_TIMEOUT, "get_init_status");
    if (init_status[0] != "true") {
        throw uhd::runtime_error(
            std::string("Device is in bad state: ") + init_status[1]);
    }

    // TODO maybe put this somewhere else?
    const std::set<std::string> key_blacklist{"serial", "claimed", "type", "rev", "addr"};
    std::map<std::string, std::string> mpm_device_args;
    for (const auto& key : mb_args.keys()) {
        if (not key_blacklist.count(key)) {
            mpm_device_args[key] = mb_args[key];
        }
    }
    if (not rpc->request_with_token<bool>(
            MPMD_DEFAULT_INIT_TIMEOUT, "init", mpm_device_args)) {
        throw uhd::runtime_error("Failed to initialize device.");
    }
}

void measure_rpc_latency(
    uhd::rpc_client::sptr rpc, const size_t duration_ms = MPMD_MEAS_LATENCY_DURATION)
{
    const double alpha        = 0.99;
    const std::string payload = "1234567890";
    auto measure_once         = [payload, rpc]() {
        const auto start = std::chrono::steady_clock::now();
        rpc->request<std::string>("ping", payload);
        return (double)std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - start)
            .count();
    };

    double max_latency = measure_once();
    double avg_latency = max_latency;

    auto end_time =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(duration_ms);
    size_t ctr = 1;
    while (std::chrono::steady_clock::now() < end_time) {
        const auto duration = measure_once();
        max_latency         = std::max(max_latency, duration);
        avg_latency         = avg_latency * alpha + (1 - alpha) * duration;
        ctr++;
        // Light throttle:
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    UHD_LOG_INFO("MPMD",
        "RPC latency (coarse estimate): Avg = " << avg_latency
                                                << " us, "
                                                   "Max = "
                                                << max_latency << ", n = " << ctr);
}

/*! Forward entries from a list of dictionaries to UHD's native logging
 *  system.
 */
void forward_logs(log_buf_t&& log_buf)
{
    for (const auto& log_record : log_buf) {
        if (log_record.count("levelname") == 0 or log_record.count("message") == 0) {
            UHD_LOG_ERROR("MPMD", "Invalid logging structure returned from MPM device!");
            continue;
        }
        if (log_record.at("levelname") == "TRACE") {
            UHD_LOG_TRACE(log_record.at("name"), log_record.at("message"));
        } else if (log_record.at("levelname") == "DEBUG") {
            UHD_LOG_DEBUG(log_record.at("name"), log_record.at("message"));
        } else if (log_record.at("levelname") == "INFO") {
            UHD_LOG_INFO(log_record.at("name"), log_record.at("message"));
        } else if (log_record.at("levelname") == "WARNING") {
            UHD_LOG_WARNING(log_record.at("name"), log_record.at("message"));
        } else if (log_record.at("levelname") == "ERROR") {
            UHD_LOG_ERROR(log_record.at("name"), log_record.at("message"));
        } else if (log_record.at("levelname") == "CRITICAL") {
            UHD_LOG_FATAL(log_record.at("name"), log_record.at("message"));
        } else {
            UHD_LOG_ERROR("MPMD",
                "Invalid log level returned from MPM device: "
                "`" << log_record.at("levelname")
                    << "'");
        }
    }
}

/*! Return a new rpc_client with given addr and mb args
 */
uhd::rpc_client::sptr make_mpm_rpc_client(const std::string& rpc_server_addr,
    const uhd::device_addr_t& mb_args,
    const size_t timeout_ms = MPMD_DEFAULT_RPC_TIMEOUT)
{
    return uhd::rpc_client::make(rpc_server_addr,
        mb_args.cast<size_t>(
            uhd::mpmd::mpmd_impl::MPM_RPC_PORT_KEY, uhd::mpmd::mpmd_impl::MPM_RPC_PORT),
        timeout_ms,
        uhd::mpmd::mpmd_impl::MPM_RPC_GET_LAST_ERROR_CMD);
}

} // namespace

using namespace uhd;
using namespace uhd::mpmd;

/******************************************************************************
 * Static Helpers
 *****************************************************************************/
boost::optional<device_addr_t> mpmd_mboard_impl::is_device_reachable(
    const device_addr_t& device_addr)
{
    UHD_LOG_TRACE(
        "MPMD", "Checking accessibility of device `" << device_addr.to_string() << "'");
    UHD_ASSERT_THROW(device_addr.has_key(MGMT_ADDR_KEY));
    const std::string rpc_addr = device_addr.get(MGMT_ADDR_KEY);
    const size_t rpc_port =
        device_addr.cast<size_t>(mpmd_impl::MPM_RPC_PORT_KEY, mpmd_impl::MPM_RPC_PORT);
    // 1) Read back device info
    dev_info device_info_dict;
    try {
        auto rpcc = uhd::rpc_client::make(rpc_addr, rpc_port);
        device_info_dict =
            rpcc->request<dev_info>(MPMD_SHORT_RPC_TIMEOUT, "get_device_info");
    } catch (const uhd::runtime_error& e) {
        UHD_LOG_DEBUG("MPMD", e.what());
        return boost::optional<device_addr_t>();
    } catch (...) {
        UHD_LOG_DEBUG("MPMD",
            "Unexpected exception when trying to query device info. Flagging "
            "device as unreachable.");
        return boost::optional<device_addr_t>();
    }
    // 2) Check for local device
    if (device_info_dict.count("connection")
        and device_info_dict.at("connection") == "local") {
        UHD_LOG_TRACE("MPMD", "Device is local, flagging as reachable.");
        return boost::optional<device_addr_t>(device_addr);
    }
    // 3) Check for network-reachable device
    // Note: This makes the assumption that devices will always allow RPC
    // connections on their CHDR addresses.
    const std::vector<std::string> addr_keys = {"second_addr", "addr", "third_addr", "fourth_addr"};
    bool addr_key_found = false;
    for (const auto& addr_key : addr_keys) {
        if (not device_info_dict.count(addr_key)) {
            continue;
        }
        addr_key_found = true;
        const std::string chdr_addr = device_info_dict.at(addr_key);
        UHD_LOG_TRACE("MPMD", "Checking reachability via network addr " << chdr_addr);
        try {
            // First do an MPM ping -- there is some issue with rpclib that can
            // lead to indefinite timeouts
            const std::string mpm_discovery_port =
                device_addr.get(mpmd_impl::MPM_DISCOVERY_PORT_KEY,
                    std::to_string(mpmd_impl::MPM_DISCOVERY_PORT));
            if (!is_pingable(chdr_addr, mpm_discovery_port)) {
                UHD_LOG_TRACE("MPMD", "Cannot MPM ping, assuming device is unreachable.");
                continue;
            }
            UHD_LOG_TRACE("MPMD", "Was able to ping device, trying RPC connection.");
            auto chdr_rpcc = uhd::rpc_client::make(chdr_addr, rpc_port);
            auto dev_info_chdr =
                chdr_rpcc->request<dev_info>(MPMD_SHORT_RPC_TIMEOUT, "get_device_info");
            if (dev_info_chdr["serial"] != device_info_dict["serial"]) {
                UHD_LOG_DEBUG("MPMD",
                    boost::format("Connected to CHDR interface, but got wrong device. "
                                  "Tried to reach serial %s, got %s")
                        % device_info_dict["serial"] % dev_info_chdr["serial"]);
                continue;
            } else {
                UHD_LOG_TRACE("MPMD",
                    boost::format("Reachable device matches expected device (serial=%s)")
                        % device_info_dict["serial"]);
            }
            device_addr_t device_addr_copy = device_addr;
            device_addr_copy["addr"]       = chdr_addr;
            return boost::optional<device_addr_t>(device_addr_copy);
        } catch (...) {
            UHD_LOG_DEBUG(
                "MPMD", "Failed to reach device on network addr " << chdr_addr << ".");
        }
    }
    if(!addr_key_found)
    {
        // 4) get_device_info didn't give us CHDR info
        // This could be because the device isn't fully 
        // initialized (e.g. e31x with a power save FPGA).
        // For UHD 4.0+, the mgmt interface will always 
        // route CHDR packets when fully initialized
        // via Virtual NIC packet fowarding.
        device_addr_t device_addr_copy = device_addr;
        device_addr_copy["addr"]       = rpc_addr;
        return boost::optional<device_addr_t>(device_addr_copy);
    }
    // If everything fails, we probably can't talk to this chap.
    UHD_LOG_TRACE(
        "MPMD", "All reachability checks failed -- assuming device is not reachable.");
    return boost::optional<device_addr_t>();
}

/*****************************************************************************
 * Structors
 ****************************************************************************/
mpmd_mboard_impl::mpmd_mboard_impl(
    const device_addr_t& mb_args_, const std::string& rpc_server_addr)
    : mb_args(mb_args_)
    , rpc(make_mpm_rpc_client(rpc_server_addr, mb_args_))
    , _claim_rpc(make_mpm_rpc_client(rpc_server_addr, mb_args, MPMD_CLAIMER_RPC_TIMEOUT))
{
    UHD_LOGGER_TRACE("MPMD") << "Initializing mboard, connecting to RPC server address: "
                             << rpc_server_addr
                             << " mboard args: " << mb_args.to_string();

    _claimer_task = claim_device_and_make_task();
    if (mb_args_.has_key(MPMD_MEAS_LATENCY_KEY)) {
        measure_rpc_latency(rpc, MPMD_MEAS_LATENCY_DURATION);
    }

    /// Get device info
    const auto device_info_dict = rpc->request<dev_info>("get_device_info");
    for (const auto& info_pair : device_info_dict) {
        device_info[info_pair.first] = info_pair.second;
    }
    UHD_LOG_DEBUG("MPMD", "MPM reports device info: " << device_info.to_string());
    /// Get dboard info
    const auto dboards_info = rpc->request<std::vector<dev_info>>("get_dboard_info");
    UHD_ASSERT_THROW(this->dboard_info.empty());
    for (const auto& dboard_info_dict : dboards_info) {
        uhd::device_addr_t this_db_info;
        for (const auto& info_pair : dboard_info_dict) {
            this_db_info[info_pair.first] = info_pair.second;
        }
        UHD_LOGGER_TRACE("MPMD")
            << "MPM reports dboard info for slot " << this->dboard_info.size() << ": "
            << this_db_info.to_string();
        this->dboard_info.push_back(this_db_info);
    }

    if (!mb_args.has_key("skip_init")) {
        // Initialize mb_iface and mb_controller
        mb_iface = std::make_unique<mpmd_mb_iface>(mb_args, rpc);
        mb_ctrl  = std::make_shared<rfnoc::mpmd_mb_controller>(std::make_shared<uhd::usrp::mpmd_rpc>(rpc), device_info);
    } // Note -- when skip_init is used, these are not initialized, and trying
      // to use them will result in a null pointer dereference exception!
}

mpmd_mboard_impl::~mpmd_mboard_impl()
{
    // Destroy the claimer task to avoid spurious asynchronous reclaim call
    // after the unclaim.
    UHD_SAFE_CALL(dump_logs(); _claimer_task.reset();
                  if (not rpc->request_with_token<bool>("unclaim")) {
                      UHD_LOG_WARNING("MPMD", "Failure to ack unclaim!");
                  });
}

/*****************************************************************************
 * Init
 ****************************************************************************/
void mpmd_mboard_impl::init()
{
    init_device(rpc, mb_args);
    mb_iface->init();
}

/*****************************************************************************
 * API
 ****************************************************************************/
uhd::rfnoc::mb_iface& mpmd_mboard_impl::get_mb_iface()
{
    return *(mb_iface.get());
}

/*****************************************************************************
 * Private methods
 ****************************************************************************/
bool mpmd_mboard_impl::claim()
{
    try {
        auto result = _claim_rpc->request_with_token<bool>("reclaim");
        // When _allow_claim_failure_flag goes from true to false, we still have
        // to wait for a successful reclaim before we can also set
        // _allow_claim_failure_latch to false, because we have no way of
        // synchronizing those events.
        // In other words, we might be setting allow_claim_failure back to false
        // too soon, but we have no way of knowing exactly when the right time
        // is.
        _allow_claim_failure_latch = _allow_claim_failure_flag.load();
        return result;
    } catch (const uhd::runtime_error& ex) {
        // Note: Any RPC error will raise a uhd::runtime_error. Other errors are
        // not handled here.
        if (_allow_claim_failure_latch) {
            UHD_LOG_DEBUG("MPMD", ex.what());
        } else {
            UHD_LOG_WARNING("MPMD", ex.what());
        }
        return _allow_claim_failure_latch;
    }
}

uhd::task::sptr mpmd_mboard_impl::claim_device_and_make_task()
{
    auto rpc_token = _claim_rpc->request<std::string>(
        "claim", mb_args.get("session_id", MPMD_DEFAULT_SESSION_ID));
    if (rpc_token.empty()) {
        throw uhd::value_error("mpmd device claiming failed!");
    }
    UHD_LOG_TRACE("MPMD", "Received claim token " << rpc_token);
    // Save token for both RPC clients
    _claim_rpc->set_token(rpc_token);
    rpc->set_token(rpc_token);
    _token = rpc_token;
    // Optionally clear log buf
    if (mb_args.has_key("skip_oldlog")) {
        try {
            this->dump_logs(true);
        } catch (const uhd::runtime_error&) {
            UHD_LOG_WARNING("MPMD", "Could not read back log queue!");
        }
    }
    return uhd::task::make(
        [this] {
            auto now = std::chrono::steady_clock::now();
            if (not this->claim()) {
                throw uhd::value_error("mpmd device reclaiming loop failed!");
            } else {
                try {
                    this->dump_logs();
                } catch (const uhd::runtime_error&) {
                    UHD_LOG_WARNING("MPMD", "Could not read back log queue!");
                }
            }
            std::this_thread::sleep_until(
                now + std::chrono::milliseconds(MPMD_RECLAIM_INTERVAL_MS));
        },
        "mpmd_claimer_task");
}

void mpmd_mboard_impl::dump_logs(const bool dump_to_null)
{
    // We need to use _claim_rpc instead of rpc because this currently only
    // gets called in the claimer loop.
    if (dump_to_null) {
        _claim_rpc->request_with_token<log_buf_t>("get_log_buf");
    } else {
        forward_logs(_claim_rpc->request_with_token<log_buf_t>("get_log_buf"));
    }
}


/*****************************************************************************
 * Factory
 ****************************************************************************/
mpmd_mboard_impl::uptr mpmd_mboard_impl::make(
    const uhd::device_addr_t& mb_args, const std::string& addr)
{
    mpmd_mboard_impl::uptr mb =
        mpmd_mboard_impl::uptr(new mpmd_mboard_impl(mb_args, addr));
    // implicit move
    return mb;
}
