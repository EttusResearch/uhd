//
// Copyright 2010-2012,2014,2017 Ettus Research, A National Instruments Company
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
#include "apply_corrections.hpp"
#include <uhd/utils/log.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/exception.hpp>
#include <uhd/transport/if_addrs.hpp>
#include <uhd/transport/udp_zero_copy.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/safe_call.hpp>
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio.hpp> //used for htonl and ntohl
#include <boost/thread.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;
namespace asio = boost::asio;

//A reasonable number of frames for send/recv and async/sync
static const size_t DEFAULT_NUM_FRAMES = 32;

/***********************************************************************
 * Discovery over the udp transport
 **********************************************************************/
device_addrs_t usrp2_find(const device_addr_t &hint_){
    //handle the multi-device discovery
    device_addrs_t hints = separate_device_addr(hint_);
    if (hints.size() > 1){
        device_addrs_t found_devices;
        std::string error_msg;
        BOOST_FOREACH(const device_addr_t &hint_i, hints){
            device_addrs_t found_devices_i = usrp2_find(hint_i);
            if (found_devices_i.size() != 1) error_msg += str(boost::format(
                "Could not resolve device hint \"%s\" to a single device."
            ) % hint_i.to_string());
            else found_devices.push_back(found_devices_i[0]);
        }
        if (found_devices.empty()) return device_addrs_t();
        if (not error_msg.empty()) throw uhd::value_error(error_msg);
        return device_addrs_t(1, combine_device_addrs(found_devices));
    }

    //initialize the hint for a single device case
    UHD_ASSERT_THROW(hints.size() <= 1);
    hints.resize(1); //in case it was empty
    device_addr_t hint = hints[0];
    device_addrs_t usrp2_addrs;

    //return an empty list of addresses when type is set to non-usrp2
    if (hint.has_key("type") and hint["type"] != "usrp2") return usrp2_addrs;

    //Return an empty list of addresses when a resource is specified,
    //since a resource is intended for a different, non-USB, device.
    if (hint.has_key("resource")) return usrp2_addrs;

    //if no address was specified, send a broadcast on each interface
    if (not hint.has_key("addr")){
        BOOST_FOREACH(const if_addrs_t &if_addrs, get_if_addrs()){
            //avoid the loopback device
            if (if_addrs.inet == asio::ip::address_v4::loopback().to_string()) continue;

            //create a new hint with this broadcast address
            device_addr_t new_hint = hint;
            new_hint["addr"] = if_addrs.bcast;

            //call discover with the new hint and append results
            device_addrs_t new_usrp2_addrs = usrp2_find(new_hint);
            usrp2_addrs.insert(usrp2_addrs.begin(),
                new_usrp2_addrs.begin(), new_usrp2_addrs.end()
            );
        }
        return usrp2_addrs;
    }

    //Create a UDP transport to communicate:
    //Some devices will cause a throw when opened for a broadcast address.
    //We print and recover so the caller can loop through all bcast addrs.
    udp_simple::sptr udp_transport;
    try{
        udp_transport = udp_simple::make_broadcast(hint["addr"], BOOST_STRINGIZE(USRP2_UDP_CTRL_PORT));
    }
    catch(const std::exception &e){
        UHD_MSG(error) << boost::format("Cannot open UDP transport on %s\n%s") % hint["addr"] % e.what() << std::endl;
        return usrp2_addrs; //dont throw, but return empty address so caller can insert
    }

    //send a hello control packet
    usrp2_ctrl_data_t ctrl_data_out = usrp2_ctrl_data_t();
    ctrl_data_out.proto_ver = uhd::htonx<uint32_t>(USRP2_FW_COMPAT_NUM);
    ctrl_data_out.id = uhd::htonx<uint32_t>(USRP2_CTRL_ID_WAZZUP_BRO);
    try
    {
        udp_transport->send(boost::asio::buffer(&ctrl_data_out, sizeof(ctrl_data_out)));
    }
    catch(const std::exception &ex)
    {
        UHD_MSG(error) << "USRP2 Network discovery error " << ex.what() << std::endl;
    }
    catch(...)
    {
        UHD_MSG(error) << "USRP2 Network discovery unknown error " << std::endl;
    }

    //loop and recieve until the timeout
    uint8_t usrp2_ctrl_data_in_mem[udp_simple::mtu]; //allocate max bytes for recv
    const usrp2_ctrl_data_t *ctrl_data_in = reinterpret_cast<const usrp2_ctrl_data_t *>(usrp2_ctrl_data_in_mem);
    while(true){
        size_t len = udp_transport->recv(asio::buffer(usrp2_ctrl_data_in_mem));
        if (len > offsetof(usrp2_ctrl_data_t, data) and ntohl(ctrl_data_in->id) == USRP2_CTRL_ID_WAZZUP_DUDE){

            //make a boost asio ipv4 with the raw addr in host byte order
            device_addr_t new_addr;
            new_addr["type"] = "usrp2";
            //We used to get the address from the control packet.
            //Now now uses the socket itself to yield the address.
            //boost::asio::ip::address_v4 ip_addr(ntohl(ctrl_data_in->data.ip_addr));
            //new_addr["addr"] = ip_addr.to_string();
            new_addr["addr"] = udp_transport->get_recv_addr();

            //Attempt a simple 2-way communication with a connected socket.
            //Reason: Although the USRP will respond the broadcast above,
            //we may not be able to communicate directly (non-broadcast).
            udp_simple::sptr ctrl_xport = udp_simple::make_connected(
                new_addr["addr"], BOOST_STRINGIZE(USRP2_UDP_CTRL_PORT)
            );
            ctrl_xport->send(boost::asio::buffer(&ctrl_data_out, sizeof(ctrl_data_out)));
            size_t len = ctrl_xport->recv(asio::buffer(usrp2_ctrl_data_in_mem));
            if (len > offsetof(usrp2_ctrl_data_t, data) and ntohl(ctrl_data_in->id) == USRP2_CTRL_ID_WAZZUP_DUDE){
                //found the device, open up for communication!
            }
            else{
                //otherwise we don't find it...
                continue;
            }

            //Attempt to read the name from the EEPROM and perform filtering.
            //This operation can throw due to compatibility mismatch.
            try{
                usrp2_iface::sptr iface = usrp2_iface::make(ctrl_xport);
                if (iface->is_device_locked()) continue; //ignore locked devices
                mboard_eeprom_t mb_eeprom = iface->mb_eeprom;
                new_addr["name"] = mb_eeprom["name"];
                new_addr["serial"] = mb_eeprom["serial"];
            }
            catch(const std::exception &){
                //set these values as empty string so the device may still be found
                //and the filter's below can still operate on the discovered device
                new_addr["name"] = "";
                new_addr["serial"] = "";
            }

            //filter the discovered device below by matching optional keys
            if (
                (not hint.has_key("name")   or hint["name"]   == new_addr["name"]) and
                (not hint.has_key("serial") or hint["serial"] == new_addr["serial"])
            ){
                usrp2_addrs.push_back(new_addr);
            }

            //dont break here, it will exit the while loop
            //just continue on to the next loop iteration
        }
        if (len == 0) break; //timeout
    }

    return usrp2_addrs;
}

/***********************************************************************
 * Make
 **********************************************************************/
static device::sptr usrp2_make(const device_addr_t &device_addr){
    return device::sptr(new usrp2_impl(device_addr));
}

UHD_STATIC_BLOCK(register_usrp2_device){
    device::register_device(&usrp2_find, &usrp2_make, device::USRP);
}

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
    std::vector<uint8_t> buffer(std::max(user_mtu.recv_mtu, user_mtu.send_mtu));
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
 * Helpers
 **********************************************************************/
static zero_copy_if::sptr make_xport(
    const std::string &addr,
    const std::string &port,
    const device_addr_t &hints,
    const std::string &filter
){

    //only copy hints that contain the filter word
    device_addr_t filtered_hints;
    BOOST_FOREACH(const std::string &key, hints.keys()){
        if (key.find(filter) == std::string::npos) continue;
        filtered_hints[key] = hints[key];
    }

    zero_copy_xport_params default_buff_args;
    default_buff_args.send_frame_size = transport::udp_simple::mtu;
    default_buff_args.recv_frame_size = transport::udp_simple::mtu;
    default_buff_args.num_send_frames = DEFAULT_NUM_FRAMES;
    default_buff_args.num_recv_frames = DEFAULT_NUM_FRAMES;

    //make the transport object with the filtered hints
    udp_zero_copy::buff_params ignored_params;
    zero_copy_if::sptr xport = udp_zero_copy::make(addr, port, default_buff_args, ignored_params, filtered_hints);

    //Send a small data packet so the usrp2 knows the udp source port.
    //This setup must happen before further initialization occurs
    //or the async update packets will cause ICMP destination unreachable.
    static const uint32_t data[2] = {
        uhd::htonx(uint32_t(0 /* don't care seq num */)),
        uhd::htonx(uint32_t(USRP2_INVALID_VRT_HEADER))
    };
    transport::managed_send_buffer::sptr send_buff = xport->get_send_buff();
    std::memcpy(send_buff->cast<void*>(), &data, sizeof(data));
    send_buff->commit(sizeof(data));

    return xport;
}

/***********************************************************************
 * Structors
 **********************************************************************/
usrp2_impl::usrp2_impl(const device_addr_t &_device_addr) :
    device_addr(_device_addr)
{
    UHD_MSG(status) << "Opening a USRP2/N-Series device..." << std::endl;

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
    if (not device_addr.has_key("send_buff_size")){
        //The buffer should be the size of the SRAM on the device,
        //because we will never commit more than the SRAM can hold.
        device_addr["send_buff_size"] = boost::lexical_cast<std::string>(USRP2_SRAM_BYTES);
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
    _type = device::USRP;
    _ignore_cal_file = device_addr.has_key("ignore-cal-file");
    _tree->create<std::string>("/name").set("USRP2 / N-Series Device");

    for (size_t mbi = 0; mbi < device_args.size(); mbi++){
        const device_addr_t device_args_i = device_args[mbi];
        const std::string mb = boost::lexical_cast<std::string>(mbi);
        const std::string addr = device_args_i["addr"];
        const fs_path mb_path = "/mboards/" + mb;

        ////////////////////////////////////////////////////////////////
        // create the iface that controls i2c, spi, uart, and wb
        ////////////////////////////////////////////////////////////////
        _mbc[mb].iface = usrp2_iface::make(udp_simple::make_connected(
            addr, BOOST_STRINGIZE(USRP2_UDP_CTRL_PORT)
        ));
        _tree->create<std::string>(mb_path / "name").set(_mbc[mb].iface->get_cname());
        _tree->create<std::string>(mb_path / "fw_version").set(_mbc[mb].iface->get_fw_version_string());

        //check the fpga compatibility number
        const uint32_t fpga_compat_num = _mbc[mb].iface->peek32(U2_REG_COMPAT_NUM_RB);
        uint16_t fpga_major = fpga_compat_num >> 16, fpga_minor = fpga_compat_num & 0xffff;
        if (fpga_major == 0){ //old version scheme
            fpga_major = fpga_minor;
            fpga_minor = 0;
        }
        int expected_fpga_compat_num = std::min(USRP2_FPGA_COMPAT_NUM, N200_FPGA_COMPAT_NUM);
        switch (_mbc[mb].iface->get_rev())
        {
        case usrp2_iface::USRP2_REV3:
        case usrp2_iface::USRP2_REV4:
            expected_fpga_compat_num = USRP2_FPGA_COMPAT_NUM;
            break;
        case usrp2_iface::USRP_N200:
        case usrp2_iface::USRP_N200_R4:
        case usrp2_iface::USRP_N210:
        case usrp2_iface::USRP_N210_R4:
            expected_fpga_compat_num = N200_FPGA_COMPAT_NUM;
            break;
        default:
            // handle case where the MB EEPROM is not programmed
            if (fpga_major == USRP2_FPGA_COMPAT_NUM or fpga_major == N200_FPGA_COMPAT_NUM)
            {
                UHD_MSG(warning)  << "Unable to identify device - assuming USRP2/N-Series device" << std::endl;
                expected_fpga_compat_num = fpga_major;
            }
        }
        if (fpga_major != expected_fpga_compat_num){
            throw uhd::runtime_error(str(boost::format(
                "\nPlease update the firmware and FPGA images for your device.\n"
                "See the application notes for USRP2/N-Series for instructions.\n"
                "Expected FPGA compatibility number %d, but got %d:\n"
                "The FPGA build is not compatible with the host code build.\n"
                "%s\n"
            ) % expected_fpga_compat_num % fpga_major % _mbc[mb].iface->images_warn_help_message()));
        }
        _tree->create<std::string>(mb_path / "fpga_version").set(str(boost::format("%u.%u") % fpga_major % fpga_minor));

        //lock the device/motherboard to this process
        _mbc[mb].iface->lock_device(true);

        ////////////////////////////////////////////////////////////////
        // construct transports for RX and TX DSPs
        ////////////////////////////////////////////////////////////////
        UHD_LOG << "Making transport for RX DSP0..." << std::endl;
        _mbc[mb].rx_dsp_xports.push_back(make_xport(
            addr, BOOST_STRINGIZE(USRP2_UDP_RX_DSP0_PORT), device_args_i, "recv"
        ));
        UHD_LOG << "Making transport for RX DSP1..." << std::endl;
        _mbc[mb].rx_dsp_xports.push_back(make_xport(
            addr, BOOST_STRINGIZE(USRP2_UDP_RX_DSP1_PORT), device_args_i, "recv"
        ));
        UHD_LOG << "Making transport for TX DSP0..." << std::endl;
        _mbc[mb].tx_dsp_xport = make_xport(
            addr, BOOST_STRINGIZE(USRP2_UDP_TX_DSP0_PORT), device_args_i, "send"
        );
        UHD_LOG << "Making transport for Control..." << std::endl;
        _mbc[mb].fifo_ctrl_xport = make_xport(
            addr, BOOST_STRINGIZE(USRP2_UDP_FIFO_CRTL_PORT), device_addr_t(), ""
        );
        //set the filter on the router to take dsp data from this port
        _mbc[mb].iface->poke32(U2_REG_ROUTER_CTRL_PORTS, (USRP2_UDP_FIFO_CRTL_PORT << 16) | USRP2_UDP_TX_DSP0_PORT);

        //create the fifo control interface for high speed register access
        _mbc[mb].fifo_ctrl = usrp2_fifo_ctrl::make(_mbc[mb].fifo_ctrl_xport);
        switch(_mbc[mb].iface->get_rev()){
        case usrp2_iface::USRP_N200:
        case usrp2_iface::USRP_N210:
        case usrp2_iface::USRP_N200_R4:
        case usrp2_iface::USRP_N210_R4:
            _mbc[mb].wbiface = _mbc[mb].fifo_ctrl;
            _mbc[mb].spiface = _mbc[mb].fifo_ctrl;
            break;
        default:
            _mbc[mb].wbiface = _mbc[mb].iface;
            _mbc[mb].spiface = _mbc[mb].iface;
            break;
        }
        _tree->create<double>(mb_path / "link_max_rate").set(USRP2_LINK_RATE_BPS);

        ////////////////////////////////////////////////////////////////
        // setup the mboard eeprom
        ////////////////////////////////////////////////////////////////
        _tree->create<mboard_eeprom_t>(mb_path / "eeprom")
            .set(_mbc[mb].iface->mb_eeprom)
            .add_coerced_subscriber(boost::bind(&usrp2_impl::set_mb_eeprom, this, mb, _1));

        ////////////////////////////////////////////////////////////////
        // create clock control objects
        ////////////////////////////////////////////////////////////////
        _mbc[mb].clock = usrp2_clock_ctrl::make(_mbc[mb].iface, _mbc[mb].spiface);
        _tree->create<double>(mb_path / "tick_rate")
            .set_publisher(boost::bind(&usrp2_clock_ctrl::get_master_clock_rate, _mbc[mb].clock))
            .add_coerced_subscriber(boost::bind(&usrp2_impl::update_tick_rate, this, _1));

        ////////////////////////////////////////////////////////////////
        // create codec control objects
        ////////////////////////////////////////////////////////////////
        const fs_path rx_codec_path = mb_path / "rx_codecs/A";
        const fs_path tx_codec_path = mb_path / "tx_codecs/A";
        _tree->create<int>(rx_codec_path / "gains"); //phony property so this dir exists
        _tree->create<int>(tx_codec_path / "gains"); //phony property so this dir exists
        _mbc[mb].codec = usrp2_codec_ctrl::make(_mbc[mb].iface, _mbc[mb].spiface);
        switch(_mbc[mb].iface->get_rev()){
        case usrp2_iface::USRP_N200:
        case usrp2_iface::USRP_N210:
        case usrp2_iface::USRP_N200_R4:
        case usrp2_iface::USRP_N210_R4:{
            _tree->create<std::string>(rx_codec_path / "name").set("ads62p44");
            _tree->create<meta_range_t>(rx_codec_path / "gains/digital/range").set(meta_range_t(0, 6.0, 0.5));
            _tree->create<double>(rx_codec_path / "gains/digital/value")
                .add_coerced_subscriber(boost::bind(&usrp2_codec_ctrl::set_rx_digital_gain, _mbc[mb].codec, _1)).set(0);
            _tree->create<meta_range_t>(rx_codec_path / "gains/fine/range").set(meta_range_t(0, 0.5, 0.05));
            _tree->create<double>(rx_codec_path / "gains/fine/value")
                .add_coerced_subscriber(boost::bind(&usrp2_codec_ctrl::set_rx_digital_fine_gain, _mbc[mb].codec, _1)).set(0);
        }break;

        case usrp2_iface::USRP2_REV3:
        case usrp2_iface::USRP2_REV4:
            _tree->create<std::string>(rx_codec_path / "name").set("ltc2284");
            break;

        case usrp2_iface::USRP_NXXX:
            _tree->create<std::string>(rx_codec_path / "name").set("??????");
            break;
        }
        _tree->create<std::string>(tx_codec_path / "name").set("ad9777");

        ////////////////////////////////////////////////////////////////////
        // Create the GPSDO control
        ////////////////////////////////////////////////////////////////////
        static const uint32_t dont_look_for_gpsdo = 0x1234abcdul;

        //disable check for internal GPSDO when not the following:
        switch(_mbc[mb].iface->get_rev()){
        case usrp2_iface::USRP_N200:
        case usrp2_iface::USRP_N210:
        case usrp2_iface::USRP_N200_R4:
        case usrp2_iface::USRP_N210_R4:
            break;
        default:
            _mbc[mb].iface->pokefw(U2_FW_REG_HAS_GPSDO, dont_look_for_gpsdo);
        }

        //otherwise if not disabled, look for the internal GPSDO
        if (_mbc[mb].iface->peekfw(U2_FW_REG_HAS_GPSDO) != dont_look_for_gpsdo)
        {
            UHD_MSG(status) << "Detecting internal GPSDO.... " << std::flush;
            try{
                _mbc[mb].gps = gps_ctrl::make(udp_simple::make_uart(udp_simple::make_connected(
                    addr, BOOST_STRINGIZE(USRP2_UDP_UART_GPS_PORT)
                )));
            }
            catch(std::exception &e){
                UHD_MSG(error) << "An error occurred making GPSDO control: " << e.what() << std::endl;
            }
            if (_mbc[mb].gps and _mbc[mb].gps->gps_detected())
            {
                BOOST_FOREACH(const std::string &name, _mbc[mb].gps->get_sensors())
                {
                    _tree->create<sensor_value_t>(mb_path / "sensors" / name)
                        .set_publisher(boost::bind(&gps_ctrl::get_sensor, _mbc[mb].gps, name));
                }
            }
            else
            {
                _mbc[mb].iface->pokefw(U2_FW_REG_HAS_GPSDO, dont_look_for_gpsdo);
            }
        }

        ////////////////////////////////////////////////////////////////
        // and do the misc mboard sensors
        ////////////////////////////////////////////////////////////////
        _tree->create<sensor_value_t>(mb_path / "sensors/mimo_locked")
            .set_publisher(boost::bind(&usrp2_impl::get_mimo_locked, this, mb));
        _tree->create<sensor_value_t>(mb_path / "sensors/ref_locked")
            .set_publisher(boost::bind(&usrp2_impl::get_ref_locked, this, mb));

        ////////////////////////////////////////////////////////////////
        // create frontend control objects
        ////////////////////////////////////////////////////////////////
        _mbc[mb].rx_fe = rx_frontend_core_200::make(
            _mbc[mb].wbiface, U2_REG_SR_ADDR(SR_RX_FRONT)
        );
        _mbc[mb].tx_fe = tx_frontend_core_200::make(
            _mbc[mb].wbiface, U2_REG_SR_ADDR(SR_TX_FRONT)
        );

        _tree->create<subdev_spec_t>(mb_path / "rx_subdev_spec")
            .add_coerced_subscriber(boost::bind(&usrp2_impl::update_rx_subdev_spec, this, mb, _1));
        _tree->create<subdev_spec_t>(mb_path / "tx_subdev_spec")
            .add_coerced_subscriber(boost::bind(&usrp2_impl::update_tx_subdev_spec, this, mb, _1));

        const fs_path rx_fe_path = mb_path / "rx_frontends" / "A";
        const fs_path tx_fe_path = mb_path / "tx_frontends" / "A";

        _tree->create<std::complex<double> >(rx_fe_path / "dc_offset" / "value")
            .set_coercer(boost::bind(&rx_frontend_core_200::set_dc_offset, _mbc[mb].rx_fe, _1))
            .set(std::complex<double>(0.0, 0.0));
        _tree->create<bool>(rx_fe_path / "dc_offset" / "enable")
            .add_coerced_subscriber(boost::bind(&rx_frontend_core_200::set_dc_offset_auto, _mbc[mb].rx_fe, _1))
            .set(true);
        _tree->create<std::complex<double> >(rx_fe_path / "iq_balance" / "value")
            .add_coerced_subscriber(boost::bind(&rx_frontend_core_200::set_iq_balance, _mbc[mb].rx_fe, _1))
            .set(std::complex<double>(0.0, 0.0));
        _tree->create<std::complex<double> >(tx_fe_path / "dc_offset" / "value")
            .set_coercer(boost::bind(&tx_frontend_core_200::set_dc_offset, _mbc[mb].tx_fe, _1))
            .set(std::complex<double>(0.0, 0.0));
        _tree->create<std::complex<double> >(tx_fe_path / "iq_balance" / "value")
            .add_coerced_subscriber(boost::bind(&tx_frontend_core_200::set_iq_balance, _mbc[mb].tx_fe, _1))
            .set(std::complex<double>(0.0, 0.0));

        ////////////////////////////////////////////////////////////////
        // create rx dsp control objects
        ////////////////////////////////////////////////////////////////
        _mbc[mb].rx_dsps.push_back(rx_dsp_core_200::make(
            _mbc[mb].wbiface, U2_REG_SR_ADDR(SR_RX_DSP0), U2_REG_SR_ADDR(SR_RX_CTRL0), USRP2_RX_SID_BASE + 0, true
        ));
        _mbc[mb].rx_dsps.push_back(rx_dsp_core_200::make(
            _mbc[mb].wbiface, U2_REG_SR_ADDR(SR_RX_DSP1), U2_REG_SR_ADDR(SR_RX_CTRL1), USRP2_RX_SID_BASE + 1, true
        ));
        for (size_t dspno = 0; dspno < _mbc[mb].rx_dsps.size(); dspno++){
            _mbc[mb].rx_dsps[dspno]->set_link_rate(USRP2_LINK_RATE_BPS);
            _tree->access<double>(mb_path / "tick_rate")
                .add_coerced_subscriber(boost::bind(&rx_dsp_core_200::set_tick_rate, _mbc[mb].rx_dsps[dspno], _1));
            fs_path rx_dsp_path = mb_path / str(boost::format("rx_dsps/%u") % dspno);
            _tree->create<meta_range_t>(rx_dsp_path / "rate/range")
                .set_publisher(boost::bind(&rx_dsp_core_200::get_host_rates, _mbc[mb].rx_dsps[dspno]));
            _tree->create<double>(rx_dsp_path / "rate/value")
                .set(1e6) //some default
                .set_coercer(boost::bind(&rx_dsp_core_200::set_host_rate, _mbc[mb].rx_dsps[dspno], _1))
                .add_coerced_subscriber(boost::bind(&usrp2_impl::update_rx_samp_rate, this, mb, dspno, _1));
            _tree->create<double>(rx_dsp_path / "freq/value")
                .set_coercer(boost::bind(&rx_dsp_core_200::set_freq, _mbc[mb].rx_dsps[dspno], _1));
            _tree->create<meta_range_t>(rx_dsp_path / "freq/range")
                .set_publisher(boost::bind(&rx_dsp_core_200::get_freq_range, _mbc[mb].rx_dsps[dspno]));
            _tree->create<stream_cmd_t>(rx_dsp_path / "stream_cmd")
                .add_coerced_subscriber(boost::bind(&rx_dsp_core_200::issue_stream_command, _mbc[mb].rx_dsps[dspno], _1));
        }

        ////////////////////////////////////////////////////////////////
        // create tx dsp control objects
        ////////////////////////////////////////////////////////////////
        _mbc[mb].tx_dsp = tx_dsp_core_200::make(
            _mbc[mb].wbiface, U2_REG_SR_ADDR(SR_TX_DSP), U2_REG_SR_ADDR(SR_TX_CTRL), USRP2_TX_ASYNC_SID
        );
        _mbc[mb].tx_dsp->set_link_rate(USRP2_LINK_RATE_BPS);
        _tree->access<double>(mb_path / "tick_rate")
            .add_coerced_subscriber(boost::bind(&tx_dsp_core_200::set_tick_rate, _mbc[mb].tx_dsp, _1));
        _tree->create<meta_range_t>(mb_path / "tx_dsps/0/rate/range")
            .set_publisher(boost::bind(&tx_dsp_core_200::get_host_rates, _mbc[mb].tx_dsp));
        _tree->create<double>(mb_path / "tx_dsps/0/rate/value")
            .set(1e6) //some default
            .set_coercer(boost::bind(&tx_dsp_core_200::set_host_rate, _mbc[mb].tx_dsp, _1))
            .add_coerced_subscriber(boost::bind(&usrp2_impl::update_tx_samp_rate, this, mb, 0, _1));
        _tree->create<double>(mb_path / "tx_dsps/0/freq/value")
            .set_coercer(boost::bind(&tx_dsp_core_200::set_freq, _mbc[mb].tx_dsp, _1));
        _tree->create<meta_range_t>(mb_path / "tx_dsps/0/freq/range")
            .set_publisher(boost::bind(&tx_dsp_core_200::get_freq_range, _mbc[mb].tx_dsp));

        //setup dsp flow control
        const double ups_per_sec = device_args_i.cast<double>("ups_per_sec", 20);
        const size_t send_frame_size = _mbc[mb].tx_dsp_xport->get_send_frame_size();
        const double ups_per_fifo = device_args_i.cast<double>("ups_per_fifo", 8.0);
        _mbc[mb].tx_dsp->set_updates(
            (ups_per_sec > 0.0)? size_t(100e6/*approx tick rate*//ups_per_sec) : 0,
            (ups_per_fifo > 0.0)? size_t(USRP2_SRAM_BYTES/ups_per_fifo/send_frame_size) : 0
        );

        ////////////////////////////////////////////////////////////////
        // create time control objects
        ////////////////////////////////////////////////////////////////
        time64_core_200::readback_bases_type time64_rb_bases;
        time64_rb_bases.rb_hi_now = U2_REG_TIME64_HI_RB_IMM;
        time64_rb_bases.rb_lo_now = U2_REG_TIME64_LO_RB_IMM;
        time64_rb_bases.rb_hi_pps = U2_REG_TIME64_HI_RB_PPS;
        time64_rb_bases.rb_lo_pps = U2_REG_TIME64_LO_RB_PPS;
        _mbc[mb].time64 = time64_core_200::make(
            _mbc[mb].wbiface, U2_REG_SR_ADDR(SR_TIME64), time64_rb_bases, mimo_clock_sync_delay_cycles
        );
        _tree->access<double>(mb_path / "tick_rate")
            .add_coerced_subscriber(boost::bind(&time64_core_200::set_tick_rate, _mbc[mb].time64, _1));
        _tree->create<time_spec_t>(mb_path / "time/now")
            .set_publisher(boost::bind(&time64_core_200::get_time_now, _mbc[mb].time64))
            .add_coerced_subscriber(boost::bind(&time64_core_200::set_time_now, _mbc[mb].time64, _1));
        _tree->create<time_spec_t>(mb_path / "time/pps")
            .set_publisher(boost::bind(&time64_core_200::get_time_last_pps, _mbc[mb].time64))
            .add_coerced_subscriber(boost::bind(&time64_core_200::set_time_next_pps, _mbc[mb].time64, _1));
        //setup time source props
        _tree->create<std::string>(mb_path / "time_source/value")
            .add_coerced_subscriber(boost::bind(&time64_core_200::set_time_source, _mbc[mb].time64, _1))
            .set("none");
        _tree->create<std::vector<std::string> >(mb_path / "time_source/options")
            .set_publisher(boost::bind(&time64_core_200::get_time_sources, _mbc[mb].time64));
        //setup reference source props
        _tree->create<std::string>(mb_path / "clock_source/value")
            .add_coerced_subscriber(boost::bind(&usrp2_impl::update_clock_source, this, mb, _1))
            .set("internal");
        std::vector<std::string> clock_sources = boost::assign::list_of("internal")("external")("mimo");
        if (_mbc[mb].gps and _mbc[mb].gps->gps_detected()) clock_sources.push_back("gpsdo");
        _tree->create<std::vector<std::string> >(mb_path / "clock_source/options").set(clock_sources);
        //plug timed commands into tree here
        switch(_mbc[mb].iface->get_rev()){
        case usrp2_iface::USRP_N200:
        case usrp2_iface::USRP_N210:
        case usrp2_iface::USRP_N200_R4:
        case usrp2_iface::USRP_N210_R4:
            _tree->create<time_spec_t>(mb_path / "time/cmd")
                .add_coerced_subscriber(boost::bind(&usrp2_fifo_ctrl::set_time, _mbc[mb].fifo_ctrl, _1));
        default: break; //otherwise, do not register
        }
        _tree->access<double>(mb_path / "tick_rate")
            .add_coerced_subscriber(boost::bind(&usrp2_fifo_ctrl::set_tick_rate, _mbc[mb].fifo_ctrl, _1));

        ////////////////////////////////////////////////////////////////////
        // create user-defined control objects
        ////////////////////////////////////////////////////////////////////
        _mbc[mb].user = user_settings_core_200::make(_mbc[mb].wbiface, U2_REG_SR_ADDR(SR_USER_REGS));
        _tree->create<user_settings_core_200::user_reg_t>(mb_path / "user/regs")
            .add_coerced_subscriber(boost::bind(&user_settings_core_200::set_reg, _mbc[mb].user, _1));

        ////////////////////////////////////////////////////////////////
        // create dboard control objects
        ////////////////////////////////////////////////////////////////

        //read the dboard eeprom to extract the dboard ids
        dboard_eeprom_t rx_db_eeprom, tx_db_eeprom, gdb_eeprom;
        rx_db_eeprom.load(*_mbc[mb].iface, USRP2_I2C_ADDR_RX_DB);
        tx_db_eeprom.load(*_mbc[mb].iface, USRP2_I2C_ADDR_TX_DB);
        gdb_eeprom.load(*_mbc[mb].iface, USRP2_I2C_ADDR_TX_DB ^ 5);

        //disable rx dc offset if LFRX
        if (rx_db_eeprom.id == 0x000f) _tree->access<bool>(rx_fe_path / "dc_offset" / "enable").set(false);

        //create the properties and register subscribers
        _tree->create<dboard_eeprom_t>(mb_path / "dboards/A/rx_eeprom")
            .set(rx_db_eeprom)
            .add_coerced_subscriber(boost::bind(&usrp2_impl::set_db_eeprom, this, mb, "rx", _1));
        _tree->create<dboard_eeprom_t>(mb_path / "dboards/A/tx_eeprom")
            .set(tx_db_eeprom)
            .add_coerced_subscriber(boost::bind(&usrp2_impl::set_db_eeprom, this, mb, "tx", _1));
        _tree->create<dboard_eeprom_t>(mb_path / "dboards/A/gdb_eeprom")
            .set(gdb_eeprom)
            .add_coerced_subscriber(boost::bind(&usrp2_impl::set_db_eeprom, this, mb, "gdb", _1));

        //create a new dboard interface and manager
        _mbc[mb].dboard_manager = dboard_manager::make(
            rx_db_eeprom, tx_db_eeprom, gdb_eeprom,
            make_usrp2_dboard_iface(_mbc[mb].wbiface, _mbc[mb].iface/*i2c*/, _mbc[mb].spiface, _mbc[mb].clock),
            _tree->subtree(mb_path / "dboards/A")
        );

        //bind frontend corrections to the dboard freq props
        const fs_path db_tx_fe_path = mb_path / "dboards" / "A" / "tx_frontends";
        BOOST_FOREACH(const std::string &name, _tree->list(db_tx_fe_path)){
            _tree->access<double>(db_tx_fe_path / name / "freq" / "value")
                .add_coerced_subscriber(boost::bind(&usrp2_impl::set_tx_fe_corrections, this, mb, _1));
        }
        const fs_path db_rx_fe_path = mb_path / "dboards" / "A" / "rx_frontends";
        BOOST_FOREACH(const std::string &name, _tree->list(db_rx_fe_path)){
            _tree->access<double>(db_rx_fe_path / name / "freq" / "value")
                .add_coerced_subscriber(boost::bind(&usrp2_impl::set_rx_fe_corrections, this, mb, _1));
        }
    }

    //initialize io handling
    this->io_init();

    //do some post-init tasks
    this->update_rates();
    BOOST_FOREACH(const std::string &mb, _mbc.keys()){
        fs_path root = "/mboards/" + mb;

        //reset cordic rates and their properties to zero
        BOOST_FOREACH(const std::string &name, _tree->list(root / "rx_dsps")){
            _tree->access<double>(root / "rx_dsps" / name / "freq" / "value").set(0.0);
        }
        BOOST_FOREACH(const std::string &name, _tree->list(root / "tx_dsps")){
            _tree->access<double>(root / "tx_dsps" / name / "freq" / "value").set(0.0);
        }

        _tree->access<subdev_spec_t>(root / "rx_subdev_spec").set(subdev_spec_t("A:" + _tree->list(root / "dboards/A/rx_frontends").at(0)));
        _tree->access<subdev_spec_t>(root / "tx_subdev_spec").set(subdev_spec_t("A:" + _tree->list(root / "dboards/A/tx_frontends").at(0)));
        _tree->access<std::string>(root / "clock_source/value").set("internal");
        _tree->access<std::string>(root / "time_source/value").set("none");

        //GPS installed: use external ref, time, and init time spec
        if (_mbc[mb].gps and _mbc[mb].gps->gps_detected()){
            _mbc[mb].time64->enable_gpsdo();
            UHD_MSG(status) << "Setting references to the internal GPSDO" << std::endl;
            _tree->access<std::string>(root / "time_source/value").set("gpsdo");
            _tree->access<std::string>(root / "clock_source/value").set("gpsdo");
        }
    }

}

usrp2_impl::~usrp2_impl(void){UHD_SAFE_CALL(
    BOOST_FOREACH(const std::string &mb, _mbc.keys()){
        _mbc[mb].tx_dsp->set_updates(0, 0);
    }
)}

void usrp2_impl::set_mb_eeprom(const std::string &mb, const uhd::usrp::mboard_eeprom_t &mb_eeprom){
    mb_eeprom.commit(*(_mbc[mb].iface), USRP2_EEPROM_MAP_KEY);
}

void usrp2_impl::set_db_eeprom(const std::string &mb, const std::string &type, const uhd::usrp::dboard_eeprom_t &db_eeprom){
    if (type == "rx") db_eeprom.store(*_mbc[mb].iface, USRP2_I2C_ADDR_RX_DB);
    if (type == "tx") db_eeprom.store(*_mbc[mb].iface, USRP2_I2C_ADDR_TX_DB);
    if (type == "gdb") db_eeprom.store(*_mbc[mb].iface, USRP2_I2C_ADDR_TX_DB ^ 5);
}

sensor_value_t usrp2_impl::get_mimo_locked(const std::string &mb){
    const bool lock = (_mbc[mb].wbiface->peek32(U2_REG_IRQ_RB) & (1<<10)) != 0;
    return sensor_value_t("MIMO", lock, "locked", "unlocked");
}

sensor_value_t usrp2_impl::get_ref_locked(const std::string &mb){
    const bool lock = (_mbc[mb].wbiface->peek32(U2_REG_IRQ_RB) & (1<<11)) != 0;
    return sensor_value_t("Ref", lock, "locked", "unlocked");
}

void usrp2_impl::set_rx_fe_corrections(const std::string &mb, const double lo_freq){
    if(not _ignore_cal_file){
        apply_rx_fe_corrections(this->get_tree()->subtree("/mboards/" + mb), "A", lo_freq);
    }
}

void usrp2_impl::set_tx_fe_corrections(const std::string &mb, const double lo_freq){
    if(not _ignore_cal_file){
        apply_tx_fe_corrections(this->get_tree()->subtree("/mboards/" + mb), "A", lo_freq);
    }
}

#include <boost/math/special_functions/round.hpp>
#include <boost/math/special_functions/sign.hpp>

void usrp2_impl::update_clock_source(const std::string &mb, const std::string &source){
    //NOTICE: U2_REG_MISC_CTRL_CLOCK is on the wb clock, and cannot be set from fifo_ctrl
    //clock source ref 10mhz
    switch(_mbc[mb].iface->get_rev()){
    case usrp2_iface::USRP_N200:
    case usrp2_iface::USRP_N210:
    case usrp2_iface::USRP_N200_R4:
    case usrp2_iface::USRP_N210_R4:
        if      (source == "internal")  _mbc[mb].iface->poke32(U2_REG_MISC_CTRL_CLOCK, 0x12);
        else if (source == "external")  _mbc[mb].iface->poke32(U2_REG_MISC_CTRL_CLOCK, 0x1C);
        else if (source == "gpsdo")     _mbc[mb].iface->poke32(U2_REG_MISC_CTRL_CLOCK, 0x1C);
        else if (source == "mimo")      _mbc[mb].iface->poke32(U2_REG_MISC_CTRL_CLOCK, 0x15);
        else throw uhd::value_error("unhandled clock configuration reference source: " + source);
        _mbc[mb].clock->enable_external_ref(true); //USRP2P has an internal 10MHz TCXO
        break;

    case usrp2_iface::USRP2_REV3:
    case usrp2_iface::USRP2_REV4:
        if      (source == "internal")  _mbc[mb].iface->poke32(U2_REG_MISC_CTRL_CLOCK, 0x10);
        else if (source == "external")  _mbc[mb].iface->poke32(U2_REG_MISC_CTRL_CLOCK, 0x1C);
        else if (source == "mimo")      _mbc[mb].iface->poke32(U2_REG_MISC_CTRL_CLOCK, 0x15);
        else throw uhd::value_error("unhandled clock configuration reference source: " + source);
        _mbc[mb].clock->enable_external_ref(source != "internal");
        break;

    case usrp2_iface::USRP_NXXX: break;
    }

    //always drive the clock over serdes if not locking to it
    _mbc[mb].clock->enable_mimo_clock_out(source != "mimo");

    //set the mimo clock delay over the serdes
    if (source != "mimo"){
        switch(_mbc[mb].iface->get_rev()){
        case usrp2_iface::USRP_N200:
        case usrp2_iface::USRP_N210:
        case usrp2_iface::USRP_N200_R4:
        case usrp2_iface::USRP_N210_R4:
            _mbc[mb].clock->set_mimo_clock_delay(mimo_clock_delay_usrp_n2xx);
            break;

        case usrp2_iface::USRP2_REV4:
            _mbc[mb].clock->set_mimo_clock_delay(mimo_clock_delay_usrp2_rev4);
            break;

        default: break; //not handled
        }
    }
}
