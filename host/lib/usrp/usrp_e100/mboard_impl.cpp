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
#include <uhd/utils/msg.hpp>
#include <uhd/exception.hpp>
#include <uhd/usrp/dsp_utils.hpp>
#include <uhd/usrp/misc_utils.hpp>
#include <uhd/usrp/mboard_props.hpp>
#include <boost/bind.hpp>

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
    default: throw uhd::value_error("unhandled clock configuration pps polarity");
    }

    //set the pps flags
    _iface->poke32(UE_REG_TIME64_FLAGS, pps_flags);

    //clock source ref 10mhz
    switch(_clock_config.ref_source){
    case clock_config_t::REF_AUTO: _clock_ctrl->use_auto_ref(); break;
    case clock_config_t::REF_INT: _clock_ctrl->use_internal_ref(); break;
    case clock_config_t::REF_SMA: _clock_ctrl->use_auto_ref(); break;
    default: throw uhd::value_error("unhandled clock configuration ref source");
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

    case MBOARD_PROP_TIME_NOW:
    case MBOARD_PROP_TIME_PPS:{
            time_spec_t time_spec = val.as<time_spec_t>();
            _iface->poke32(UE_REG_TIME64_TICKS, time_spec.get_tick_count(_clock_ctrl->get_fpga_clock_rate()));
            boost::uint32_t imm_flags = (key.as<mboard_prop_t>() == MBOARD_PROP_TIME_NOW)? 1 : 0;
            _iface->poke32(UE_REG_TIME64_IMM, imm_flags);
            _iface->poke32(UE_REG_TIME64_SECS, time_spec.get_full_secs());
        }
        return;

    case MBOARD_PROP_RX_SUBDEV_SPEC:{
        _rx_subdev_spec = val.as<subdev_spec_t>();
        verify_rx_subdev_spec(_rx_subdev_spec, _mboard_proxy->get_link());
        //sanity check
        UHD_ASSERT_THROW(_rx_subdev_spec.size() <= E100_NUM_RX_DSPS);

        //determine frontend swap IQ from the first channel
        bool fe_swap_iq = false;
        switch(_dboard_manager->get_rx_subdev(_rx_subdev_spec.at(0).sd_name)[SUBDEV_PROP_CONNECTION].as<subdev_conn_t>()){
        case SUBDEV_CONN_COMPLEX_QI:
        case SUBDEV_CONN_REAL_Q:
            fe_swap_iq = true;
            break;
        default: fe_swap_iq = false;
        }
        _iface->poke32(UE_REG_RX_FE_SWAP_IQ, fe_swap_iq? 1 : 0);

        //set the dsp mux for each channel
        for (size_t i = 0; i < _rx_subdev_spec.size(); i++){
            bool iq_swap = false, real_mode = false;
            switch(_dboard_manager->get_rx_subdev(_rx_subdev_spec.at(i).sd_name)[SUBDEV_PROP_CONNECTION].as<subdev_conn_t>()){
            case SUBDEV_CONN_COMPLEX_IQ:
                iq_swap = fe_swap_iq;
                real_mode = false;
                break;
            case SUBDEV_CONN_COMPLEX_QI:
                iq_swap = not fe_swap_iq;
                real_mode = false;
                break;
            case SUBDEV_CONN_REAL_I:
                iq_swap = fe_swap_iq;
                real_mode = true;
                break;
            case SUBDEV_CONN_REAL_Q:
                iq_swap = not fe_swap_iq;
                real_mode = true;
                break;
            }
            _iface->poke32(UE_REG_DSP_RX_MUX(i),
                (iq_swap?   UE_FLAG_DSP_RX_MUX_SWAP_IQ   : 0) |
                (real_mode? UE_FLAG_DSP_RX_MUX_REAL_MODE : 0)
            );
        }
        this->update_xport_channel_mapping();
    }return;

    case MBOARD_PROP_TX_SUBDEV_SPEC:
        _tx_subdev_spec = val.as<subdev_spec_t>();
        verify_tx_subdev_spec(_tx_subdev_spec, _mboard_proxy->get_link());
        //sanity check
        UHD_ASSERT_THROW(_tx_subdev_spec.size() <= E100_NUM_TX_DSPS);
        //set the mux
        _iface->poke32(UE_REG_TX_FE_MUX, dsp_type1::calc_tx_mux_word(
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
        UHD_MSG(warning)
            << "I see that you are setting the master clock rate from the API.\n"
            << "You may pass this into the device address as master_clock_rate=<rate>.\n"
            << "This way, the clock rate is guaranteed to be initialized first.\n"
            << "See the application notes for USRP-E1XX for further instructions.\n"
        ;
        _clock_ctrl->set_fpga_clock_rate(val.as<double>());
        this->update_xport_channel_mapping();
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}
