//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0
//

#include "mpmd_impl.hpp"
#include <uhd/utils/safe_call.hpp>
#include <uhd/utils/log.hpp>
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
    const size_t MPMD_DEFAULT_RPC_TIMEOUT     = 2000;
    //! Default session ID (MPM will recognize a session by this name)
    const std::string MPMD_DEFAULT_SESSION_ID = "UHD";
    //! Key to initialize a ping/measurement latency test
    const std::string MPMD_MEAS_LATENCY_KEY = "measure_rpc_latency";
    //! Duration of a latency measurement test
    constexpr size_t MPMD_MEAS_LATENCY_DURATION = 1000;


    /*************************************************************************
     * Helper functions
     ************************************************************************/
    void init_device(
            uhd::rpc_client::sptr rpc,
            const uhd::device_addr_t mb_args
    ) {
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
        rpc->set_timeout(mb_args.cast<size_t>(
            "init_timeout", MPMD_DEFAULT_INIT_TIMEOUT
        ));
        if (not rpc->request_with_token<bool>("init", mpm_device_args)) {
            throw uhd::runtime_error("Failed to initialize device.");
        }
        rpc->set_timeout(mb_args.cast<size_t>(
            "rpc_timeout", MPMD_DEFAULT_RPC_TIMEOUT
        ));
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

}

using namespace uhd;
using namespace uhd::mpmd;


/*****************************************************************************
 * Structors
 ****************************************************************************/
mpmd_mboard_impl::mpmd_mboard_impl(
        const device_addr_t &mb_args_,
        const std::string& rpc_server_addr
) : mb_args(mb_args_)
  , rpc(uhd::rpc_client::make(
              rpc_server_addr,
              mb_args_.cast<size_t>(
                  mpmd_impl::MPM_RPC_PORT_KEY,
                  mpmd_impl::MPM_RPC_PORT
              ),
              mpmd_impl::MPM_RPC_GET_LAST_ERROR_CMD))
  , _xport_mgr(xport::mpmd_xport_mgr::make(mb_args))
{
    UHD_LOGGER_TRACE("MPMD")
        << "Initializing mboard, connecting to RPC server address: "
        << rpc_server_addr
        << " mboard args: " << mb_args.to_string()
    ;

    _claimer_task = claim_device_and_make_task(rpc, mb_args);
    if (mb_args_.has_key(MPMD_MEAS_LATENCY_KEY)) {
        measure_rpc_latency(rpc, MPMD_MEAS_LATENCY_DURATION);
    }

    // No one else can now claim the device.
    if (mb_args_.has_key("skip_init")) {
        UHD_LOG_DEBUG("MPMD", "Claimed device, but skipped init.");
        return;
    }

    init_device(rpc, mb_args);
    // RFNoC block clocks are now on. Noc-IDs can be read back.

    auto device_info_dict = rpc->request<dev_info>("get_device_info");
    for (const auto &info_pair : device_info_dict) {
        device_info[info_pair.first] = info_pair.second;
    }
    UHD_LOGGER_TRACE("MPMD")
        << "MPM reports device info: " << device_info.to_string();
    auto dboards_info = rpc->request<std::vector<dev_info>>("get_dboard_info");
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

    // Initialize properties
    this->num_xbars = rpc->request<size_t>("get_num_xbars");
    // xbar_local_addrs is not yet valid after this!
    this->xbar_local_addrs.resize(this->num_xbars, 0xFF);
}

mpmd_mboard_impl::~mpmd_mboard_impl()
{
    UHD_SAFE_CALL(
        if (not rpc->request_with_token<bool>("unclaim")) {
            UHD_LOG_WARNING("MPMD", "Failure to ack unclaim!");
        }
    );
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
    rpc->set_timeout(mb_args.cast<size_t>(
            "rpc_timeout", MPMD_DEFAULT_RPC_TIMEOUT
    ));
}


/*****************************************************************************
 * Private methods
 ****************************************************************************/
bool mpmd_mboard_impl::claim()
{
    try {
        return rpc->request_with_token<bool>("reclaim");
    } catch (...) {
        UHD_LOG_WARNING("MPMD", "Reclaim failed. Exiting claimer loop.");
        return false;
    }
}

uhd::task::sptr mpmd_mboard_impl::claim_device_and_make_task(
        uhd::rpc_client::sptr rpc,
        const uhd::device_addr_t mb_args
) {
    auto rpc_token = rpc->request<std::string>("claim",
        mb_args.get("session_id", MPMD_DEFAULT_SESSION_ID)
    );
    if (rpc_token.empty()) {
        throw uhd::value_error("mpmd device claiming failed!");
    }
    UHD_LOG_TRACE("MPMD", "Received claim token " << rpc_token);
    rpc->set_token(rpc_token);
    return uhd::task::make([this] {
        if (not this->claim()) {
            throw uhd::value_error("mpmd device reclaiming loop failed!");
        };
        std::this_thread::sleep_for(
            std::chrono::milliseconds(MPMD_RECLAIM_INTERVAL_MS)
        );
    });
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

