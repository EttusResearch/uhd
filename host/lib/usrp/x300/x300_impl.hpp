//
// Copyright 2013-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_X300_IMPL_HPP
#define INCLUDED_X300_IMPL_HPP

#include "x300_clock_ctrl.hpp"
#include "x300_conn_mgr.hpp"
#include "x300_defaults.hpp"
#include "x300_device_args.hpp"
#include "x300_fw_common.h"
#include "x300_mboard_type.hpp"
#include "x300_radio_ctrl_impl.hpp"
#include "x300_regs.hpp"
#include <uhd/types/device_addr.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/types/wb_iface.hpp>
#include <uhd/usrp/gps_ctrl.hpp>
#include <uhd/usrp/subdev_spec.hpp>
#include <uhdlib/usrp/cores/i2c_core_100_wb32.hpp>
#include <atomic>
#include <memory>

uhd::device_addrs_t x300_find(const uhd::device_addr_t& hint_);

class x300_impl : public uhd::usrp::device3_impl
{
public:
    x300_impl(const uhd::device_addr_t&);
    void setup_mb(const size_t which, const uhd::device_addr_t&);
    ~x300_impl(void);

protected:
    void subdev_to_blockid(const uhd::usrp::subdev_spec_pair_t& spec,
        const size_t mb_i,
        uhd::rfnoc::block_id_t& block_id,
        uhd::device_addr_t& block_args);
    uhd::usrp::subdev_spec_pair_t blockid_to_subdev(
        const uhd::rfnoc::block_id_t& blockid, const uhd::device_addr_t& block_args);

private:
    // vector of member objects per motherboard
    struct mboard_members_t
    {
        uhd::usrp::x300::x300_device_args_t args;

        bool initialization_done = false;
        uhd::task::sptr claimer_task;
        uhd::usrp::x300::xport_path_t xport_path;
        uhd::device_addr_t send_args;
        uhd::device_addr_t recv_args;

        // perifs in the zpu
        uhd::wb_iface::sptr zpu_ctrl;
        spi_core_3000::sptr zpu_spi;
        i2c_core_100_wb32::sptr zpu_i2c;

        // other perifs on mboard
        x300_clock_ctrl::sptr clock;
        uhd::gps_ctrl::sptr gps;

        uhd::usrp::x300::fw_regmap_t::sptr fw_regmap;

        // which FPGA image is loaded
        std::string loaded_fpga_image;

        size_t hw_rev;
        std::string current_refclk_src;

        std::vector<uhd::rfnoc::x300_radio_ctrl_impl::sptr> radios;

        uhd::usrp::x300::conn_manager::sptr conn_mgr;
    };
    std::vector<mboard_members_t> _mb;

    std::atomic<size_t> _sid_framer;

    uhd::sid_t allocate_sid(mboard_members_t& mb,
        const uhd::sid_t& address,
        const uint32_t src_addr,
        const uint32_t src_dst);
    uhd::both_xports_t make_transport(const uhd::sid_t& address,
        const xport_type_t xport_type,
        const uhd::device_addr_t& args);

    //! get mtu
    size_t get_mtu(const size_t, const uhd::direction_t);

    bool _ignore_cal_file;

    void update_clock_control(mboard_members_t&);
    void initialize_clock_control(mboard_members_t& mb);
    void set_time_source_out(mboard_members_t&, const bool);
    void update_clock_source(mboard_members_t&, const std::string&);
    void update_time_source(mboard_members_t&, const std::string&);
    void sync_times(mboard_members_t&, const uhd::time_spec_t&);

    uhd::sensor_value_t get_ref_locked(mboard_members_t& mb);
    bool wait_for_clk_locked(mboard_members_t& mb, uint32_t which, double timeout);
    bool is_pps_present(mboard_members_t& mb);

    void check_fw_compat(const uhd::fs_path& mb_path, const mboard_members_t& members);
    void check_fpga_compat(const uhd::fs_path& mb_path, const mboard_members_t& members);

    /// More IO stuff
    uhd::device_addr_t get_tx_hints(size_t mb_index);
    uhd::device_addr_t get_rx_hints(size_t mb_index);

    void post_streamer_hooks(uhd::direction_t dir);
};

#endif /* INCLUDED_X300_IMPL_HPP */
