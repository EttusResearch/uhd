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
#include "usrp2_regs.hpp"
#include <uhd/usrp/gps_ctrl.hpp>
#include <uhd/usrp/misc_utils.hpp>
#include <uhd/usrp/dsp_utils.hpp>
#include <uhd/usrp/mboard_props.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/assert.hpp>
#include <uhd/utils/algorithm.hpp>
#include <boost/bind.hpp>
#include <iostream>

static const double mimo_clock_delay_usrp2_rev4 = 4.18e-9;
static const double mimo_clock_delay_usrp_n2xx = 3.55e-9;
static const size_t mimo_clock_sync_delay_cycles = 137;

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;

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
usrp2_mboard_impl::usrp2_mboard_impl(
    const device_addr_t &device_addr, //global args passed into make
    const device_addr_t &device_args, //separated mboard specific args
    size_t index, usrp2_impl &device
):
    _index(index), _device(device),
    _iface(usrp2_iface::make(udp_simple::make_connected(
        device_addr["addr"], boost::lexical_cast<std::string>(USRP2_UDP_CTRL_PORT)
    )))
{

    //setup the dsp transport hints (default to a large recv buff)
    device_addr_t dsp_xport_hints = device_addr;
    if (not dsp_xport_hints.has_key("recv_buff_size")){
        //only enable on platforms that are happy with the large buffer resize
        #if defined(UHD_PLATFORM_LINUX) || defined(UHD_PLATFORM_WIN32)
            //set to half-a-second of buffering at max rate
            dsp_xport_hints["recv_buff_size"] = "50e6";
        #endif /*defined(UHD_PLATFORM_LINUX) || defined(UHD_PLATFORM_WIN32)*/
    }

    //construct transports for dsp and async errors
    std::cout << "Making transport for DSP0..." << std::endl;
    device.dsp_xports.push_back(udp_zero_copy::make(
        device_addr["addr"], boost::lexical_cast<std::string>(USRP2_UDP_DSP0_PORT), dsp_xport_hints
    ));
    init_xport(device.dsp_xports.back());

    std::cout << "Making transport for DSP1..." << std::endl;
    device.dsp_xports.push_back(udp_zero_copy::make(
        device_addr["addr"], boost::lexical_cast<std::string>(USRP2_UDP_DSP1_PORT), dsp_xport_hints
    ));
    init_xport(device.dsp_xports.back());

    std::cout << "Making transport for ERR0..." << std::endl;
    device.err_xports.push_back(udp_zero_copy::make(
        device_addr["addr"], boost::lexical_cast<std::string>(USRP2_UDP_ERR0_PORT), device_addr_t()
    ));
    init_xport(device.err_xports.back());

    //contruct the interfaces to mboard perifs
    _clock_ctrl = usrp2_clock_ctrl::make(_iface);
    _codec_ctrl = usrp2_codec_ctrl::make(_iface);
//    _gps_ctrl = gps_ctrl::make(
//        _iface->get_gps_write_fn(),
//        _iface->get_gps_read_fn());

    //if(_gps_ctrl->gps_detected()) std::cout << "GPS time: " << _gps_ctrl->get_time() << std::endl;

    //init the dsp stuff (before setting update packets)
    dsp_init();

    //setting the cycles per update (disabled by default)
    const double ups_per_sec = device_args.cast<double>("ups_per_sec", 0.0);
    if (ups_per_sec > 0.0){
        const size_t cycles_per_up = size_t(_clock_ctrl->get_master_clock_rate()/ups_per_sec);
        _iface->poke32(_iface->regs.tx_ctrl_cycles_per_up, U2_FLAG_TX_CTRL_UP_ENB | cycles_per_up);
    }

    //setting the packets per update (enabled by default)
    size_t send_frame_size = device.dsp_xports[0]->get_send_frame_size();
    const double ups_per_fifo = device_args.cast<double>("ups_per_fifo", 8.0);
    if (ups_per_fifo > 0.0){
        const size_t packets_per_up = size_t(usrp2_impl::sram_bytes/ups_per_fifo/send_frame_size);
        _iface->poke32(_iface->regs.tx_ctrl_packets_per_up, U2_FLAG_TX_CTRL_UP_ENB | packets_per_up);
    }

    //initialize the clock configuration
    if (device_args.has_key("mimo_mode")){
        if (device_args["mimo_mode"] == "master"){
            _mimo_clocking_mode_is_master = true;
        }
        else if (device_args["mimo_mode"] == "slave"){
            _mimo_clocking_mode_is_master = false;
        }
        else throw std::runtime_error(
            "mimo_mode must be set to master or slave"
        );
    }
    else {
        _mimo_clocking_mode_is_master = (_iface->peek32(_iface->regs.status) & (1 << 8)) != 0;
    }
    std::cout << boost::format("mboard%d MIMO %s") % _index %
        (_mimo_clocking_mode_is_master?"master":"slave") << std::endl;

    //init the clock config
    _clock_config = clock_config_t::internal();
    update_clock_config();

    //init the codec before the dboard
    codec_init();

    //init the tx and rx dboards (do last)
    dboard_init();

    //set default subdev specs
    (*this)[MBOARD_PROP_RX_SUBDEV_SPEC] = subdev_spec_t();
    (*this)[MBOARD_PROP_TX_SUBDEV_SPEC] = subdev_spec_t();

    //This is a hack/fix for the lingering packet problem.
    stream_cmd_t stream_cmd(stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    for (size_t i = 0; i < NUM_RX_DSPS; i++){
        size_t index = device.dsp_xports.size() - NUM_RX_DSPS + i;
        stream_cmd.num_samps = 1;
        this->issue_ddc_stream_cmd(stream_cmd, i);
        device.dsp_xports.at(index)->get_recv_buff(0.01).get(); //recv with timeout for lingering
        device.dsp_xports.at(index)->get_recv_buff(0.01).get(); //recv with timeout for expected
        _iface->poke32(_iface->regs.rx_ctrl[i].clear_overrun, 1); //resets sequence
    }
}

usrp2_mboard_impl::~usrp2_mboard_impl(void){
    _iface->poke32(_iface->regs.tx_ctrl_cycles_per_up, 0);
    _iface->poke32(_iface->regs.tx_ctrl_packets_per_up, 0);
}

/***********************************************************************
 * Helper Methods
 **********************************************************************/
void usrp2_mboard_impl::update_clock_config(void){
    boost::uint32_t pps_flags = 0;

    //translate pps source enums
    switch(_clock_config.pps_source){
    case clock_config_t::PPS_SMA:  pps_flags |= U2_FLAG_TIME64_PPS_SMA;  break;
    default: throw std::runtime_error("unhandled clock configuration pps source");
    }

    //translate pps polarity enums
    switch(_clock_config.pps_polarity){
    case clock_config_t::PPS_POS: pps_flags |= U2_FLAG_TIME64_PPS_POSEDGE; break;
    case clock_config_t::PPS_NEG: pps_flags |= U2_FLAG_TIME64_PPS_NEGEDGE; break;
    default: throw std::runtime_error("unhandled clock configuration pps polarity");
    }

    //set the pps flags
    _iface->poke32(_iface->regs.time64_flags, pps_flags);

    //clock source ref 10mhz
    switch(_iface->get_rev()){
    case usrp2_iface::USRP_N200:
    case usrp2_iface::USRP_N210:
        switch(_clock_config.ref_source){
        case clock_config_t::REF_INT : _iface->poke32(_iface->regs.misc_ctrl_clock, 0x12); break;
        case clock_config_t::REF_SMA : _iface->poke32(_iface->regs.misc_ctrl_clock, 0x1C); break;
        default: throw std::runtime_error("unhandled clock configuration reference source");
        }
        _clock_ctrl->enable_external_ref(true); //USRP2P has an internal 10MHz TCXO
        break;

    case usrp2_iface::USRP2_REV3:
    case usrp2_iface::USRP2_REV4:
        switch(_clock_config.ref_source){
        case clock_config_t::REF_INT : _iface->poke32(_iface->regs.misc_ctrl_clock, 0x10); break;
        case clock_config_t::REF_SMA : _iface->poke32(_iface->regs.misc_ctrl_clock, 0x1C); break;
        default: throw std::runtime_error("unhandled clock configuration reference source");
        }
        _clock_ctrl->enable_external_ref(_clock_config.ref_source != clock_config_t::REF_INT);
        break;

    case usrp2_iface::USRP_NXXX: break;
    }

    //Handle the serdes clocking based on master/slave mode:
    //   - Masters always drive the clock over serdes.
    //   - Slaves always lock to this serdes clock.
    //   - Slaves lock their time over the serdes.
    if (_mimo_clocking_mode_is_master){
        _clock_ctrl->enable_mimo_clock_out(true);
        switch(_iface->get_rev()){
        case usrp2_iface::USRP_N200:
        case usrp2_iface::USRP_N210:
            _clock_ctrl->set_mimo_clock_delay(mimo_clock_delay_usrp_n2xx);
            break;

        case usrp2_iface::USRP2_REV4:
            _clock_ctrl->set_mimo_clock_delay(mimo_clock_delay_usrp2_rev4);
            break;

        default: break; //not handled
        }
        _iface->poke32(_iface->regs.time64_mimo_sync, 0);
    }
    else{
        _iface->poke32(_iface->regs.misc_ctrl_clock, 0x15);
        _clock_ctrl->enable_external_ref(true);
        _clock_ctrl->enable_mimo_clock_out(false);
        _iface->poke32(_iface->regs.time64_mimo_sync,
            (1 << 8) | (mimo_clock_sync_delay_cycles & 0xff)
        );
    }

}

void usrp2_mboard_impl::set_time_spec(const time_spec_t &time_spec, bool now){
    //dont set the time for slave devices, they always take from mimo cable
    if (not _mimo_clocking_mode_is_master) return;

    //set the ticks
    _iface->poke32(_iface->regs.time64_ticks, time_spec.get_tick_count(get_master_clock_freq()));

    //set the flags register
    boost::uint32_t imm_flags = (now)? U2_FLAG_TIME64_LATCH_NOW : U2_FLAG_TIME64_LATCH_NEXT_PPS;
    _iface->poke32(_iface->regs.time64_imm, imm_flags);

    //set the seconds (latches in all 3 registers)
    _iface->poke32(_iface->regs.time64_secs, boost::uint32_t(time_spec.get_full_secs()));
}

/***********************************************************************
 * MBoard Get Properties
 **********************************************************************/
static const std::string dboard_name = "0";

void usrp2_mboard_impl::get(const wax::obj &key_, wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);
    //handle the get request conditioned on the key
    switch(key.as<mboard_prop_t>()){
    case MBOARD_PROP_NAME:
        val = _iface->get_cname() + " mboard";
        return;

    case MBOARD_PROP_OTHERS:
        val = prop_names_t();
        return;

    case MBOARD_PROP_RX_DBOARD:
        UHD_ASSERT_THROW(key.name == dboard_name);
        val = _rx_dboard_proxy->get_link();
        return;

    case MBOARD_PROP_RX_DBOARD_NAMES:
        val = prop_names_t(1, dboard_name);
        return;

    case MBOARD_PROP_TX_DBOARD:
        UHD_ASSERT_THROW(key.name == dboard_name);
        val = _tx_dboard_proxy->get_link();
        return;

    case MBOARD_PROP_TX_DBOARD_NAMES:
        val = prop_names_t(1, dboard_name);
        return;

    case MBOARD_PROP_RX_DSP:
        val = _rx_dsp_proxies[key.name]->get_link();
        return;

    case MBOARD_PROP_RX_DSP_NAMES:
        val = _rx_dsp_proxies.keys();
        return;

    case MBOARD_PROP_TX_DSP:
        val = _tx_dsp_proxies[key.name]->get_link();
        return;

    case MBOARD_PROP_TX_DSP_NAMES:
        val = _tx_dsp_proxies.keys();
        return;

    case MBOARD_PROP_CLOCK_CONFIG:
        val = _clock_config;
        return;

    case MBOARD_PROP_TIME_NOW: while(true){
        uint32_t secs = _iface->peek32(_iface->regs.time64_secs_rb_imm);
        uint32_t ticks = _iface->peek32(_iface->regs.time64_ticks_rb_imm);
        if (secs != _iface->peek32(_iface->regs.time64_secs_rb_imm)) continue;
        val = time_spec_t(secs, ticks, get_master_clock_freq());
        return;
    }

    case MBOARD_PROP_TIME_PPS: while(true){
        uint32_t secs = _iface->peek32(_iface->regs.time64_secs_rb_pps);
        uint32_t ticks = _iface->peek32(_iface->regs.time64_ticks_rb_pps);
        if (secs != _iface->peek32(_iface->regs.time64_secs_rb_pps)) continue;
        val = time_spec_t(secs, ticks, get_master_clock_freq());
        return;
    }

    case MBOARD_PROP_RX_SUBDEV_SPEC:
        val = _rx_subdev_spec;
        return;

    case MBOARD_PROP_TX_SUBDEV_SPEC:
        val = _tx_subdev_spec;
        return;

    case MBOARD_PROP_EEPROM_MAP:
        val = _iface->mb_eeprom;
        return;

    case MBOARD_PROP_CLOCK_RATE:
        val = this->get_master_clock_freq();
        return;

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

/***********************************************************************
 * MBoard Set Properties
 **********************************************************************/
void usrp2_mboard_impl::set(const wax::obj &key, const wax::obj &val){
    //handle the set request conditioned on the key
    switch(key.as<mboard_prop_t>()){

    case MBOARD_PROP_CLOCK_CONFIG:
        _clock_config = val.as<clock_config_t>();
        update_clock_config();
        return;

    case MBOARD_PROP_TIME_NOW:
        set_time_spec(val.as<time_spec_t>(), true);
        return;

    case MBOARD_PROP_TIME_PPS:
        set_time_spec(val.as<time_spec_t>(), false);
        return;

    case MBOARD_PROP_RX_SUBDEV_SPEC:
        _rx_subdev_spec = val.as<subdev_spec_t>();
        verify_rx_subdev_spec(_rx_subdev_spec, this->get_link());
        //sanity check
        UHD_ASSERT_THROW(_rx_subdev_spec.size() <= NUM_RX_DSPS);
        //set the mux
        for (size_t i = 0; i < _rx_subdev_spec.size(); i++){
            if (_rx_subdev_spec.size() >= 1) _iface->poke32(_iface->regs.dsp_rx[i].mux, dsp_type1::calc_rx_mux_word(
                _dboard_manager->get_rx_subdev(_rx_subdev_spec[i].sd_name)[SUBDEV_PROP_CONNECTION].as<subdev_conn_t>()
            ));
        }
        _device.update_xport_channel_mapping();
        return;

    case MBOARD_PROP_TX_SUBDEV_SPEC:
        _tx_subdev_spec = val.as<subdev_spec_t>();
        verify_tx_subdev_spec(_tx_subdev_spec, this->get_link());
        //sanity check
        UHD_ASSERT_THROW(_tx_subdev_spec.size() <= NUM_TX_DSPS);
        //set the mux
        for (size_t i = 0; i < _rx_subdev_spec.size(); i++){
            _iface->poke32(_iface->regs.dsp_tx_mux, dsp_type1::calc_tx_mux_word(
                _dboard_manager->get_tx_subdev(_tx_subdev_spec[i].sd_name)[SUBDEV_PROP_CONNECTION].as<subdev_conn_t>()
            ));
        }
        _device.update_xport_channel_mapping();
        return;

    case MBOARD_PROP_EEPROM_MAP:
        // Step1: commit the map, writing only those values set.
        // Step2: readback the entire eeprom map into the iface.
        val.as<mboard_eeprom_t>().commit(*_iface, mboard_eeprom_t::MAP_N100);
        _iface->mb_eeprom = mboard_eeprom_t(*_iface, mboard_eeprom_t::MAP_N100);
        return;

    case MBOARD_PROP_CLOCK_RATE:
        UHD_ASSERT_THROW(val.as<double>() == this->get_master_clock_freq());
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}
