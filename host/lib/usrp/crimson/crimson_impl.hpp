//
// Copyright 2014 Per Vices Corporation
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

#ifndef INCLUDED_CRIMSON_IMPL_HPP
#define INCLUDED_CRIMSON_IMPL_HPP

#include <uhd/property_tree.hpp>
#include <uhd/device.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include <uhd/usrp/dboard_eeprom.hpp>
#include <uhd/usrp/subdev_spec.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/transport/udp_simple.hpp> //mtu
#include <uhd/utils/tasks.hpp>
#include <boost/weak_ptr.hpp>
#include <uhd/usrp/gps_ctrl.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>
#include <uhd/transport/bounded_buffer.hpp>
#include <uhd/transport/nirio/niusrprio_session.h>
#include <uhd/transport/vrt_if_packet.hpp>
#include <uhd/types/wb_iface.hpp>
#include <boost/asio.hpp>
#include <uhd/stream.hpp>
#include <uhd/types/metadata.hpp>
#include "crimson_fw_common.h"
#include "crimson_iface.hpp"

static const double CRIMSON_DEFAULT_TICK_RATE          = 200e6;        //Hz
static const double CRIMSON_BUS_CLOCK_RATE             = 166.666667e6; //Hz

static const size_t CRIMSON_10GE_DATA_FRAME_MAX_SIZE   = 8000;     //bytes
static const size_t CRIMSON_1GE_DATA_FRAME_MAX_SIZE    = 1472;     //bytes
static const size_t CRIMSON_ETH_MSG_FRAME_SIZE         = uhd::transport::udp_simple::mtu;  //bytes

static const size_t CRIMSON_ETH_MSG_NUM_FRAMES         = 32;
static const size_t CRIMSON_ETH_DATA_NUM_FRAMES        = 32;
static const double CRIMSON_DEFAULT_SYSREF_RATE        = 10e6;

static const size_t CRIMSON_MAX_RATE_10GIGE            = 800000000; // bytes/s
static const size_t CRIMSON_MAX_RATE_1GIGE             = 100000000; // bytes/s

uhd::wb_iface::sptr crimson_make_ctrl_iface_enet(uhd::transport::udp_simple::sptr udp);

/*******************************************************************
 * Crimson Device Implementation Class
 ******************************************************************/
class crimson_impl : public uhd::device
{
public:
    // shared pointer to the Crimson device
    typedef boost::shared_ptr<crimson_impl> sptr;

    // This is the core constructor to be called when a crimson device is found
    crimson_impl(const uhd::device_addr_t &);
    ~crimson_impl(void);

    // pointers to the streams for the device
    virtual uhd::rx_streamer::sptr get_rx_stream(const uhd::stream_args_t &args);
    virtual uhd::tx_streamer::sptr get_tx_stream(const uhd::stream_args_t &args);

    // UHD legacy support
    virtual bool recv_async_msg(uhd::async_metadata_t &async_metadata, double timeout = 0.1);

private:
    // helper functions to wrap send and recv as get and set
    std::string get_string(std::string req);
    void set_string(const std::string pre, std::string data);

    // wrapper for type <double> through the ASCII Crimson interface
    double get_double(std::string req);
    void set_double(const std::string pre, double data);

    // wrapper for type <bool> through the ASCII Crimson interface
    bool get_bool(std::string req);
    void set_bool(const std::string pre, bool data);

    // wrapper for type <int> through the ASCII Crimson interface
    int get_int(std::string req);
    void set_int(const std::string pre, int data);

    // wrapper for type <mboard_eeprom_t> through the ASCII Crimson interface
    uhd::usrp::mboard_eeprom_t get_mboard_eeprom(std::string req);
    void set_mboard_eeprom(const std::string pre, uhd::usrp::mboard_eeprom_t data);

    // wrapper for type <dboard_eeprom_t> through the ASCII Crimson interface
    uhd::usrp::dboard_eeprom_t get_dboard_eeprom(std::string req);
    void set_dboard_eeprom(const std::string pre, uhd::usrp::dboard_eeprom_t data);

    // wrapper for type <sensor_value_t> through the ASCII Crimson interface
    uhd::sensor_value_t get_sensor_value(std::string req);
    void set_sensor_value(const std::string pre, uhd::sensor_value_t data);

    // wrapper for type <meta_range_t> through the ASCII Crimson interface
    uhd::meta_range_t get_meta_range(std::string req);
    void set_meta_range(const std::string pre, uhd::meta_range_t data);

    // wrapper for type <complex<double>> through the ASCII Crimson interface
    std::complex<double>  get_complex_double(std::string req);
    void set_complex_double(const std::string pre, std::complex<double> data);

    // wrapper for type <stream_cmd_t> through the ASCII Crimson interface
    uhd::stream_cmd_t get_stream_cmd(std::string req);
    void set_stream_cmd(const std::string pre, uhd::stream_cmd_t data);

    // wrapper for type <time_spec_t> through the ASCII Crimson interface
    uhd::time_spec_t get_time_spec(std::string req);
    void set_time_spec(const std::string pre, uhd::time_spec_t data);

    // private pointer to the UDP interface, this is the path to send commands to Crimson
    uhd::wb_iface::sptr _iface;
};

#endif /* INCLUDED_CRIMSON_IMPL_HPP */
