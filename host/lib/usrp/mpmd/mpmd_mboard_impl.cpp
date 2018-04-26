//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "mpmd_impl.hpp"
#include <uhd/utils/safe_call.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/transport/udp_simple.hpp>
#include <chrono>
#include <thread>

namespace {
    /*************************************************************************
     * Local constants
     ************************************************************************/
    //! Time between reclaims (ms)
    const size_t MPMD_RECLAIM_INTERVAL_MS     = 1000;
    //! Default timeout value for the init() RPC call (ms)
    const size_t MPMD_DEFAULT_INIT_TIMEOUT    = 120000;
    //! Default timeout value for RPC calls (ms)
    constexpr size_t MPMD_DEFAULT_RPC_TIMEOUT     = 2000;
    //! Short timeout value for RPC calls (ms), used for calls that shouldn't
    // take long. This value can be used to quickly determine a link status.
    constexpr size_t MPMD_SHORT_RPC_TIMEOUT     = 2000;
    //! Timeout for pings (seconds).
    constexpr double MPMD_PING_TIMEOUT          = 0.1;
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
        auto udp = uhd::transport::udp_simple::make_broadcast(
            ip_addr,
            udp_port
        );
        const std::string send_buf(
            uhd::mpmd::mpmd_impl::MPM_ECHO_CMD + " ping"
        );
        std::vector<uint8_t> recv_buf;
        recv_buf.resize(send_buf.size(), ' ');
        udp->send(boost::asio::buffer(send_buf.c_str(), send_buf.size()));
        const size_t len =
            udp->recv(boost::asio::buffer(recv_buf), MPMD_PING_TIMEOUT);
        if (len == 0) {
            UHD_LOG_TRACE("MPMD",
                "Received no MPM ping, assuming device is unreachable.");
            return false;
        }
        if (len != send_buf.size()) {
            UHD_LOG_DEBUG("MPMD",
                "Received bad return packet on MPM ping. Assuming endpoint"
                " is not a valid MPM device.");
            return false;
        }
        if (std::memcmp(
                    (void *) &recv_buf[0],
                    (void *) &send_buf[0],
                    send_buf.size()) != 0) {
            UHD_LOG_DEBUG("MPMD",
                "Received invalid return packet on MPM ping. Assuming endpoint"
                " is not a valid MPM device.");
            return false;
        }
        return true;
    }

    /*! Call init() on an MPM device.
     */
    void init_device(
            uhd::rpc_client::sptr rpc,
            const uhd::device_addr_t mb_args
    ) {
        auto init_status =
            rpc->request_with_token<std::vector<std::string>>(
                "get_init_status");
        if (init_status[0] != "true") {
            throw uhd::runtime_error(
                std::string("Device is in bad state: ") + init_status[1]
            );
        }

        // TODO maybe put this somewhere else?
        const std::set<std::string> key_blacklist{
            "serial", "claimed", "type", "rev", "addr"
        };
        std::map<std::string, std::string> mpm_device_args;
        for (const auto &key : mb_args.keys()) {
            if (not key_blacklist.count(key)) {
                mpm_device_args[key] = mb_args[key];
            }
        }
        if (not rpc->request_with_token<bool>("init", mpm_device_args)) {
            throw uhd::runtime_error("Failed to initialize device.");
        }
    }

    void measure_rpc_latency(
        uhd::rpc_client::sptr rpc,
        const size_t duration_ms=MPMD_MEAS_LATENCY_DURATION
    ) {
        const double alpha = 0.99;
        const std::string payload = "1234567890";
        auto measure_once = [payload, rpc](){
            const auto start = std::chrono::steady_clock::now();
            rpc->request<std::string>("ping", payload);
            return (double) std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now() - start
            ).count();
        };

        double max_latency = measure_once();
        double avg_latency = max_latency;

        auto end_time = std::chrono::steady_clock::now()
                            + std::chrono::milliseconds(duration_ms);
        size_t ctr = 1;
        while (std::chrono::steady_clock::now() < end_time) {
            const auto duration = measure_once();
            max_latency = std::max(max_latency, duration);
            avg_latency = avg_latency * alpha + (1-alpha) * duration;
            ctr++;
            // Light throttle:
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        UHD_LOG_INFO("MPMD",
            "RPC latency (coarse estimate): Avg = " << avg_latency << " us, "
            "Max = " << max_latency
            << ", n = " << ctr);
    }

    /*! Forward entries from a list of dictionaries to UHD's native logging
     *  system.
     */
    void forward_logs(log_buf_t&& log_buf)
    {
        for (const auto &log_record : log_buf) {
            if (log_record.count("levelname") == 0 or \
                    log_record.count("message") == 0) {
                UHD_LOG_ERROR("MPMD",
                    "Invalid logging structure returned from MPM device!");
                continue;
            }
            if (log_record.at("levelname") == "TRACE") {
                UHD_LOG_TRACE(
                    log_record.at("name"),
                    log_record.at("message")
                );
            }
            else if (log_record.at("levelname") == "DEBUG") {
                UHD_LOG_DEBUG(
                    log_record.at("name"),
                    log_record.at("message")
                );
            }
            else if (log_record.at("levelname") == "INFO") {
                UHD_LOG_INFO(
                    log_record.at("name"),
                    log_record.at("message")
                );
            }
            else if (log_record.at("levelname") == "WARNING") {
                UHD_LOG_WARNING(
                    log_record.at("name"),
                    log_record.at("message")
                );
            }
            else if (log_record.at("levelname") == "ERROR") {
                UHD_LOG_ERROR(
                    log_record.at("name"),
                    log_record.at("message")
                );
            }
            else if (log_record.at("levelname") == "CRITICAL") {
                UHD_LOG_FATAL(
                    log_record.at("name"),
                    log_record.at("message")
                );
            } else {
                UHD_LOG_ERROR("MPMD",
                    "Invalid log level returned from MPM device: "
                    "`" << log_record.at("levelname") << "'");
            }
        }
    }

    /*! Return a new rpc_client with given addr and mb args
     */
    uhd::rpc_client::sptr make_mpm_rpc_client(
        const std::string& rpc_server_addr,
        const uhd::device_addr_t& mb_args
    ){
        return uhd::rpc_client::make(
            rpc_server_addr,
            mb_args.cast<size_t>(
                uhd::mpmd::mpmd_impl::MPM_RPC_PORT_KEY,
                uhd::mpmd::mpmd_impl::MPM_RPC_PORT
            ),
            uhd::mpmd::mpmd_impl::MPM_RPC_GET_LAST_ERROR_CMD);
    }

}

using namespace uhd;
using namespace uhd::mpmd;

/******************************************************************************
 * Static Helpers
 *****************************************************************************/
boost::optional<device_addr_t> mpmd_mboard_impl::is_device_reachable(
        const device_addr_t &device_addr
) {
    UHD_LOG_TRACE("MPMD",
        "Checking accessibility of device `" << device_addr.to_string()
        << "'");
    UHD_ASSERT_THROW(device_addr.has_key(xport::MGMT_ADDR_KEY));
    const std::string rpc_addr = device_addr.get(xport::MGMT_ADDR_KEY);
    const size_t rpc_port = device_addr.cast<size_t>(
        mpmd_impl::MPM_RPC_PORT_KEY,
        mpmd_impl::MPM_RPC_PORT
    );
    auto rpcc = uhd::rpc_client::make(rpc_addr, rpc_port);
    rpcc->set_timeout(MPMD_SHORT_RPC_TIMEOUT);
    // 1) Read back device info
    dev_info device_info_dict;
    try {
        device_info_dict = rpcc->request<dev_info>("get_device_info");
    } catch (const uhd::runtime_error& e) {
        UHD_LOG_ERROR("MPMD", e.what());
    } catch (...) {
        UHD_LOG_DEBUG("MPMD",
            "Unexpected exception when trying to query device info. Flagging "
            "device as unreachable.");
        return boost::optional<device_addr_t>();
    }
    // 2) Check for local device
    if (device_info_dict.count("connection") and
            device_info_dict.at("connection") == "local") {
        UHD_LOG_TRACE("MPMD", "Device is local, flagging as reachable.");
        return boost::optional<device_addr_t>(device_addr);
    }
    // 3) Check for network-reachable device
    // Note: This makes the assumption that devices will always allow RPC
    // connections on their CHDR addresses.
    const std::vector<std::string> addr_keys = {"second_addr", "addr"};
    for (const auto& addr_key : addr_keys) {
        if (not device_info_dict.count(addr_key)) {
            continue;
        }
        const std::string chdr_addr = device_info_dict.at(addr_key);
        UHD_LOG_TRACE("MPMD",
            "Checking reachability via network addr " << chdr_addr);
        try {
            // First do an MPM ping -- there is some issue with rpclib that can
            // lead to indefinite timeouts
            const std::string mpm_discovery_port = device_addr.get(
                mpmd_impl::MPM_DISCOVERY_PORT_KEY,
                std::to_string(mpmd_impl::MPM_DISCOVERY_PORT)
            );
            if (!is_pingable(chdr_addr, mpm_discovery_port)) {
                UHD_LOG_TRACE("MPMD",
                    "Cannot MPM ping, assuming device is unreachable.");
                continue;
            }
            UHD_LOG_TRACE("MPMD",
                "Was able to ping device, trying RPC connection.");
            auto chdr_rpcc = uhd::rpc_client::make(chdr_addr, rpc_port);
            chdr_rpcc->set_timeout(MPMD_SHORT_RPC_TIMEOUT);
            auto dev_info_chdr = chdr_rpcc->request<dev_info>("get_device_info");
            if (dev_info_chdr["serial"] != device_info_dict["serial"]) {
                UHD_LOG_DEBUG("MPMD", boost::format(
                    "Connected to CHDR interface, but got wrong device. "
                    "Tried to reach serial %s, got %s")
                     % device_info_dict["serial"] % dev_info_chdr["serial"]);
                return boost::optional<device_addr_t>();
            } else {
                UHD_LOG_TRACE("MPMD", boost::format(
                    "Reachable device matches expected device (serial=%s)")
                    % device_info_dict["serial"] );
            }
            device_addr_t device_addr_copy = device_addr;
            device_addr_copy["addr"] = chdr_addr;
            return boost::optional<device_addr_t>(device_addr_copy);
        } catch (...) {
            UHD_LOG_DEBUG("MPMD",
                "Failed to reach device on network addr " << chdr_addr << ".");
        }
    }
    // If everything fails, we probably can't talk to this chap.
    UHD_LOG_TRACE("MPMD",
        "All reachability checks failed -- assuming device is not reachable.");
    return boost::optional<device_addr_t>();
}

/*****************************************************************************
 * Structors
 ****************************************************************************/
mpmd_mboard_impl::mpmd_mboard_impl(
        const device_addr_t &mb_args_,
        const std::string& rpc_server_addr
) : mb_args(mb_args_)
    , rpc(make_mpm_rpc_client(rpc_server_addr, mb_args_))
    , num_xbars(rpc->request<size_t>("get_num_xbars"))
    , _claim_rpc(make_mpm_rpc_client(rpc_server_addr, mb_args_))
    // xbar_local_addrs is not yet valid after this!
    , xbar_local_addrs(num_xbars, 0xFF)
    , _xport_mgr(xport::mpmd_xport_mgr::make(mb_args))
{
    UHD_LOGGER_TRACE("MPMD")
        << "Initializing mboard, connecting to RPC server address: "
        << rpc_server_addr
        << " mboard args: " << mb_args.to_string()
        << " number of crossbars: " << num_xbars
    ;

    _claimer_task = claim_device_and_make_task();
    if (mb_args_.has_key(MPMD_MEAS_LATENCY_KEY)) {
        measure_rpc_latency(rpc, MPMD_MEAS_LATENCY_DURATION);
    }

    /// Get device info
    const auto device_info_dict = rpc->request<dev_info>("get_device_info");
    for (const auto &info_pair : device_info_dict) {
        device_info[info_pair.first] = info_pair.second;
    }
    UHD_LOGGER_TRACE("MPMD")
        << "MPM reports device info: " << device_info.to_string();
    /// Get dboard info
    const auto dboards_info =
        rpc->request<std::vector<dev_info>>("get_dboard_info");
    UHD_ASSERT_THROW(this->dboard_info.size() == 0);
    for (const auto &dboard_info_dict : dboards_info) {
        uhd::device_addr_t this_db_info;
        for (const auto &info_pair : dboard_info_dict) {
            this_db_info[info_pair.first] = info_pair.second;
        }
        UHD_LOGGER_TRACE("MPMD")
            << "MPM reports dboard info for slot " << this->dboard_info.size()
            << ": " << this_db_info.to_string();
        this->dboard_info.push_back(this_db_info);
    }
}

mpmd_mboard_impl::~mpmd_mboard_impl()
{
    try {
        dump_logs();
    } catch (...) {
        UHD_LOG_WARNING("MPMD", "Could not flush log queue on exit!");
    }
    UHD_SAFE_CALL(
        if (not rpc->request_with_token<bool>("unclaim")) {
            UHD_LOG_WARNING("MPMD", "Failure to ack unclaim!");
        }
    );
}

/*****************************************************************************
 * Init
 ****************************************************************************/
void mpmd_mboard_impl::init()
{
    this->set_rpcc_timeout(mb_args.cast<size_t>(
        "init_timeout", MPMD_DEFAULT_INIT_TIMEOUT
    ));
    init_device(rpc, mb_args);
    this->set_rpcc_timeout(mb_args.cast<size_t>(
        "rpc_timeout", MPMD_DEFAULT_RPC_TIMEOUT
    ));
    // RFNoC block clocks are now on. Noc-IDs can be read back.
}

/*****************************************************************************
 * API
 ****************************************************************************/
void mpmd_mboard_impl::set_xbar_local_addr(
        const size_t xbar_index,
        const size_t local_addr
) {
    UHD_ASSERT_THROW(rpc->request_with_token<bool>("set_xbar_local_addr", xbar_index, local_addr));
    UHD_ASSERT_THROW(xbar_index < xbar_local_addrs.size());
    xbar_local_addrs.at(xbar_index) = local_addr;
}

uhd::both_xports_t mpmd_mboard_impl::make_transport(
        const sid_t& sid,
        usrp::device3_impl::xport_type_t xport_type,
        const uhd::device_addr_t& xport_args
) {
    const std::string xport_type_str = [xport_type](){
        switch (xport_type) {
        case mpmd_impl::CTRL:
            return "CTRL";
        case mpmd_impl::ASYNC_MSG:
            return "ASYNC_MSG";
        case mpmd_impl::RX_DATA:
            return "RX_DATA";
        case mpmd_impl::TX_DATA:
            return "TX_DATA";
        default:
            UHD_THROW_INVALID_CODE_PATH();
        };
    }();

    UHD_LOGGER_TRACE("MPMD")
        << __func__ << "(): Creating new transport of type: "
        << xport_type_str
    ;

    using namespace uhd::mpmd::xport;
    const auto xport_info_list =
        rpc->request_with_token<mpmd_xport_mgr::xport_info_list_t>(
            "request_xport",
            sid.get_dst(),
            sid.get_src(),
            xport_type_str
    );
    UHD_LOGGER_TRACE("MPMD")
        << __func__
        << "(): request_xport() gave us " << xport_info_list.size()
        << " option(s)."
    ;
    if (xport_info_list.empty()) {
        UHD_LOG_ERROR("MPMD", "No viable transport path found!");
        throw uhd::runtime_error("No viable transport path found!");
    }

    xport::mpmd_xport_mgr::xport_info_t xport_info_out;
    auto xports = _xport_mgr->make_transport(
        xport_info_list,
        xport_type,
        xport_args,
        xport_info_out
    );

    if (not rpc->request_with_token<bool>(
                "commit_xport",
                xport_info_out)) {
        UHD_LOG_ERROR("MPMD", "Failed to create UDP transport!");
        throw uhd::runtime_error("commit_xport() failed!");
    }

    return xports;
}

size_t mpmd_mboard_impl::get_mtu(const uhd::direction_t dir) const
{
    return _xport_mgr->get_mtu(dir);
}

uhd::device_addr_t mpmd_mboard_impl::get_rx_hints() const
{
    // TODO: See if we need to do anything here. get_rx_stream() might care.
    device_addr_t rx_hints;
    return rx_hints;
}

uhd::device_addr_t mpmd_mboard_impl::get_tx_hints() const
{
    // TODO: See if we need to do anything here. get_tx_stream() might care.
    device_addr_t tx_hints;
    return tx_hints;
}

void mpmd_mboard_impl::set_timeout_default()
{
    this->set_rpcc_timeout(mb_args.cast<size_t>(
            "rpc_timeout", MPMD_DEFAULT_RPC_TIMEOUT
    ));
}


/*****************************************************************************
 * Private methods
 ****************************************************************************/
bool mpmd_mboard_impl::claim()
{
    try {
        return _claim_rpc->request_with_token<bool>("reclaim");
    } catch (...) {
        UHD_LOG_WARNING("MPMD", "Reclaim failed. Exiting claimer loop.");
        return false;
    }
}

void mpmd_mboard_impl::set_rpcc_timeout(const uint64_t timeout_ms){
    rpc->set_timeout(timeout_ms);
    //FIXME: remove this when we know why rpc client didn't reset timer
    // while other rpc client not yet return.
    _claim_rpc->set_timeout(timeout_ms);
}

uhd::task::sptr mpmd_mboard_impl::claim_device_and_make_task(
) {
    auto rpc_token = _claim_rpc->request<std::string>("claim",
        mb_args.get("session_id", MPMD_DEFAULT_SESSION_ID)
    );
    if (rpc_token.empty()) {
        throw uhd::value_error("mpmd device claiming failed!");
    }
    UHD_LOG_TRACE("MPMD", "Received claim token " << rpc_token);
    // Save token for both RPC clients
    _claim_rpc->set_token(rpc_token);
    rpc->set_token(rpc_token);
    return uhd::task::make([this] {
        auto now = std::chrono::steady_clock::now();
        if (not this->claim()) {
            throw uhd::value_error("mpmd device reclaiming loop failed!");
        };
        this->dump_logs();
        std::this_thread::sleep_until(
            now + std::chrono::milliseconds(MPMD_RECLAIM_INTERVAL_MS)
        );
    });
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
    const uhd::device_addr_t &mb_args,
    const std::string& addr
) {
    mpmd_mboard_impl::uptr mb =
        mpmd_mboard_impl::uptr(new mpmd_mboard_impl(mb_args, addr));
    // implicit move
    return mb;
}

