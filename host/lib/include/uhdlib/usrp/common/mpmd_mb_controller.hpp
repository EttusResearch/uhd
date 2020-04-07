//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/rfnoc/mb_controller.hpp>
#include <uhdlib/utils/rpc.hpp>
#include <memory>

namespace uhd { namespace rfnoc {

/*! MPM-Specific version of the mb_controller
 *
 * Reminder: There is one of these per motherboard.
 *
 * This motherboard controller abstracts out a bunch of RPC calls.
 */
class mpmd_mb_controller : public mb_controller
{
public:
    using sptr = std::shared_ptr<mpmd_mb_controller>;

    mpmd_mb_controller(uhd::rpc_client::sptr rpcc, uhd::device_addr_t device_info);

    //! Return reference to the RPC client
    uhd::rpc_client::sptr get_rpc_client()
    {
        return _rpc;
    }

    /**************************************************************************
     * Timekeeper API
     *************************************************************************/
    //! MPM-specific version of the timekeeper controls
    //
    // MPM devices talk to MPM via RPC to control the timekeeper
    class mpmd_timekeeper : public mb_controller::timekeeper
    {
    public:
        using sptr = std::shared_ptr<mpmd_timekeeper>;

        mpmd_timekeeper(const size_t tk_idx, uhd::rpc_client::sptr rpc_client)
            : _tk_idx(tk_idx), _rpc(rpc_client)
        {
            // nop
        }

        uint64_t get_ticks_now();
        uint64_t get_ticks_last_pps();
        void set_ticks_now(const uint64_t ticks);
        void set_ticks_next_pps(const uint64_t ticks);
        void set_period(const uint64_t period_ns);

        /*! Update the tick rate
         *  Note: This is separate from set_tick_rate because the latter is
         *  protected, and we need to implement mpmd-specific functionality here
         */
        void update_tick_rate(const double tick_rate);

    private:
        const size_t _tk_idx;
        uhd::rpc_client::sptr _rpc;
    };

    /**************************************************************************
     * Motherboard Control API (see mb_controller.hpp)
     *************************************************************************/
    std::string get_mboard_name() const;
    void set_time_source(const std::string& source);
    std::string get_time_source() const;
    std::vector<std::string> get_time_sources() const;
    void set_clock_source(const std::string& source);
    std::string get_clock_source() const;
    std::vector<std::string> get_clock_sources() const;
    void set_sync_source(const std::string& clock_source, const std::string& time_source);
    void set_sync_source(const uhd::device_addr_t& sync_source);
    uhd::device_addr_t get_sync_source() const;
    std::vector<uhd::device_addr_t> get_sync_sources();
    void set_clock_source_out(const bool enb);
    void set_time_source_out(const bool enb);
    uhd::sensor_value_t get_sensor(const std::string& name);
    std::vector<std::string> get_sensor_names();
    uhd::usrp::mboard_eeprom_t get_eeprom();
    std::vector<std::string> get_gpio_banks() const;
    std::vector<std::string> get_gpio_srcs(const std::string& bank) const;
    std::vector<std::string> get_gpio_src(const std::string& bank);
    void set_gpio_src(const std::string& bank, const std::vector<std::string>& src);

private:
    /**************************************************************************
     * Attributes
     *************************************************************************/
    //! Reference to RPC interface
    mutable uhd::rpc_client::sptr _rpc;

    uhd::device_addr_t _device_info;

    //! List of MB sensor names
    std::unordered_set<std::string> _sensor_names;

    //! Cache of available GPIO sources
    std::vector<std::string> _gpio_banks;
    std::unordered_map<std::string, std::vector<std::string>> _gpio_srcs;
};

}} // namespace uhd::rfnoc
