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

#include "usrp_e_impl.hpp"
#include "usrp_e_regs.hpp"
#include "../dsp_utils.hpp"
#include "../misc_utils.hpp"
#include <uhd/utils/assert.hpp>
#include <uhd/usrp/mboard_props.hpp>
#include <boost/bind.hpp>

using namespace uhd;
using namespace uhd::usrp;

/***********************************************************************
 * Mboard Initialization
 **********************************************************************/
void usrp_e_impl::mboard_init(void){
    _mboard_proxy = wax_obj_proxy::make(
        boost::bind(&usrp_e_impl::mboard_get, this, _1, _2),
        boost::bind(&usrp_e_impl::mboard_set, this, _1, _2)
    );

    //init the clock config
    _clock_config.ref_source = clock_config_t::REF_AUTO;
    _clock_config.pps_source = clock_config_t::PPS_SMA;

    //TODO poke the clock config regs

    //set default subdev specs
    this->mboard_set(MBOARD_PROP_RX_SUBDEV_SPEC, subdev_spec_t());
    this->mboard_set(MBOARD_PROP_TX_SUBDEV_SPEC, subdev_spec_t());
}

/***********************************************************************
 * Mboard Get
 **********************************************************************/
void usrp_e_impl::mboard_get(const wax::obj &key_, wax::obj &val){
    wax::obj key; std::string name;
    boost::tie(key, name) = extract_named_prop(key_);


    //handle the get request conditioned on the key
    switch(key.as<mboard_prop_t>()){
    case MBOARD_PROP_NAME:
        val = std::string("usrp-e mboard");
        return;

    case MBOARD_PROP_OTHERS:
        val = prop_names_t();
        return;

    case MBOARD_PROP_RX_DBOARD:
        UHD_ASSERT_THROW(name == "");
        val = _rx_dboard_proxy->get_link();
        return;

    case MBOARD_PROP_RX_DBOARD_NAMES:
        val = prop_names_t(1, ""); //vector of size 1 with empty string
        return;

    case MBOARD_PROP_TX_DBOARD:
        UHD_ASSERT_THROW(name == "");
        val = _tx_dboard_proxy->get_link();
        return;

    case MBOARD_PROP_TX_DBOARD_NAMES:
        val = prop_names_t(1, ""); //vector of size 1 with empty string
        return;

    case MBOARD_PROP_RX_DSP:
        UHD_ASSERT_THROW(name == "");
        val = _rx_ddc_proxy->get_link();
        return;

    case MBOARD_PROP_RX_DSP_NAMES:
        val = prop_names_t(1, "");
        return;

    case MBOARD_PROP_TX_DSP:
        UHD_ASSERT_THROW(name == "");
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

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

/***********************************************************************
 * Mboard Set
 **********************************************************************/
void usrp_e_impl::mboard_set(const wax::obj &key, const wax::obj &val){
    //handle the get request conditioned on the key
    switch(key.as<mboard_prop_t>()){

    case MBOARD_PROP_STREAM_CMD:
        issue_stream_cmd(val.as<stream_cmd_t>());
        return;

    case MBOARD_PROP_TIME_NOW:
    case MBOARD_PROP_TIME_NEXT_PPS:{
            time_spec_t time_spec = val.as<time_spec_t>();
            _iface->poke32(UE_REG_TIME64_TICKS, time_spec.get_tick_count(MASTER_CLOCK_RATE));
            boost::uint32_t imm_flags = (key.as<mboard_prop_t>() == MBOARD_PROP_TIME_NOW)? 1 : 0;
            _iface->poke32(UE_REG_TIME64_IMM, imm_flags);
            _iface->poke32(UE_REG_TIME64_SECS, time_spec.get_full_secs());
        }
        return;

    case MBOARD_PROP_RX_SUBDEV_SPEC:
        _rx_subdev_spec = val.as<subdev_spec_t>();
        verify_rx_subdev_spec(_rx_subdev_spec, this->get_link());
        //sanity check
        UHD_ASSERT_THROW(_rx_subdev_spec.size() == 1);
        //set the mux
        _iface->poke32(UE_REG_DSP_RX_MUX, dsp_type1::calc_rx_mux_word(
            _dboard_manager->get_rx_subdev(_rx_subdev_spec.front().sd_name)[SUBDEV_PROP_CONNECTION].as<subdev_conn_t>()
        ));
        return;

    case MBOARD_PROP_TX_SUBDEV_SPEC:
        _tx_subdev_spec = val.as<subdev_spec_t>();
        verify_tx_subdev_spec(_tx_subdev_spec, this->get_link());
        //sanity check
        UHD_ASSERT_THROW(_tx_subdev_spec.size() == 1);
        //set the mux
        _iface->poke32(UE_REG_DSP_TX_MUX, dsp_type1::calc_tx_mux_word(
            _dboard_manager->get_tx_subdev(_tx_subdev_spec.front().sd_name)[SUBDEV_PROP_CONNECTION].as<subdev_conn_t>()
        ));
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}
