//
// Copyright 2011-2012 Ettus Research LLC
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

#include "fx2_ctrl.hpp"
#include "clock_ctrl.hpp"
#include "codec_ctrl.hpp"
#include "i2c_core_200.hpp"
#include "rx_frontend_core_200.hpp"
#include "tx_frontend_core_200.hpp"
#include "rx_dsp_core_200.hpp"
#include "tx_dsp_core_200.hpp"
#include "time64_core_200.hpp"
#include "fifo_ctrl_excelsior.hpp"
#include "user_settings_core_200.hpp"
#include "recv_packet_demuxer_3000.hpp"
#include <uhd/device.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/types/clock_config.hpp>
#include <uhd/types/stream_cmd.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>
#include <uhd/usrp/subdev_spec.hpp>
#include <uhd/usrp/dboard_eeprom.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include <uhd/transport/usb_zero_copy.hpp>
#include <boost/weak_ptr.hpp>

static const double          B100_LINK_RATE_BPS = 256e6/5; //pratical link rate (< 480 Mbps)
static const std::string     B100_FW_FILE_NAME = "usrp_b100_fw.ihx";
static const std::string     B100_FPGA_FILE_NAME = "usrp_b100_fpga.bin";
static const boost::uint16_t B100_FW_COMPAT_NUM = 0x04;
static const boost::uint16_t B100_FPGA_COMPAT_NUM = 11;
static const boost::uint32_t B100_RX_SID_BASE = 30;
static const boost::uint32_t B100_TX_ASYNC_SID = 10;
static const boost::uint32_t B100_CTRL_MSG_SID = 20;
static const double          B100_DEFAULT_TICK_RATE = 64e6;
static const size_t          B100_MAX_PKT_BYTE_LIMIT = 2048;
static const std::string     B100_EEPROM_MAP_KEY = "B100";

#define I2C_ADDR_TX_A       (I2C_DEV_EEPROM | 0x4)
#define I2C_ADDR_RX_A       (I2C_DEV_EEPROM | 0x5)
#define I2C_ADDR_TX_B       (I2C_DEV_EEPROM | 0x6)
#define I2C_ADDR_RX_B       (I2C_DEV_EEPROM | 0x7)
#define I2C_DEV_EEPROM      0x50

#define VRQ_FW_COMPAT       0x83
#define VRQ_ENABLE_GPIF     0x0d
#define VRQ_CLEAR_FPGA_FIFO 0x0e

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
    uhd::rx_streamer::sptr get_rx_stream(const uhd::stream_args_t &args);
    uhd::tx_streamer::sptr get_tx_stream(const uhd::stream_args_t &args);
    bool recv_async_msg(uhd::async_metadata_t &, double);

private:
    uhd::property_tree::sptr _tree;

    //controllers
    fifo_ctrl_excelsior::sptr _fifo_ctrl;
    i2c_core_200::sptr _fpga_i2c_ctrl;
    rx_frontend_core_200::sptr _rx_fe;
    tx_frontend_core_200::sptr _tx_fe;
    std::vector<rx_dsp_core_200::sptr> _rx_dsps;
    tx_dsp_core_200::sptr _tx_dsp;
    time64_core_200::sptr _time64;
    user_settings_core_200::sptr _user;
    b100_clock_ctrl::sptr _clock_ctrl;
    b100_codec_ctrl::sptr _codec_ctrl;
    uhd::usrp::fx2_ctrl::sptr _fx2_ctrl;

    //transports
    uhd::transport::zero_copy_if::sptr _ctrl_transport;
    uhd::transport::zero_copy_if::sptr _data_transport;
    boost::shared_ptr<uhd::usrp::recv_packet_demuxer_3000> _recv_demuxer;

    //dboard stuff
    uhd::usrp::dboard_manager::sptr _dboard_manager;
    uhd::usrp::dboard_iface::sptr _dboard_iface;

    //device properties interface
    uhd::property_tree::sptr get_tree(void) const{
        return _tree;
    }

    std::vector<boost::weak_ptr<uhd::rx_streamer> > _rx_streamers;
    std::vector<boost::weak_ptr<uhd::tx_streamer> > _tx_streamers;

    void check_fw_compat(void);
    void check_fpga_compat(void);
    double update_rx_codec_gain(const double); //sets A and B at once
    void set_mb_eeprom(const uhd::usrp::mboard_eeprom_t &);
    void set_db_eeprom(const std::string &, const uhd::usrp::dboard_eeprom_t &);
    void update_tick_rate(const double rate);
    void update_rx_samp_rate(const size_t, const double rate);
    void update_tx_samp_rate(const size_t, const double rate);
    void update_rates(void);
    void update_rx_subdev_spec(const uhd::usrp::subdev_spec_t &);
    void update_tx_subdev_spec(const uhd::usrp::subdev_spec_t &);
    void update_clock_source(const std::string &);
    void enable_gpif(const bool);
    void clear_fpga_fifo(void);
    uhd::sensor_value_t get_ref_locked(void);
    void set_rx_fe_corrections(const double);
    void set_tx_fe_corrections(const double);
};

#endif /* INCLUDED_b100_IMPL_HPP */
