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
#include "../dsp_utils.hpp"
#include <uhd/usrp/mboard_props.hpp>
#include <uhd/utils/assert.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/types/mac_addr.hpp>
#include <uhd/types/dict.hpp>
#include <boost/bind.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <iostream>

using namespace uhd;
using namespace uhd::usrp;

/***********************************************************************
 * Structors
 **********************************************************************/
usrp2_mboard_impl::usrp2_mboard_impl(
    size_t index,
    transport::udp_simple::sptr ctrl_transport,
    const usrp2_io_helper &io_helper
):
    _index(index),
    _io_helper(io_helper)
{
    //make a new interface for usrp2 stuff
    _iface = usrp2_iface::make(ctrl_transport);

    //extract the mboard rev numbers
    _rev_lo = _iface->read_eeprom(I2C_ADDR_MBOARD, EE_MBOARD_REV_LSB, 1).at(0);
    _rev_hi = _iface->read_eeprom(I2C_ADDR_MBOARD, EE_MBOARD_REV_MSB, 1).at(0);

    //contruct the interfaces to mboard perifs
    _clock_ctrl = usrp2_clock_ctrl::make(_iface);
    _codec_ctrl = usrp2_codec_ctrl::make(_iface);
    _serdes_ctrl = usrp2_serdes_ctrl::make(_iface);

    //TODO move to dsp impl...
    //load the allowed decim/interp rates
    //_USRP2_RATES = range(4, 128+1, 1) + range(130, 256+1, 2) + range(260, 512+1, 4)
    _allowed_decim_and_interp_rates.clear();
    for (size_t i = 4; i <= 128; i+=1){
        _allowed_decim_and_interp_rates.push_back(i);
    }
    for (size_t i = 130; i <= 256; i+=2){
        _allowed_decim_and_interp_rates.push_back(i);
    }
    for (size_t i = 260; i <= 512; i+=4){
        _allowed_decim_and_interp_rates.push_back(i);
    }

    //init the rx control registers
    _iface->poke32(U2_REG_RX_CTRL_NSAMPS_PER_PKT, _io_helper.get_max_recv_samps_per_packet());
    _iface->poke32(U2_REG_RX_CTRL_NCHANNELS, 1);
    _iface->poke32(U2_REG_RX_CTRL_CLEAR_OVERRUN, 1); //reset
    _iface->poke32(U2_REG_RX_CTRL_VRT_HEADER, 0
        | (0x1 << 28) //if data with stream id
        | (0x1 << 26) //has trailer
        | (0x3 << 22) //integer time other
        | (0x1 << 20) //fractional time sample count
    );
    _iface->poke32(U2_REG_RX_CTRL_VRT_STREAM_ID, 0);
    _iface->poke32(U2_REG_RX_CTRL_VRT_TRAILER, 0);
    _iface->poke32(U2_REG_TIME64_TPS, size_t(get_master_clock_freq()));

    //init the tx control registers
    _iface->poke32(U2_REG_TX_CTRL_NUM_CHAN, 0);    //1 channel
    _iface->poke32(U2_REG_TX_CTRL_CLEAR_STATE, 1); //reset
    _iface->poke32(U2_REG_TX_CTRL_REPORT_SID, 1);  //sid 1 (different from rx)

    //init the ddc
    init_ddc_config();

    //init the duc
    init_duc_config();

    //initialize the clock configuration
    init_clock_config();

    //init the tx and rx dboards (do last)
    dboard_init();
}

usrp2_mboard_impl::~usrp2_mboard_impl(void){
    /* NOP */
}

/***********************************************************************
 * Helper Methods
 **********************************************************************/
void usrp2_mboard_impl::init_clock_config(void){
    //setup the clock configuration settings
    _clock_config.ref_source = clock_config_t::REF_INT;
    _clock_config.pps_source = clock_config_t::PPS_SMA;
    _clock_config.pps_polarity = clock_config_t::PPS_NEG;

    //update the clock config (sends a control packet)
    update_clock_config();
}

void usrp2_mboard_impl::update_clock_config(void){
    boost::uint32_t pps_flags = 0;

    //translate pps source enums
    switch(_clock_config.pps_source){
    case clock_config_t::PPS_SMA:  pps_flags |= U2_FLAG_TIME64_PPS_SMA;  break;
    case clock_config_t::PPS_MIMO: pps_flags |= U2_FLAG_TIME64_PPS_MIMO; break;
    default: throw std::runtime_error("usrp2: unhandled clock configuration pps source");
    }

    //translate pps polarity enums
    switch(_clock_config.pps_polarity){
    case clock_config_t::PPS_POS: pps_flags |= U2_FLAG_TIME64_PPS_POSEDGE; break;
    case clock_config_t::PPS_NEG: pps_flags |= U2_FLAG_TIME64_PPS_NEGEDGE; break;
    default: throw std::runtime_error("usrp2: unhandled clock configuration pps polarity");
    }

    //set the pps flags
    _iface->poke32(U2_REG_TIME64_FLAGS, pps_flags);

    //clock source ref 10mhz
    switch(_clock_config.ref_source){
    case clock_config_t::REF_INT : _iface->poke32(U2_REG_MISC_CTRL_CLOCK, 0x10); break;
    case clock_config_t::REF_SMA : _iface->poke32(U2_REG_MISC_CTRL_CLOCK, 0x1C); break;
    case clock_config_t::REF_MIMO: _iface->poke32(U2_REG_MISC_CTRL_CLOCK, 0x15); break;
    default: throw std::runtime_error("usrp2: unhandled clock configuration reference source");
    }

    //clock source ref 10mhz
    bool use_external = _clock_config.ref_source != clock_config_t::REF_INT;
    _clock_ctrl->enable_external_ref(use_external);
}

void usrp2_mboard_impl::set_time_spec(const time_spec_t &time_spec, bool now){
    //set the ticks
    _iface->poke32(U2_REG_TIME64_TICKS, time_spec.get_tick_count(get_master_clock_freq()));

    //set the flags register
    boost::uint32_t imm_flags = (now)? U2_FLAG_TIME64_LATCH_NOW : U2_FLAG_TIME64_LATCH_NEXT_PPS;
    _iface->poke32(U2_REG_TIME64_IMM, imm_flags);

    //set the seconds (latches in all 3 registers)
    _iface->poke32(U2_REG_TIME64_SECS, boost::uint32_t(time_spec.get_full_secs()));
}

void usrp2_mboard_impl::issue_ddc_stream_cmd(const stream_cmd_t &stream_cmd){
    _iface->poke32(U2_REG_RX_CTRL_STREAM_CMD, dsp_type1::calc_stream_cmd_word(
        stream_cmd, _io_helper.get_max_recv_samps_per_packet()
    ));
    _iface->poke32(U2_REG_RX_CTRL_TIME_SECS,  boost::uint32_t(stream_cmd.time_spec.get_full_secs()));
    _iface->poke32(U2_REG_RX_CTRL_TIME_TICKS, stream_cmd.time_spec.get_tick_count(get_master_clock_freq()));
}

/***********************************************************************
 * MBoard Get Properties
 **********************************************************************/
void usrp2_mboard_impl::get(const wax::obj &key_, wax::obj &val){
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
        val = str(boost::format("usrp2 mboard%d - rev %d:%d") % _index % _rev_hi % _rev_lo);
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

    case MBOARD_PROP_TIME_NOW:{
            usrp2_iface::pair64 time64(
                _iface->peek64(U2_REG_TIME64_SECS_RB, U2_REG_TIME64_TICKS_RB)
            );
            val = time_spec_t(
                time64.first, time64.second, get_master_clock_freq()
            );
        }
        return;

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

/***********************************************************************
 * MBoard Set Properties
 **********************************************************************/
void usrp2_mboard_impl::set(const wax::obj &key, const wax::obj &val){
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
