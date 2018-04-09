//
// Copyright 2010-2012,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_USRP2_IMPL_HPP
#define INCLUDED_USRP2_IMPL_HPP

#include "usrp2_iface.hpp"
#include "usrp2_fifo_ctrl.hpp"
#include "clock_ctrl.hpp"
#include "codec_ctrl.hpp"
#include <uhdlib/usrp/cores/rx_frontend_core_200.hpp>
#include <uhdlib/usrp/cores/tx_frontend_core_200.hpp>
#include <uhdlib/usrp/cores/rx_dsp_core_200.hpp>
#include <uhdlib/usrp/cores/tx_dsp_core_200.hpp>
#include <uhdlib/usrp/cores/time64_core_200.hpp>
#include <uhdlib/usrp/cores/user_settings_core_200.hpp>
#include <uhdlib/usrp/cores/gpio_core_200.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/usrp/gps_ctrl.hpp>
#include <uhd/device.hpp>
#include <uhd/utils/pimpl.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/types/stream_cmd.hpp>
#include <uhd/types/clock_config.hpp>
#include <uhd/usrp/dboard_eeprom.hpp>
#include <uhd/transport/vrt_if_packet.hpp>
#include <uhd/transport/udp_simple.hpp>
#include <uhd/transport/udp_zero_copy.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include <uhd/usrp/subdev_spec.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <atomic>

static const double USRP2_LINK_RATE_BPS = 1000e6/8;
static const double mimo_clock_delay_usrp2_rev4 = 4.18e-9;
static const double mimo_clock_delay_usrp_n2xx = 4.10e-9;
static const size_t mimo_clock_sync_delay_cycles = 138;
static const size_t USRP2_SRAM_BYTES = size_t(1 << 20);
static const uint32_t USRP2_TX_ASYNC_SID = 2;
static const uint32_t USRP2_RX_SID_BASE = 3;

uhd::device_addrs_t usrp2_find(const uhd::device_addr_t &hint_);

//! Make a usrp2 dboard interface.
uhd::usrp::dboard_iface::sptr make_usrp2_dboard_iface(
    uhd::timed_wb_iface::sptr wb_iface,
    uhd::i2c_iface::sptr i2c_iface,
    uhd::spi_iface::sptr spi_iface,
    usrp2_clock_ctrl::sptr clk_ctrl
);

/*!
 * USRP2 implementation guts:
 * The implementation details are encapsulated here.
 * Handles device properties and streaming...
 */
class usrp2_impl : public uhd::device{
public:
    usrp2_impl(const uhd::device_addr_t &);
    ~usrp2_impl(void);

    //the io interface
    uhd::rx_streamer::sptr get_rx_stream(const uhd::stream_args_t &args);
    uhd::tx_streamer::sptr get_tx_stream(const uhd::stream_args_t &args);
    bool recv_async_msg(uhd::async_metadata_t &, double);

    static uhd::usrp::mboard_eeprom_t get_mb_eeprom(usrp2_iface &);

private:
    struct mb_container_type{
        usrp2_iface::sptr iface;
        usrp2_fifo_ctrl::sptr fifo_ctrl;
        uhd::spi_iface::sptr spiface;
        uhd::timed_wb_iface::sptr wbiface;
        usrp2_clock_ctrl::sptr clock;
        usrp2_codec_ctrl::sptr codec;
        uhd::gps_ctrl::sptr gps;
        rx_frontend_core_200::sptr rx_fe;
        tx_frontend_core_200::sptr tx_fe;
        std::vector<rx_dsp_core_200::sptr> rx_dsps;
        std::vector<boost::weak_ptr<uhd::rx_streamer> > rx_streamers;
        std::vector<boost::weak_ptr<uhd::tx_streamer> > tx_streamers;
        tx_dsp_core_200::sptr tx_dsp;
        time64_core_200::sptr time64;
        user_settings_core_200::sptr user;
        std::vector<uhd::transport::zero_copy_if::sptr> rx_dsp_xports;
        uhd::transport::zero_copy_if::sptr tx_dsp_xport;
        uhd::transport::zero_copy_if::sptr fifo_ctrl_xport;
        uhd::usrp::dboard_manager::sptr dboard_manager;
        size_t rx_chan_occ, tx_chan_occ;
        mb_container_type(void): rx_chan_occ(0), tx_chan_occ(0){}
    };
    uhd::dict<std::string, mb_container_type> _mbc;

    void set_mb_eeprom(const std::string &, const uhd::usrp::mboard_eeprom_t &);
    void set_db_eeprom(const std::string &, const std::string &, const uhd::usrp::dboard_eeprom_t &);

    uhd::sensor_value_t get_mimo_locked(const std::string &);
    uhd::sensor_value_t get_ref_locked(const std::string &);

    void set_rx_fe_corrections(const std::string &mb, const double);
    void set_tx_fe_corrections(const std::string &mb, const double);
    bool _ignore_cal_file;

    //io impl methods and members
    uhd::device_addr_t device_addr;
    UHD_PIMPL_DECL(io_impl) _io_impl;
    std::atomic<bool> _pirate_task_exit;
    void io_init(void);
    void update_tick_rate(const double rate);
    void update_rx_samp_rate(const std::string &, const size_t, const double rate);
    void update_tx_samp_rate(const std::string &, const size_t, const double rate);
    void update_rates(void);
    //update spec methods are coercers until we only accept db_name == A
    void update_rx_subdev_spec(const std::string &, const uhd::usrp::subdev_spec_t &);
    void update_tx_subdev_spec(const std::string &, const uhd::usrp::subdev_spec_t &);
    double set_tx_dsp_freq(const std::string &, const double);
    uhd::meta_range_t get_tx_dsp_freq_range(const std::string &);
    void update_clock_source(const std::string &, const std::string &);
    void program_stream_dest(uhd::transport::zero_copy_if::sptr &, const uhd::stream_args_t &);
};

#endif /* INCLUDED_USRP2_IMPL_HPP */
