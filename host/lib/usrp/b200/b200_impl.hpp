//
// Copyright 2012-2013 Ettus Research LLC
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

#ifndef INCLUDED_B200_IMPL_HPP
#define INCLUDED_B200_IMPL_HPP

#include "b200_iface.hpp"
#include "b200_uart.hpp"
#include "ad9361_ctrl.hpp"
#include "adf4001_ctrl.hpp"
#include "rx_vita_core_3000.hpp"
#include "tx_vita_core_3000.hpp"
#include "time_core_3000.hpp"
#include "gpio_core_200.hpp"
#include "radio_ctrl_core_3000.hpp"
#include "rx_dsp_core_3000.hpp"
#include "tx_dsp_core_3000.hpp"
#include <uhd/device.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/utils/pimpl.hpp>
#include <uhd/utils/tasks.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/types/clock_config.hpp>
#include <uhd/types/stream_cmd.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>
#include <uhd/usrp/subdev_spec.hpp>
#include <uhd/usrp/gps_ctrl.hpp>
#include <uhd/transport/usb_zero_copy.hpp>
#include <uhd/transport/bounded_buffer.hpp>
#include <boost/weak_ptr.hpp>
#include "recv_packet_demuxer_3000.hpp"

static const std::string     B200_FW_FILE_NAME = "usrp_b200_fw.hex";
static const std::string     B200_FPGA_FILE_NAME = "usrp_b200_fpga.bin";
static const std::string     B210_FPGA_FILE_NAME = "usrp_b210_fpga.bin";
static const boost::uint8_t  B200_FW_COMPAT_NUM_MAJOR = 0x03;
static const boost::uint8_t  B200_FW_COMPAT_NUM_MINOR = 0x00;
static const boost::uint16_t B200_FPGA_COMPAT_NUM = 0x02;
static const double          B200_LINK_RATE_BPS = (5e9)/8; //practical link rate (5 Gbps)
static const double          B200_BUS_CLOCK_RATE = 100e6;
static const double          B200_DEFAULT_TICK_RATE = 32e6;
static const boost::uint32_t B200_GPSDO_ST_NONE = 0x83;

#define FLIP_SID(sid) (((sid)<<16)|((sid)>>16))

static const boost::uint32_t B200_CTRL0_MSG_SID = 0x00000010;
static const boost::uint32_t B200_RESP0_MSG_SID = FLIP_SID(B200_CTRL0_MSG_SID);

static const boost::uint32_t B200_CTRL1_MSG_SID = 0x00000020;
static const boost::uint32_t B200_RESP1_MSG_SID = FLIP_SID(B200_CTRL1_MSG_SID);

static const boost::uint32_t B200_TX_DATA0_SID = 0x00000050;
static const boost::uint32_t B200_TX_MSG0_SID = FLIP_SID(B200_TX_DATA0_SID);

static const boost::uint32_t B200_TX_DATA1_SID = 0x00000060;
static const boost::uint32_t B200_TX_MSG1_SID = FLIP_SID(B200_TX_DATA1_SID);

static const boost::uint32_t B200_RX_DATA0_SID = 0x000000A0;
static const boost::uint32_t B200_RX_DATA1_SID = 0x000000B0;

static const boost::uint32_t B200_TX_GPS_UART_SID = 0x00000030;
static const boost::uint32_t B200_RX_GPS_UART_SID = FLIP_SID(B200_TX_GPS_UART_SID);

static const boost::uint32_t B200_LOCAL_CTRL_SID = 0x00000040;
static const boost::uint32_t B200_LOCAL_RESP_SID = FLIP_SID(B200_LOCAL_CTRL_SID);

/***********************************************************************
 * The B200 Capability Constants
 **********************************************************************/

//! Implementation guts
struct b200_impl : public uhd::device
{
    //structors
    b200_impl(const uhd::device_addr_t &);
    ~b200_impl(void);

    //the io interface
    uhd::rx_streamer::sptr get_rx_stream(const uhd::stream_args_t &args);
    uhd::tx_streamer::sptr get_tx_stream(const uhd::stream_args_t &args);
    bool recv_async_msg(uhd::async_metadata_t &, double);

    uhd::property_tree::sptr _tree;

    //controllers
    b200_iface::sptr _iface;
    radio_ctrl_core_3000::sptr _local_ctrl;
    ad9361_ctrl::sptr _codec_ctrl;
    spi_core_3000::sptr _spi_iface;
    boost::shared_ptr<uhd::usrp::adf4001_ctrl> _adf4001_iface;
    uhd::gps_ctrl::sptr _gps;

    //transports
    uhd::transport::zero_copy_if::sptr _data_transport;
    uhd::transport::zero_copy_if::sptr _ctrl_transport;
    boost::shared_ptr<uhd::usrp::recv_packet_demuxer_3000> _demux;

    //device properties interface
    uhd::property_tree::sptr get_tree(void) const
    {
        return _tree;
    }

    boost::weak_ptr<uhd::rx_streamer> _rx_streamer;
    boost::weak_ptr<uhd::tx_streamer> _tx_streamer;

    //async ctrl + msgs
    uhd::task::sptr _async_task;
    typedef uhd::transport::bounded_buffer<uhd::async_metadata_t> async_md_type;
    struct AsyncTaskData
    {
        boost::shared_ptr<async_md_type> async_md;
        boost::weak_ptr<radio_ctrl_core_3000> local_ctrl;
        boost::weak_ptr<radio_ctrl_core_3000> radio_ctrl[2];
        b200_uart::sptr gpsdo_uart;
    };
    boost::shared_ptr<AsyncTaskData> _async_task_data;
    void handle_async_task(uhd::transport::zero_copy_if::sptr, boost::shared_ptr<AsyncTaskData>);

    void register_loopback_self_test(uhd::wb_iface::sptr iface);
    void codec_loopback_self_test(uhd::wb_iface::sptr iface);
    void set_mb_eeprom(const uhd::usrp::mboard_eeprom_t &);
    void check_fw_compat(void);
    void check_fpga_compat(void);
    void update_rx_subdev_spec(const uhd::usrp::subdev_spec_t &);
    void update_tx_subdev_spec(const uhd::usrp::subdev_spec_t &);
    void update_time_source(const std::string &);
    void update_clock_source(const std::string &);
    void update_bandsel(const std::string& which, double freq);
    void update_antenna_sel(const size_t which, const std::string &ant);
    uhd::sensor_value_t get_ref_locked(void);

    //perifs in the radio core
    struct radio_perifs_t
    {
        radio_ctrl_core_3000::sptr ctrl;
        gpio_core_200_32wo::sptr atr;
        time_core_3000::sptr time64;
        rx_vita_core_3000::sptr framer;
        rx_dsp_core_3000::sptr ddc;
        tx_vita_core_3000::sptr deframer;
        tx_dsp_core_3000::sptr duc;
        boost::weak_ptr<uhd::rx_streamer> rx_streamer;
        boost::weak_ptr<uhd::tx_streamer> tx_streamer;
        bool ant_rx2;
    };
    std::vector<radio_perifs_t> _radio_perifs;
    void setup_radio(const size_t which_radio);
    void handle_overflow(const size_t index);

    struct gpio_state {
        boost::uint32_t  tx_bandsel_a, tx_bandsel_b, rx_bandsel_a, rx_bandsel_b, rx_bandsel_c, codec_arst, mimo, ref_sel;

        gpio_state() {
            tx_bandsel_a = 0;
            tx_bandsel_b = 0;
            rx_bandsel_a = 0;
            rx_bandsel_b = 0;
            rx_bandsel_c = 0;
            codec_arst = 0;
            mimo = 0;
            ref_sel = 0;
        }
    } _gpio_state;

    void update_gpio_state(void);
    void reset_codec_dcm(void);

    void update_enables(void);
    void update_atrs(void);

    void update_tick_rate(const double);
    void update_rx_samp_rate(const size_t, const double);
    void update_tx_samp_rate(const size_t, const double);

    double _tick_rate;
    double get_tick_rate(void){return _tick_rate;}
    double set_tick_rate(const double rate);
};

#endif /* INCLUDED_B200_IMPL_HPP */
