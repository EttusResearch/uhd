//
// Copyright 2013-2015 Ettus Research LLC
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

#ifndef INCLUDED_E300_IMPL_HPP
#define INCLUDED_E300_IMPL_HPP

#include <uhd/device.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/usrp/subdev_spec.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>
#include <uhd/usrp/dboard_eeprom.hpp>
#include <uhd/transport/bounded_buffer.hpp>
#include <uhd/types/serial.hpp>
#include <uhd/types/sensors.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <string>
#include "e300_fifo_config.hpp"
#include "radio_ctrl_core_3000.hpp"
#include "rx_frontend_core_200.hpp"
#include "tx_frontend_core_200.hpp"
#include "rx_vita_core_3000.hpp"
#include "tx_vita_core_3000.hpp"
#include "time_core_3000.hpp"
#include "rx_dsp_core_3000.hpp"
#include "tx_dsp_core_3000.hpp"
#include "ad9361_ctrl.hpp"
#include "ad936x_manager.hpp"
#include "gpio_core_200.hpp"

#include "e300_global_regs.hpp"
#include "e300_i2c.hpp"
#include "e300_eeprom_manager.hpp"
#include "e300_sensor_manager.hpp"

/* if we don't compile with gpsd support, don't bother */
#ifdef E300_GPSD
#include "gpsd_iface.hpp"
#endif

namespace uhd { namespace usrp { namespace e300 {

static const std::string E300_FPGA_FILE_NAME = "usrp_e300_fpga.bit";
static const std::string E310_SG1_FPGA_FILE_NAME = "usrp_e310_fpga.bit";
static const std::string E310_SG3_FPGA_FILE_NAME = "usrp_e310_fpga_sg3.bit";

static const std::string E3XX_SG1_FPGA_IDLE_FILE_NAME = "usrp_e3xx_fpga_idle.bit";
static const std::string E3XX_SG3_FPGA_IDLE_FILE_NAME = "usrp_e3xx_fpga_idle_sg3.bit";

static const std::string E300_TEMP_SYSFS = "iio:device0";
static const std::string E300_SPIDEV_DEVICE  = "/dev/spidev0.1";
static const std::string E300_I2CDEV_DEVICE  = "/dev/i2c-0";

static std::string E300_SERVER_RX_PORT0    = "21756";
static std::string E300_SERVER_TX_PORT0    = "21757";
static std::string E300_SERVER_CTRL_PORT0  = "21758";

static std::string E300_SERVER_RX_PORT1    = "21856";
static std::string E300_SERVER_TX_PORT1    = "21857";
static std::string E300_SERVER_CTRL_PORT1  = "21858";


static std::string E300_SERVER_CODEC_PORT  = "21759";
static std::string E300_SERVER_GREGS_PORT  = "21760";
static std::string E300_SERVER_I2C_PORT    = "21761";
static std::string E300_SERVER_SENSOR_PORT = "21762";

static const double E300_RX_SW_BUFF_FULLNESS = 0.9;        //Buffer should be half full
static const size_t E300_RX_FC_REQUEST_FREQ = 32; // per flow ctrl window
static const size_t E300_TX_FC_RESPONSE_FREQ = 8; // per flow ctrl window

// crossbar settings
static const boost::uint8_t E300_RADIO_DEST_PREFIX_TX   = 0;
static const boost::uint8_t E300_RADIO_DEST_PREFIX_CTRL = 1;
static const boost::uint8_t E300_RADIO_DEST_PREFIX_RX   = 2;

static const boost::uint8_t E300_XB_DST_AXI = 0;
static const boost::uint8_t E300_XB_DST_R0  = 1;
static const boost::uint8_t E300_XB_DST_R1  = 2;
static const boost::uint8_t E300_XB_DST_CE0 = 3;
static const boost::uint8_t E300_XB_DST_CE1 = 4;

static const boost::uint8_t E300_DEVICE_THERE = 2;
static const boost::uint8_t E300_DEVICE_HERE  = 0;

static const size_t E300_R0_CTRL_STREAM    = (0 << 2) | E300_RADIO_DEST_PREFIX_CTRL;
static const size_t E300_R0_TX_DATA_STREAM = (0 << 2) | E300_RADIO_DEST_PREFIX_TX;
static const size_t E300_R0_RX_DATA_STREAM = (0 << 2) | E300_RADIO_DEST_PREFIX_RX;

static const size_t E300_R1_CTRL_STREAM    = (1 << 2) | E300_RADIO_DEST_PREFIX_CTRL;
static const size_t E300_R1_TX_DATA_STREAM = (1 << 2) | E300_RADIO_DEST_PREFIX_TX;
static const size_t E300_R1_RX_DATA_STREAM = (1 << 2) | E300_RADIO_DEST_PREFIX_RX;

uhd::device_addrs_t e300_find(const uhd::device_addr_t &multi_dev_hint);
void get_e3x0_fpga_images(const uhd::device_addr_t &device_args,
                          std::string &fpga_image,
                          std::string &idle_image);

/*!
 * USRP-E300 implementation guts:
 * The implementation details are encapsulated here.
 * Handles properties on the mboard, dboard, dsps...
 */
class e300_impl : public uhd::device
{
public:
    //structors
    e300_impl(const uhd::device_addr_t &);
    virtual ~e300_impl(void);

    //the io interface
    boost::mutex _stream_spawn_mutex;
    uhd::rx_streamer::sptr get_rx_stream(const uhd::stream_args_t &);
    uhd::tx_streamer::sptr get_tx_stream(const uhd::stream_args_t &);

    typedef uhd::transport::bounded_buffer<uhd::async_metadata_t> async_md_type;
    boost::shared_ptr<async_md_type> _async_md;

    bool recv_async_msg(uhd::async_metadata_t &, double);

private: // types
    // sid convenience struct
    struct sid_config_t
    {
        boost::uint8_t router_addr_there;
        boost::uint8_t dst_prefix; //2bits
        boost::uint8_t router_dst_there;
        boost::uint8_t router_dst_here;
    };

    // perifs in the radio core
    struct radio_perifs_t
    {
        radio_ctrl_core_3000::sptr ctrl;
        gpio_core_200_32wo::sptr atr;
        time_core_3000::sptr time64;
        rx_vita_core_3000::sptr framer;
        rx_dsp_core_3000::sptr ddc;
        tx_vita_core_3000::sptr deframer;
        tx_dsp_core_3000::sptr duc;
        rx_frontend_core_200::sptr rx_fe;
        tx_frontend_core_200::sptr tx_fe;

        boost::weak_ptr<uhd::rx_streamer> rx_streamer;
        boost::weak_ptr<uhd::tx_streamer> tx_streamer;

        bool ant_rx2;
    };

    //frontend cache so we can update gpios
    struct fe_control_settings_t
    {
        fe_control_settings_t(void)
        {
            rx_freq = 1e9;
            tx_freq = 1e9;
        }
        double rx_freq;
        double tx_freq;
    };

    // convenience struct
    struct both_xports_t
    {
        uhd::transport::zero_copy_if::sptr recv;
        uhd::transport::zero_copy_if::sptr send;
    };

    enum xport_t {AXI, ETH};

    enum compat_t {FPGA_MAJOR, FPGA_MINOR};

    struct gpio_t
    {
        gpio_t() : pps_sel(global_regs::PPS_INT),
            mimo(0), codec_arst(0), tx_bandsels(0),
            rx_bandsel_a(0), rx_bandsel_b(0), rx_bandsel_c(0)
        {}

        boost::uint32_t pps_sel;
        boost::uint32_t mimo;
        boost::uint32_t codec_arst;

        boost::uint32_t tx_bandsels;
        boost::uint32_t rx_bandsel_a;
        boost::uint32_t rx_bandsel_b;
        boost::uint32_t rx_bandsel_c;

        boost::uint32_t time_sync;

        static const size_t PPS_SEL     = 0;
        static const size_t MIMO        = 2;
        static const size_t CODEC_ARST  = 3;
        static const size_t TX_BANDSEL  = 4;
        static const size_t RX_BANDSELA = 7;
        static const size_t RX_BANDSELB = 13;
        static const size_t RX_BANDSELC = 17;
        static const size_t TIME_SYNC   = 21;
    };

private: // methods
    void _register_loopback_self_test(uhd::wb_iface::sptr iface);

    boost::uint32_t _get_version(compat_t which);
    std::string _get_version_hash(void);

    void _setup_radio(const size_t which_radio);

    boost::uint32_t _allocate_sid(const sid_config_t &config);

    void _setup_dest_mapping(
        const boost::uint32_t sid,
        const size_t which_stream);

    size_t _get_axi_dma_channel(
        boost::uint8_t destination,
        boost::uint8_t prefix);

    boost::uint16_t _get_udp_port(
        boost::uint8_t destination,
        boost::uint8_t prefix);

    both_xports_t _make_transport(
        const boost::uint8_t &destination,
        const boost::uint8_t &prefix,
        const uhd::transport::zero_copy_xport_params &params,
        boost::uint32_t &sid);

    double _get_tick_rate(void){return _tick_rate;}
    double _set_tick_rate(const double rate);

    void _update_gpio_state(void);
    void _update_enables(void);
    void _reset_codec_mmcm(void);
    void _update_bandsel(const std::string& which, double freq);

    void _check_tick_rate_with_current_streamers(const double rate);
    void _enforce_tick_rate_limits(
        const size_t change,
        const double tick_rate,
        const std::string &direction);

    void _update_tick_rate(const double);
    void _update_rx_samp_rate(const size_t, const double);
    void _update_tx_samp_rate(const size_t, const double);

    void _update_time_source(const std::string &source);
    void _update_clock_source(const std::string &);
    void _set_time(const uhd::time_spec_t&);
    void _sync_times(void);

    void _update_subdev_spec(
        const std::string &txrx,
        const uhd::usrp::subdev_spec_t &spec);

    void _codec_loopback_self_test(uhd::wb_iface::sptr iface);

    void _update_atrs(void);
    void _update_antenna_sel(const size_t &fe, const std::string &ant);
    void _update_fe_lo_freq(const std::string &fe, const double freq);

    // overflow handling is special for MIMO case
    void _handle_overflow(
        radio_perifs_t &perif,
        boost::weak_ptr<uhd::rx_streamer> streamer);


    // get frontend lock sensor
    uhd::sensor_value_t _get_fe_pll_lock(const bool is_tx);

    // internal gpios
    boost::uint8_t _get_internal_gpio(gpio_core_200::sptr);

    void _set_internal_gpio(
        gpio_core_200::sptr gpio,
        const gpio_attr_t attr,
        const boost::uint32_t value);

private: // members
    uhd::device_addr_t                     _device_addr;
    xport_t                                _xport_path;
    e300_fifo_interface::sptr              _fifo_iface;
    size_t                                 _sid_framer;
    radio_perifs_t                         _radio_perifs[2];
    double                                 _tick_rate;
    ad9361_ctrl::sptr                      _codec_ctrl;
    ad936x_manager::sptr                   _codec_mgr;
    fe_control_settings_t                  _settings;
    global_regs::sptr                      _global_regs;
    e300_sensor_manager::sptr              _sensor_manager;
    e300_eeprom_manager::sptr              _eeprom_manager;
    uhd::transport::zero_copy_xport_params _data_xport_params;
    uhd::transport::zero_copy_xport_params _ctrl_xport_params;
    std::string                            _idle_image;
    bool                                   _do_not_reload;
    gpio_t                                 _misc;
#ifdef E300_GPSD
    gpsd_iface::sptr                       _gps;
    static const size_t                    _GPS_TIMEOUT = 5;
#endif
};

}}} // namespace

#endif /* INCLUDED_E300_IMPL_HPP */
