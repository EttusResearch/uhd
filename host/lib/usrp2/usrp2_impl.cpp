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
#include <uhd/types/ranges.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/safe_call.hpp>
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio.hpp> //used for htonl and ntohl

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
 * Helpers
 **********************************************************************/
static void init_xport(zero_copy_if::sptr xport){
    //Send a small data packet so the usrp2 knows the udp source port.
    //This setup must happen before further initialization occurs
    //or the async update packets will cause ICMP destination unreachable.
    static const boost::uint32_t data[2] = {
        uhd::htonx(boost::uint32_t(0 /* don't care seq num */)),
        uhd::htonx(boost::uint32_t(USRP2_INVALID_VRT_HEADER))
    };

    transport::managed_send_buffer::sptr send_buff = xport->get_send_buff();
    std::memcpy(send_buff->cast<void*>(), &data, sizeof(data));
    send_buff->commit(sizeof(data));
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

        const std::string addr = device_args[mb]["addr"];
        property_tree::path_type mb_path = str(boost::format("/mboards/%u") % mb);

        ////////////////////////////////////////////////////////////////
        // construct transports for dsp and async errors
        ////////////////////////////////////////////////////////////////
        UHD_LOG << "Making transport for DSP0..." << std::endl;
        _mboard_stuff[mb].dsp_xports.push_back(udp_zero_copy::make(
            addr, BOOST_STRINGIZE(USRP2_UDP_DSP0_PORT), device_args[mb]
        ));
        init_xport(_mboard_stuff[mb].dsp_xports.back());

        UHD_LOG << "Making transport for DSP1..." << std::endl;
        _mboard_stuff[mb].dsp_xports.push_back(udp_zero_copy::make(
            addr, BOOST_STRINGIZE(USRP2_UDP_DSP1_PORT), device_args[mb]
        ));
        init_xport(_mboard_stuff[mb].dsp_xports.back());

        UHD_LOG << "Making transport for ERR0..." << std::endl;
        _mboard_stuff[mb].err_xports.push_back(udp_zero_copy::make(
            addr, BOOST_STRINGIZE(USRP2_UDP_ERR0_PORT), device_addr_t()
        ));
        init_xport(_mboard_stuff[mb].err_xports.back());

        ////////////////////////////////////////////////////////////////
        // create the iface that controls i2c, spi, uart, and wb
        ////////////////////////////////////////////////////////////////
        _mboard_stuff[mb].iface = usrp2_iface::make(udp_simple::make_connected(
            addr, BOOST_STRINGIZE(USRP2_UDP_CTRL_PORT)
        ));
        _tree->create(mb_path / "name", property<std::string>(_mboard_stuff[mb].iface->get_cname()));

        ////////////////////////////////////////////////////////////////
        // setup the mboard eeprom
        ////////////////////////////////////////////////////////////////
        property<usrp::mboard_eeprom_t> mb_eeprom_prop;
        mb_eeprom_prop.set(_mboard_stuff[mb].iface->mb_eeprom);
        mb_eeprom_prop.subscribe(boost::bind(&usrp2_impl::set_mb_eeprom, this, mb, _1));
        _tree->create(mb_path / "eeprom", mb_eeprom_prop);

        ////////////////////////////////////////////////////////////////
        // create clock control objects
        ////////////////////////////////////////////////////////////////
        _mboard_stuff[mb].clock = usrp2_clock_ctrl::make(_mboard_stuff[mb].iface);
        const double tick_rate = _mboard_stuff[mb].clock->get_master_clock_rate();
        property<double> tick_rate_prop(tick_rate);
        _tree->create(mb_path / "tick_rate", tick_rate_prop);

        ////////////////////////////////////////////////////////////////
        // create codec control objects
        ////////////////////////////////////////////////////////////////
        property_tree::path_type rx_codec_path = mb_path / "rx_codecs/A";
        property_tree::path_type tx_codec_path = mb_path / "tx_codecs/A";
        _mboard_stuff[mb].codec = usrp2_codec_ctrl::make(_mboard_stuff[mb].iface);
        switch(_mboard_stuff[mb].iface->get_rev()){
        case usrp2_iface::USRP_N200:
        case usrp2_iface::USRP_N210:
        case usrp2_iface::USRP_N200_R4:
        case usrp2_iface::USRP_N210_R4:{
            _tree->create(rx_codec_path / "name", property<std::string>("ads62p44"));
            _tree->create(rx_codec_path / "gains/digital/range", property<meta_range_t>(meta_range_t(0, 6.0, 0.5)));
            property<double> dig_gain_prop, fine_gain_prop;
            dig_gain_prop.subscribe(boost::bind(&usrp2_codec_ctrl::set_rx_digital_gain, _mboard_stuff[mb].codec, _1));
            _tree->create(rx_codec_path / "gains/digital/value", dig_gain_prop);
            _tree->create(rx_codec_path / "gains/fine/range", property<meta_range_t>(meta_range_t(0, 0.5, 0.05)));
            fine_gain_prop.subscribe(boost::bind(&usrp2_codec_ctrl::set_rx_digital_fine_gain, _mboard_stuff[mb].codec, _1));
            _tree->create(rx_codec_path / "gains/fine/value", fine_gain_prop);
        }break;

        case usrp2_iface::USRP2_REV3:
        case usrp2_iface::USRP2_REV4:
            _tree->create(rx_codec_path / "name", property<std::string>("ltc2284"));
            break;

        case usrp2_iface::USRP_NXXX:
            _tree->create(rx_codec_path / "name", property<std::string>("??????"));
            break;
        }
        _tree->create(tx_codec_path / "name", property<std::string>("ad9777"));

        ////////////////////////////////////////////////////////////////
        // create gpsdo control objects
        ////////////////////////////////////////////////////////////////
        if (_mboard_stuff[mb].iface->mb_eeprom["gpsdo"] == "internal"){
            _mboard_stuff[mb].gps = gps_ctrl::make(
                _mboard_stuff[mb].iface->get_gps_write_fn(),
                _mboard_stuff[mb].iface->get_gps_read_fn()
            );
            BOOST_FOREACH(const std::string &name, _mboard_stuff[mb].gps->get_sensors()){
                property<sensor_value_t> sensor_prop;
                sensor_prop.publish(boost::bind(&gps_ctrl::get_sensor, _mboard_stuff[mb].gps, name));
                _tree->create(mb_path / "sensors" / name, sensor_prop);
            }
        }

        ////////////////////////////////////////////////////////////////
        // and do the misc mboard sensors
        ////////////////////////////////////////////////////////////////
        property<sensor_value_t> mimo_lock_sensor_prop;
        mimo_lock_sensor_prop.publish(boost::bind(&usrp2_impl::get_mimo_locked, this, mb));
        _tree->create(mb_path / "sensors/mimo_locked", mimo_lock_sensor_prop);
        property<sensor_value_t> ref_lock_sensor_prop;
        ref_lock_sensor_prop.publish(boost::bind(&usrp2_impl::get_ref_locked, this, mb));
        _tree->create(mb_path / "sensors/ref_locked", ref_lock_sensor_prop);





        //TODO //initialize VITA time to GPS time

        //TODO clock source, time source







        ////////////////////////////////////////////////////////////////
        // create frontend control objects
        ////////////////////////////////////////////////////////////////
        _mboard_stuff[mb].rx_fe = rx_frontend_core_200::make(
            _mboard_stuff[mb].iface, U2_REG_SR_ADDR(SR_RX_FRONT)
        );
        _mboard_stuff[mb].tx_fe = tx_frontend_core_200::make(
            _mboard_stuff[mb].iface, U2_REG_SR_ADDR(SR_TX_FRONT)
        );
        //TODO lots of properties to expose here for frontends

        ////////////////////////////////////////////////////////////////
        // create dsp control objects
        ////////////////////////////////////////////////////////////////
        _mboard_stuff[mb].rx_dsps.push_back(rx_dsp_core_200::make(
            _mboard_stuff[mb].iface, U2_REG_SR_ADDR(SR_RX_DSP0), U2_REG_SR_ADDR(SR_RX_CTRL0), 3
        ));
        _mboard_stuff[mb].rx_dsps.push_back(rx_dsp_core_200::make(
            _mboard_stuff[mb].iface, U2_REG_SR_ADDR(SR_RX_DSP1), U2_REG_SR_ADDR(SR_RX_CTRL1), 4
        ));
        for (size_t dspno = 0; dspno < _mboard_stuff[mb].rx_dsps.size(); dspno++){
            _mboard_stuff[mb].rx_dsps[dspno]->set_tick_rate(tick_rate); //does not change on usrp2
            property_tree::path_type rx_dsp_path = mb_path / str(boost::format("rx_dsps/%u") % dspno);
            property<double> host_rate_prop, freq_prop;
            host_rate_prop.subscribe_master(boost::bind(&rx_dsp_core_200::set_host_rate, _mboard_stuff[mb].rx_dsps[dspno], _1));
            _tree->create(rx_dsp_path / "rate/value", host_rate_prop);
            freq_prop.subscribe_master(boost::bind(&rx_dsp_core_200::set_freq, _mboard_stuff[mb].rx_dsps[dspno], _1));
            _tree->create(rx_dsp_path / "freq/value", freq_prop);
            //TODO set nsamps per packet
        }
        _mboard_stuff[mb].tx_dsp = tx_dsp_core_200::make(
            _mboard_stuff[mb].iface, U2_REG_SR_ADDR(SR_TX_DSP), U2_REG_SR_ADDR(SR_TX_CTRL), 2
        );
        _mboard_stuff[mb].tx_dsp->set_tick_rate(tick_rate); //does not change on usrp2
        property<double> tx_dsp_host_rate_prop, tx_dsp_freq_prop;
        tx_dsp_host_rate_prop.subscribe_master(boost::bind(&tx_dsp_core_200::set_host_rate, _mboard_stuff[mb].tx_dsp, _1));
        _tree->create(mb_path / "tx_dsps/0/rate/value", tx_dsp_host_rate_prop);
        tx_dsp_freq_prop.subscribe_master(boost::bind(&tx_dsp_core_200::set_freq, _mboard_stuff[mb].tx_dsp, _1));
        _tree->create(mb_path / "tx_dsps/0/freq/value", tx_dsp_freq_prop);
        //TODO combine w/ codec shift
        //setup dsp flow control
        const double ups_per_sec = device_args[mb].cast<double>("ups_per_sec", 20);
        const size_t send_frame_size = _mboard_stuff[mb].dsp_xports.front()->get_send_frame_size();
        const double ups_per_fifo = device_args[mb].cast<double>("ups_per_fifo", 8.0);
        _mboard_stuff[mb].tx_dsp->set_updates(
            (ups_per_sec > 0.0)? size_t(tick_rate/ups_per_sec) : 0,
            (ups_per_fifo > 0.0)? size_t(USRP2_SRAM_BYTES/ups_per_fifo/send_frame_size) : 0
        );

        ////////////////////////////////////////////////////////////////
        // create time control objects
        ////////////////////////////////////////////////////////////////
        time64_core_200::readback_bases_type time64_rb_bases;
        time64_rb_bases.rb_secs_imm = U2_REG_TIME64_SECS_RB_IMM;
        time64_rb_bases.rb_ticks_imm = U2_REG_TIME64_TICKS_RB_IMM;
        time64_rb_bases.rb_secs_pps = U2_REG_TIME64_SECS_RB_PPS;
        time64_rb_bases.rb_ticks_pps = U2_REG_TIME64_TICKS_RB_PPS;
        _mboard_stuff[mb].time64 = time64_core_200::make(
            _mboard_stuff[mb].iface, U2_REG_SR_ADDR(SR_TIME64), time64_rb_bases, mimo_clock_sync_delay_cycles
        );
        _mboard_stuff[mb].time64->set_tick_rate(tick_rate); //does not change on usrp2
        property<time_spec_t> time_now_prop, time_pps_prop;
        time_now_prop.publish(boost::bind(&time64_core_200::get_time_now, _mboard_stuff[mb].time64));
        time_now_prop.subscribe(boost::bind(&time64_core_200::set_time_now, _mboard_stuff[mb].time64, _1));
        _tree->create(mb_path / "time/now", time_now_prop);
        time_pps_prop.publish(boost::bind(&time64_core_200::get_time_last_pps, _mboard_stuff[mb].time64));
        time_pps_prop.subscribe(boost::bind(&time64_core_200::set_time_next_pps, _mboard_stuff[mb].time64, _1));
        _tree->create(mb_path / "time/pps", time_pps_prop);

        ////////////////////////////////////////////////////////////////
        // create dboard control objects
        ////////////////////////////////////////////////////////////////

        //read the dboard eeprom to extract the dboard ids
        dboard_eeprom_t rx_db_eeprom, tx_db_eeprom, gdb_eeprom;
        rx_db_eeprom.load(*_mboard_stuff[mb].iface, USRP2_I2C_ADDR_RX_DB);
        tx_db_eeprom.load(*_mboard_stuff[mb].iface, USRP2_I2C_ADDR_TX_DB);
        gdb_eeprom.load(*_mboard_stuff[mb].iface, USRP2_I2C_ADDR_TX_DB ^ 5);

        //create the properties and register subscribers
        property<dboard_eeprom_t> rx_db_eeprom_prop(rx_db_eeprom), tx_db_eeprom_prop(tx_db_eeprom), gdb_eeprom_prop(gdb_eeprom);
        rx_db_eeprom_prop.subscribe(boost::bind(&usrp2_impl::set_db_eeprom, this, mb, "rx", _1));
        _tree->create(mb_path / "dboards/A/rx_eeprom", rx_db_eeprom_prop);
        tx_db_eeprom_prop.subscribe(boost::bind(&usrp2_impl::set_db_eeprom, this, mb, "tx", _1));
        _tree->create(mb_path / "dboards/A/tx_eeprom", tx_db_eeprom_prop);
        gdb_eeprom_prop.subscribe(boost::bind(&usrp2_impl::set_db_eeprom, this, mb, "gdb", _1));
        _tree->create(mb_path / "dboards/A/gdb_eeprom", gdb_eeprom_prop);

        //create a new dboard interface and manager
        _mboard_stuff[mb].dboard_iface = make_usrp2_dboard_iface(_mboard_stuff[mb].iface, _mboard_stuff[mb].clock);
        _tree->create(mb_path / "dboards/A/iface", property<dboard_iface::sptr>(_mboard_stuff[mb].dboard_iface));
        _mboard_stuff[mb].dboard_manager = dboard_manager::make(
            rx_db_eeprom.id,
            ((gdb_eeprom.id == dboard_id_t::none())? tx_db_eeprom : gdb_eeprom).id,
            _mboard_stuff[mb].dboard_iface
        );
        BOOST_FOREACH(const std::string &name, _mboard_stuff[mb].dboard_manager->get_rx_subdev_names()){
            dboard_manager::populate_prop_tree_from_subdev(
                _tree, mb_path / "dboards/A/rx_frontends" / name,
                _mboard_stuff[mb].dboard_manager->get_rx_subdev(name)
            );
        }
        BOOST_FOREACH(const std::string &name, _mboard_stuff[mb].dboard_manager->get_tx_subdev_names()){
            dboard_manager::populate_prop_tree_from_subdev(
                _tree, mb_path / "dboards/A/tx_frontends" / name,
                _mboard_stuff[mb].dboard_manager->get_tx_subdev(name)
            );
        }
    }
}

usrp2_impl::~usrp2_impl(void){UHD_SAFE_CALL(
    for (size_t mb = 0; mb < _mboard_stuff.size(); mb++){
        _mboard_stuff[mb].tx_dsp->set_updates(0, 0);
    }
)}

void usrp2_impl::set_mb_eeprom(const size_t which_mb, const uhd::usrp::mboard_eeprom_t &mb_eeprom){
    mb_eeprom.commit(*(_mboard_stuff[which_mb].iface), mboard_eeprom_t::MAP_N100);
}

void usrp2_impl::set_db_eeprom(const size_t which_mb, const std::string &type, const uhd::usrp::dboard_eeprom_t &db_eeprom){
    if (type == "rx") db_eeprom.store(*_mboard_stuff[which_mb].iface, USRP2_I2C_ADDR_RX_DB);
    if (type == "tx") db_eeprom.store(*_mboard_stuff[which_mb].iface, USRP2_I2C_ADDR_TX_DB);
    if (type == "gdb") db_eeprom.store(*_mboard_stuff[which_mb].iface, USRP2_I2C_ADDR_TX_DB ^ 5);
}

sensor_value_t usrp2_impl::get_mimo_locked(const size_t which_mb){
    const bool lock = (_mboard_stuff[which_mb].iface->peek32(U2_REG_IRQ_RB) & (1<<10)) != 0;
    return sensor_value_t("MIMO", lock, "locked", "unlocked");
}

sensor_value_t usrp2_impl::get_ref_locked(const size_t which_mb){
    const bool lock = (_mboard_stuff[which_mb].iface->peek32(U2_REG_IRQ_RB) & (1<<11)) != 0;
    return sensor_value_t("Ref", lock, "locked", "unlocked");
}
