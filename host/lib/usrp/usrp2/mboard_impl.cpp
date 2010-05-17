//
// Copyright 2010 Ettus Research LLC
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
#include <uhd/usrp/mboard_props.hpp>
#include <uhd/utils/assert.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/types/mac_addr.hpp>
#include <uhd/types/dict.hpp>
#include <boost/bind.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/assign/list_of.hpp>

using namespace uhd;
using namespace uhd::usrp;

/***********************************************************************
 * Helper Methods
 **********************************************************************/
void usrp2_impl::mboard_init(void){
    _mboard_proxy = wax_obj_proxy::make(
        boost::bind(&usrp2_impl::mboard_get, this, _1, _2),
        boost::bind(&usrp2_impl::mboard_set, this, _1, _2)
    );
}

void usrp2_impl::init_clock_config(void){
    //setup the clock configuration settings
    _clock_config.ref_source = clock_config_t::REF_INT;
    _clock_config.pps_source = clock_config_t::PPS_SMA;
    _clock_config.pps_polarity = clock_config_t::PPS_NEG;

    //update the clock config (sends a control packet)
    update_clock_config();
}

void usrp2_impl::update_clock_config(void){
    boost::uint32_t pps_flags = 0;

    //translate pps source enums
    switch(_clock_config.pps_source){
    case clock_config_t::PPS_SMA:  pps_flags |= FRF_TIME64_PPS_SMA;  break;
    case clock_config_t::PPS_MIMO: pps_flags |= FRF_TIME64_PPS_MIMO; break;
    default: throw std::runtime_error("usrp2: unhandled clock configuration pps source");
    }

    //translate pps polarity enums
    switch(_clock_config.pps_polarity){
    case clock_config_t::PPS_POS: pps_flags |= FRF_TIME64_PPS_POSEDGE; break;
    case clock_config_t::PPS_NEG: pps_flags |= FRF_TIME64_PPS_NEGEDGE; break;
    default: throw std::runtime_error("usrp2: unhandled clock configuration pps polarity");
    }

    //set the pps flags
    _iface->poke32(FR_TIME64_FLAGS, pps_flags);

    //clock source ref 10mhz
    switch(_clock_config.ref_source){
    case clock_config_t::REF_INT : _iface->poke32(FR_MISC_CTRL_CLOCK, 0x10); break;
    case clock_config_t::REF_SMA : _iface->poke32(FR_MISC_CTRL_CLOCK, 0x1C); break;
    case clock_config_t::REF_MIMO: _iface->poke32(FR_MISC_CTRL_CLOCK, 0x15); break;
    default: throw std::runtime_error("usrp2: unhandled clock configuration reference source");
    }

    //clock source ref 10mhz
    bool use_external = _clock_config.ref_source != clock_config_t::REF_INT;
    _clock_ctrl->enable_external_ref(use_external);
}

void usrp2_impl::set_time_spec(const time_spec_t &time_spec, bool now){
    //set ticks and seconds
    _iface->poke32(FR_TIME64_SECS, time_spec.secs);
    _iface->poke32(FR_TIME64_TICKS, time_spec.get_ticks(get_master_clock_freq()));

    //set the register to latch it all in
    boost::uint32_t imm_flags = (now)? FRF_TIME64_LATCH_NOW : FRF_TIME64_LATCH_NEXT_PPS;
    _iface->poke32(FR_TIME64_IMM, imm_flags);
}

void usrp2_impl::issue_ddc_stream_cmd(const stream_cmd_t &stream_cmd){
    UHD_ASSERT_THROW(stream_cmd.num_samps <= FR_RX_CTRL_MAX_SAMPS_PER_CMD);

    //setup the mode to instruction flags
    typedef boost::tuple<bool, bool, bool> inst_t;
    static const uhd::dict<stream_cmd_t::stream_mode_t, inst_t> mode_to_inst = boost::assign::map_list_of
                                                            //reload, chain, samps
        (stream_cmd_t::STREAM_MODE_START_CONTINUOUS,   inst_t(true,  true,  false))
        (stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS,    inst_t(false, false, false))
        (stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE, inst_t(false, false, true))
        (stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_MORE, inst_t(false, true,  true))
    ;

    //setup the instruction flag values
    bool inst_reload, inst_chain, inst_samps;
    boost::tie(inst_reload, inst_chain, inst_samps) = mode_to_inst[stream_cmd.stream_mode];

    //issue the stream command
    _iface->poke32(FR_RX_CTRL_STREAM_CMD, FR_RX_CTRL_MAKE_CMD(
        (inst_samps)? stream_cmd.num_samps : ((inst_chain)? max_rx_samps_per_packet() : 1),
        (stream_cmd.stream_now)? 1 : 0,
        (inst_chain)? 1 : 0,
        (inst_reload)? 1 : 0
    ));
    _iface->poke32(FR_RX_CTRL_TIME_SECS,  stream_cmd.time_spec.secs);
    _iface->poke32(FR_RX_CTRL_TIME_TICKS, stream_cmd.time_spec.get_ticks(get_master_clock_freq()));
}

/***********************************************************************
 * MBoard Get Properties
 **********************************************************************/
void usrp2_impl::mboard_get(const wax::obj &key_, wax::obj &val){
    wax::obj key; std::string name;
    boost::tie(key, name) = extract_named_prop(key_);

    //handle the other props
    if (key.type() == typeid(std::string)){
        if (key.as<std::string>() == "mac-addr"){
            byte_vector_t bytes = _iface->read_eeprom(I2C_ADDR_MBOARD, EE_MBOARD_MAC_ADDR, 6);
            val = mac_addr_t::from_bytes(bytes).to_string();
            return;
        }

        if (key.as<std::string>() == "ip-addr"){
            boost::asio::ip::address_v4::bytes_type bytes;
            std::copy(_iface->read_eeprom(I2C_ADDR_MBOARD, EE_MBOARD_IP_ADDR, 4), bytes);
            val = boost::asio::ip::address_v4(bytes).to_string();
            return;
        }
    }

    //handle the get request conditioned on the key
    switch(key.as<mboard_prop_t>()){
    case MBOARD_PROP_NAME:
        val = std::string("usrp2 mboard");
        return;

    case MBOARD_PROP_OTHERS:{
            prop_names_t others = boost::assign::list_of
                ("mac-addr")
                ("ip-addr")
            ;
            val = others;
        }
        return;

    case MBOARD_PROP_RX_DBOARD:
        UHD_ASSERT_THROW(name == "");
        val = _rx_dboard_proxy->get_link();
        return;

    case MBOARD_PROP_RX_DBOARD_NAMES:
        val = prop_names_t(1, "");
        return;

    case MBOARD_PROP_TX_DBOARD:
        UHD_ASSERT_THROW(name == "");
        val = _tx_dboard_proxy->get_link();
        return;

    case MBOARD_PROP_TX_DBOARD_NAMES:
        val = prop_names_t(1, "");
        return;

    case MBOARD_PROP_RX_DSP:
        UHD_ASSERT_THROW(name == "");
        val = _rx_dsp_proxy->get_link();
        return;

    case MBOARD_PROP_RX_DSP_NAMES:
        val = prop_names_t(1, "");
        return;

    case MBOARD_PROP_TX_DSP:
        UHD_ASSERT_THROW(name == "");
        val = _tx_dsp_proxy->get_link();
        return;

    case MBOARD_PROP_TX_DSP_NAMES:
        val = prop_names_t(1, "");
        return;

    case MBOARD_PROP_CLOCK_CONFIG:
        val = _clock_config;
        return;

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

/***********************************************************************
 * MBoard Set Properties
 **********************************************************************/
void usrp2_impl::mboard_set(const wax::obj &key, const wax::obj &val){
    //handle the other props
    if (key.type() == typeid(std::string)){
        if (key.as<std::string>() == "mac-addr"){
            byte_vector_t bytes = mac_addr_t::from_string(val.as<std::string>()).to_bytes();
            _iface->write_eeprom(I2C_ADDR_MBOARD, EE_MBOARD_MAC_ADDR, bytes);
            return;
        }

        if (key.as<std::string>() == "ip-addr"){
            byte_vector_t bytes(4);
            std::copy(boost::asio::ip::address_v4::from_string(val.as<std::string>()).to_bytes(), bytes);
            _iface->write_eeprom(I2C_ADDR_MBOARD, EE_MBOARD_IP_ADDR, bytes);
            return;
        }
    }

    //handle the get request conditioned on the key
    switch(key.as<mboard_prop_t>()){

    case MBOARD_PROP_CLOCK_CONFIG:
        _clock_config = val.as<clock_config_t>();
        update_clock_config();
        return;

    case MBOARD_PROP_TIME_NOW:
        set_time_spec(val.as<time_spec_t>(), true);
        return;

    case MBOARD_PROP_TIME_NEXT_PPS:
        set_time_spec(val.as<time_spec_t>(), false);
        return;

    case MBOARD_PROP_STREAM_CMD:
        issue_ddc_stream_cmd(val.as<stream_cmd_t>());
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}
