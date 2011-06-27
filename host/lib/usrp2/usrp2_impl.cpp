//
// Copyright 2010-2011 Ettus Research LLC
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

#include "usrp2_impl.hpp"
#include "fw_common.h"
#include <uhd/utils/log.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/exception.hpp>
#include <uhd/transport/if_addrs.hpp>
#include <uhd/transport/udp_zero_copy.hpp>
#include <uhd/usrp/device_props.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/byteswap.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio.hpp> //used for htonl and ntohl
#include <vector>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;
namespace asio = boost::asio;

/***********************************************************************
 * MTU Discovery
 **********************************************************************/
struct mtu_result_t{
    size_t recv_mtu, send_mtu;
};

static mtu_result_t determine_mtu(const std::string &addr, const mtu_result_t &user_mtu){
    udp_simple::sptr udp_sock = udp_simple::make_connected(
        addr, BOOST_STRINGIZE(USRP2_UDP_CTRL_PORT)
    );

    //The FPGA offers 4K buffers, and the user may manually request this.
    //However, multiple simultaneous receives (2DSP slave + 2DSP master),
    //require that buffering to be used internally, and this is a safe setting.
    std::vector<boost::uint8_t> buffer(std::max(user_mtu.recv_mtu, user_mtu.send_mtu));
    usrp2_ctrl_data_t *ctrl_data = reinterpret_cast<usrp2_ctrl_data_t *>(&buffer.front());
    static const double echo_timeout = 0.020; //20 ms

    //test holler - check if its supported in this fw version
    ctrl_data->id = htonl(USRP2_CTRL_ID_HOLLER_AT_ME_BRO);
    ctrl_data->proto_ver = htonl(USRP2_FW_COMPAT_NUM);
    ctrl_data->data.echo_args.len = htonl(sizeof(usrp2_ctrl_data_t));
    udp_sock->send(boost::asio::buffer(buffer, sizeof(usrp2_ctrl_data_t)));
    udp_sock->recv(boost::asio::buffer(buffer), echo_timeout);
    if (ntohl(ctrl_data->id) != USRP2_CTRL_ID_HOLLER_BACK_DUDE)
        throw uhd::not_implemented_error("holler protocol not implemented");

    size_t min_recv_mtu = sizeof(usrp2_ctrl_data_t), max_recv_mtu = user_mtu.recv_mtu;
    size_t min_send_mtu = sizeof(usrp2_ctrl_data_t), max_send_mtu = user_mtu.send_mtu;

    while (min_recv_mtu < max_recv_mtu){

        size_t test_mtu = (max_recv_mtu/2 + min_recv_mtu/2 + 3) & ~3;

        ctrl_data->id = htonl(USRP2_CTRL_ID_HOLLER_AT_ME_BRO);
        ctrl_data->proto_ver = htonl(USRP2_FW_COMPAT_NUM);
        ctrl_data->data.echo_args.len = htonl(test_mtu);
        udp_sock->send(boost::asio::buffer(buffer, sizeof(usrp2_ctrl_data_t)));

        size_t len = udp_sock->recv(boost::asio::buffer(buffer), echo_timeout);

        if (len >= test_mtu) min_recv_mtu = test_mtu;
        else                 max_recv_mtu = test_mtu - 4;

    }

    while (min_send_mtu < max_send_mtu){

        size_t test_mtu = (max_send_mtu/2 + min_send_mtu/2 + 3) & ~3;

        ctrl_data->id = htonl(USRP2_CTRL_ID_HOLLER_AT_ME_BRO);
        ctrl_data->proto_ver = htonl(USRP2_FW_COMPAT_NUM);
        ctrl_data->data.echo_args.len = htonl(sizeof(usrp2_ctrl_data_t));
        udp_sock->send(boost::asio::buffer(buffer, test_mtu));

        size_t len = udp_sock->recv(boost::asio::buffer(buffer), echo_timeout);
        if (len >= sizeof(usrp2_ctrl_data_t)) len = ntohl(ctrl_data->data.echo_args.len);

        if (len >= test_mtu) min_send_mtu = test_mtu;
        else                 max_send_mtu = test_mtu - 4;
    }

    mtu_result_t mtu;
    mtu.recv_mtu = min_recv_mtu;
    mtu.send_mtu = min_send_mtu;
    return mtu;
}

/***********************************************************************
 * Structors
 **********************************************************************/
usrp2_impl::usrp2_impl(const device_addr_t &_device_addr){
    UHD_MSG(status) << "Opening a USRP2/N-Series device..." << std::endl;
    device_addr_t device_addr = _device_addr;

    //setup the dsp transport hints (default to a large recv buff)
    if (not device_addr.has_key("recv_buff_size")){
        #if defined(UHD_PLATFORM_MACOS) || defined(UHD_PLATFORM_BSD)
            //limit buffer resize on macos or it will error
            device_addr["recv_buff_size"] = "1e6";
        #elif defined(UHD_PLATFORM_LINUX) || defined(UHD_PLATFORM_WIN32)
            //set to half-a-second of buffering at max rate
            device_addr["recv_buff_size"] = "50e6";
        #endif
    }

    device_addrs_t device_args = separate_device_addr(device_addr);

    //extract the user's requested MTU size or default
    mtu_result_t user_mtu;
    user_mtu.recv_mtu = size_t(device_addr.cast<double>("recv_frame_size", udp_simple::mtu));
    user_mtu.send_mtu = size_t(device_addr.cast<double>("send_frame_size", udp_simple::mtu));

    try{
        //calculate the minimum send and recv mtu of all devices
        mtu_result_t mtu = determine_mtu(device_args[0]["addr"], user_mtu);
        for (size_t i = 1; i < device_args.size(); i++){
            mtu_result_t mtu_i = determine_mtu(device_args[i]["addr"], user_mtu);
            mtu.recv_mtu = std::min(mtu.recv_mtu, mtu_i.recv_mtu);
            mtu.send_mtu = std::min(mtu.send_mtu, mtu_i.send_mtu);
        }

        device_addr["recv_frame_size"] = boost::lexical_cast<std::string>(mtu.recv_mtu);
        device_addr["send_frame_size"] = boost::lexical_cast<std::string>(mtu.send_mtu);

        UHD_MSG(status) << boost::format("Current recv frame size: %d bytes") % mtu.recv_mtu << std::endl;
        UHD_MSG(status) << boost::format("Current send frame size: %d bytes") % mtu.send_mtu << std::endl;
    }
    catch(const uhd::not_implemented_error &){
        //just ignore this error, makes older fw work...
    }

    device_args = separate_device_addr(device_addr); //update args for new frame sizes

    ////////////////////////////////////////////////////////////////////
    // create controller objects and initialize the properties tree
    ////////////////////////////////////////////////////////////////////
    _tree = property_tree::make();

    _mboard_stuff.resize(device_args.size());
    for (size_t mb = 0; mb < _mboard_stuff.size(); mb++){
        property_tree::path_type mb_path = str(boost::format("/mboard%u") % mb);

        //create the iface that controls i2c, spi, uart, and wb
        _mboard_stuff[mb].iface = usrp2_iface::make(udp_simple::make_connected(
            device_args[mb]["addr"], BOOST_STRINGIZE(USRP2_UDP_CTRL_PORT)
        ));

        //setup the mboard eeprom
        //TODO perform eeprom read here (not in iface)
        //TODO move the iface REV checks out of here
        property<usrp::mboard_eeprom_t> mb_eeprom_prop;
        mb_eeprom_prop.set(_mboard_stuff[mb].iface->mb_eeprom);
        mb_eeprom_prop.subscribe(boost::bind(&usrp2_impl::set_mb_eeprom, this, mb, _1));
        _tree->create(mb_path / "eeprom", mb_eeprom_prop);

        //create clock control objects
        _mboard_stuff[mb].clock = usrp2_clock_ctrl::make(_mboard_stuff[mb].iface);
        property<double> tick_rate_prop(_mboard_stuff[mb].clock->get_master_clock_rate());
        _tree->create(mb_path / "tick_rate", tick_rate_prop);

        //create codec control objects
        _mboard_stuff[mb].codec = usrp2_codec_ctrl::make(_mboard_stuff[mb].iface);

        //create frontend control objects
        _mboard_stuff[mb].rx_fe = rx_frontend_core_200::make(
            _mboard_stuff[mb].iface, U2_REG_SR_ADDR(SR_RX_FRONT)
        );
        _mboard_stuff[mb].tx_fe = tx_frontend_core_200::make(
            _mboard_stuff[mb].iface, U2_REG_SR_ADDR(SR_TX_FRONT)
        );

        //create dsp control objects

        //create time control objects

    }

}

void usrp2_impl::set_mb_eeprom(const size_t which_mb, const uhd::usrp::mboard_eeprom_t &mb_eeprom){
    mb_eeprom.commit(*(_mboard_stuff[which_mb].iface), mboard_eeprom_t::MAP_N100);
}
