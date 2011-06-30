//
// Copyright 2011 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef INCLUDED_B100_IMPL_HPP
#define INCLUDED_B100_IMPL_HPP

#include "../fx2/fx2_ctrl.hpp"
#include "b100_ctrl.hpp"
#include "clock_ctrl.hpp"
#include "codec_ctrl.hpp"
#include "spi_core_100.hpp"
#include "i2c_core_100.hpp"
#include "rx_frontend_core_200.hpp"
#include "tx_frontend_core_200.hpp"
#include "rx_dsp_core_200.hpp"
#include "tx_dsp_core_200.hpp"
#include "time64_core_200.hpp"
#include <uhd/device.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/utils/pimpl.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/types/otw_type.hpp>
#include <uhd/types/clock_config.hpp>
#include <uhd/types/stream_cmd.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>
#include <uhd/usrp/subdev_spec.hpp>
#include <uhd/usrp/dboard_eeprom.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include <uhd/transport/usb_zero_copy.hpp>

static const std::string     B100_FW_FILE_NAME = "usrp_b100_fw.ihx";
static const std::string     B100_FPGA_FILE_NAME = "usrp_b100_fpga.bin";
static const boost::uint16_t B100_FW_COMPAT_NUM = 0x02;
static const boost::uint16_t B100_FPGA_COMPAT_NUM = 0x05;
static const boost::uint32_t B100_RX_SID_BASE = 2;
static const boost::uint32_t B100_TX_ASYNC_SID = 1;
static const double          B100_DEFAULT_TICK_RATE = 64e6;

//! Make a b100 dboard interface
uhd::usrp::dboard_iface::sptr make_b100_dboard_iface(
    wb_iface::sptr wb_iface,
    uhd::i2c_iface::sptr i2c_iface,
    uhd::spi_iface::sptr spi_iface,
    b100_clock_ctrl::sptr clock,
    b100_codec_ctrl::sptr codec
);

//! Implementation guts
class b100_impl : public uhd::device {
public:
    //structors
    b100_impl(const uhd::device_addr_t &);
    ~b100_impl(void);

    //the io interface
    size_t send(const send_buffs_type &,
                size_t,
                const uhd::tx_metadata_t &,
                const uhd::io_type_t &,
                send_mode_t, double);
    size_t recv(const recv_buffs_type &,
                size_t, uhd::rx_metadata_t &,
                const uhd::io_type_t &,
                recv_mode_t, double);
    size_t get_max_send_samps_per_packet(void) const;
    size_t get_max_recv_samps_per_packet(void) const;
    bool recv_async_msg(uhd::async_metadata_t &, double);

private:
    uhd::property_tree::sptr _tree;

    //controllers
    spi_core_100::sptr _fpga_spi_ctrl;
    i2c_core_100::sptr _fpga_i2c_ctrl;
    rx_frontend_core_200::sptr _rx_fe;
    tx_frontend_core_200::sptr _tx_fe;
    std::vector<rx_dsp_core_200::sptr> _rx_dsps;
    tx_dsp_core_200::sptr _tx_dsp;
    time64_core_200::sptr _time64;
    b100_clock_ctrl::sptr _clock_ctrl;
    b100_codec_ctrl::sptr _codec_ctrl;
    b100_ctrl::sptr _fpga_ctrl;
    uhd::usrp::fx2_ctrl::sptr _fx2_ctrl;

    //transports
    uhd::transport::zero_copy_if::sptr _data_transport, _ctrl_transport;

    //dboard stuff
    uhd::usrp::dboard_manager::sptr _dboard_manager;
    uhd::usrp::dboard_iface::sptr _dboard_iface;

    //handle io stuff
    uhd::otw_type_t _rx_otw_type, _tx_otw_type;
    UHD_PIMPL_DECL(io_impl) _io_impl;
    void io_init(void);

    //device properties interface
    void get(const wax::obj &, wax::obj &val){
        val = _tree; //entry point into property tree
    }

    void check_fw_compat(void);
    void check_fpga_compat(void);
    double update_rx_codec_gain(const double); //sets A and B at once
    void set_mb_eeprom(const uhd::usrp::mboard_eeprom_t &);
    void set_db_eeprom(const std::string &, const uhd::usrp::dboard_eeprom_t &);
    void update_tick_rate(const double rate);
    void update_rx_samp_rate(const double rate);
    void update_tx_samp_rate(const double rate);
    void update_rx_subdev_spec(const uhd::usrp::subdev_spec_t &);
    void update_tx_subdev_spec(const uhd::usrp::subdev_spec_t &);
    void update_ref_source(const std::string &);
    void prepare_gpif(void);
};

#endif /* INCLUDED_b100_IMPL_HPP */
