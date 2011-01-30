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

#include "usrp_e100_impl.hpp"
#include "usrp_e100_regs.hpp"
#include <uhd/usrp/dsp_utils.hpp>
#include <uhd/usrp/misc_utils.hpp>
#include <uhd/utils/assert.hpp>
#include <uhd/usrp/mboard_props.hpp>
#include <boost/bind.hpp>
#include <iostream>

using namespace uhd;
using namespace uhd::usrp;

/***********************************************************************
 * Mboard Initialization
 **********************************************************************/
void usrp_e100_impl::mboard_init(void){
    _mboard_proxy = wax_obj_proxy::make(
        boost::bind(&usrp_e100_impl::mboard_get, this, _1, _2),
        boost::bind(&usrp_e100_impl::mboard_set, this, _1, _2)
    );

    //set the ticks per seconds into the vita time control
    _iface->poke32(UE_REG_TIME64_TPS,
        boost::uint32_t(_clock_ctrl->get_fpga_clock_rate())
    );

    //init the clock config
    _clock_config = clock_config_t::internal();
    update_clock_config();
}

void usrp_e100_impl::update_clock_config(void){
    boost::uint32_t pps_flags = 0;

    //translate pps polarity enums
    switch(_clock_config.pps_polarity){
    case clock_config_t::PPS_POS: pps_flags |= UE_FLAG_TIME64_PPS_POSEDGE; break;
    case clock_config_t::PPS_NEG: pps_flags |= UE_FLAG_TIME64_PPS_NEGEDGE; break;
    default: throw std::runtime_error("unhandled clock configuration pps polarity");
    }

    //set the pps flags
    _iface->poke32(UE_REG_TIME64_FLAGS, pps_flags);

    //clock source ref 10mhz
    switch(_clock_config.ref_source){
    case clock_config_t::REF_AUTO: _clock_ctrl->use_auto_ref(); break;
    case clock_config_t::REF_INT: _clock_ctrl->use_internal_ref(); break;
    case clock_config_t::REF_SMA: _clock_ctrl->use_auto_ref(); break;
    default: throw std::runtime_error("unhandled clock configuration ref source");
    }
}

/***********************************************************************
 * Mboard Get
 **********************************************************************/
void usrp_e100_impl::mboard_get(const wax::obj &key_, wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<mboard_prop_t>()){
    case MBOARD_PROP_NAME:
        val = std::string("usrp-e mboard");
        return;

    case MBOARD_PROP_OTHERS:
        val = prop_names_t();
        return;

    case MBOARD_PROP_RX_DBOARD:
        UHD_ASSERT_THROW(key.name == "");
        val = _rx_dboard_proxy->get_link();
        return;

    case MBOARD_PROP_RX_DBOARD_NAMES:
        val = prop_names_t(1, ""); //vector of size 1 with empty string
        return;

    case MBOARD_PROP_TX_DBOARD:
        UHD_ASSERT_THROW(key.name == "");
        val = _tx_dboard_proxy->get_link();
        return;

    case MBOARD_PROP_TX_DBOARD_NAMES:
        val = prop_names_t(1, ""); //vector of size 1 with empty string
        return;

    case MBOARD_PROP_RX_DSP:
        UHD_ASSERT_THROW(key.name == "");
        val = _rx_ddc_proxy->get_link();
        return;

    case MBOARD_PROP_RX_DSP_NAMES:
        val = prop_names_t(1, "");
        return;

    case MBOARD_PROP_TX_DSP:
        UHD_ASSERT_THROW(key.name == "");
        val = _tx_duc_proxy->get_link();
        return;

    case MBOARD_PROP_TX_DSP_NAMES:
        val = prop_names_t(1, "");
        return;

    case MBOARD_PROP_CLOCK_CONFIG:
        val = _clock_config;
        return;

    case MBOARD_PROP_RX_SUBDEV_SPEC:
        val = _rx_subdev_spec;
        return;

    case MBOARD_PROP_TX_SUBDEV_SPEC:
        val = _tx_subdev_spec;
        return;

    case MBOARD_PROP_EEPROM_MAP:
        val = _iface->mb_eeprom;
        return;

    case MBOARD_PROP_TIME_NOW: while(true){
        uint32_t secs = _iface->peek32(UE_REG_RB_TIME_NOW_SECS);
        uint32_t ticks = _iface->peek32(UE_REG_RB_TIME_NOW_TICKS);
        if (secs != _iface->peek32(UE_REG_RB_TIME_NOW_SECS)) continue;
        val = time_spec_t(secs, ticks, _clock_ctrl->get_fpga_clock_rate());
        return;
    }

    case MBOARD_PROP_TIME_PPS: while(true){
        uint32_t secs = _iface->peek32(UE_REG_RB_TIME_PPS_SECS);
        uint32_t ticks = _iface->peek32(UE_REG_RB_TIME_PPS_TICKS);
        if (secs != _iface->peek32(UE_REG_RB_TIME_PPS_SECS)) continue;
        val = time_spec_t(secs, ticks, _clock_ctrl->get_fpga_clock_rate());
        return;
    }

    case MBOARD_PROP_CLOCK_RATE:
        val = _clock_ctrl->get_fpga_clock_rate();
        return;

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

/***********************************************************************
 * Mboard Set
 **********************************************************************/
void usrp_e100_impl::mboard_set(const wax::obj &key, const wax::obj &val){
    //handle the get request conditioned on the key
    switch(key.as<mboard_prop_t>()){

    case MBOARD_PROP_STREAM_CMD:
        issue_stream_cmd(val.as<stream_cmd_t>());
        return;

    case MBOARD_PROP_TIME_NOW:
    case MBOARD_PROP_TIME_PPS:{
            time_spec_t time_spec = val.as<time_spec_t>();
            _iface->poke32(UE_REG_TIME64_TICKS, time_spec.get_tick_count(_clock_ctrl->get_fpga_clock_rate()));
            boost::uint32_t imm_flags = (key.as<mboard_prop_t>() == MBOARD_PROP_TIME_NOW)? 1 : 0;
            _iface->poke32(UE_REG_TIME64_IMM, imm_flags);
            _iface->poke32(UE_REG_TIME64_SECS, time_spec.get_full_secs());
        }
        return;

    case MBOARD_PROP_RX_SUBDEV_SPEC:
        _rx_subdev_spec = val.as<subdev_spec_t>();
        verify_rx_subdev_spec(_rx_subdev_spec, _mboard_proxy->get_link());
        //sanity check
        UHD_ASSERT_THROW(_rx_subdev_spec.size() == 1);
        //set the mux
        _iface->poke32(UE_REG_DSP_RX_MUX, dsp_type1::calc_rx_mux_word(
            _dboard_manager->get_rx_subdev(_rx_subdev_spec.front().sd_name)[SUBDEV_PROP_CONNECTION].as<subdev_conn_t>()
        ));
        return;

    case MBOARD_PROP_TX_SUBDEV_SPEC:
        _tx_subdev_spec = val.as<subdev_spec_t>();
        verify_tx_subdev_spec(_tx_subdev_spec, _mboard_proxy->get_link());
        //sanity check
        UHD_ASSERT_THROW(_tx_subdev_spec.size() == 1);
        //set the mux
        _iface->poke32(UE_REG_DSP_TX_MUX, dsp_type1::calc_tx_mux_word(
            _dboard_manager->get_tx_subdev(_tx_subdev_spec.front().sd_name)[SUBDEV_PROP_CONNECTION].as<subdev_conn_t>()
        ));
        return;

    case MBOARD_PROP_EEPROM_MAP:
        // Step1: commit the map, writing only those values set.
        // Step2: readback the entire eeprom map into the iface.
        val.as<mboard_eeprom_t>().commit(_iface->get_i2c_dev_iface(), mboard_eeprom_t::MAP_E100);
        _iface->mb_eeprom = mboard_eeprom_t(_iface->get_i2c_dev_iface(), mboard_eeprom_t::MAP_E100);
        return;
        
    case MBOARD_PROP_CLOCK_CONFIG:
        _clock_config = val.as<clock_config_t>();
        update_clock_config();
        return;

    case MBOARD_PROP_CLOCK_RATE:
        _clock_ctrl->set_fpga_clock_rate(val.as<double>());
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}
