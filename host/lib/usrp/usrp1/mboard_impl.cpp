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

#include "usrp1_impl.hpp"
#include "usrp_commands.h"
#include <uhd/utils/assert.hpp>
#include <uhd/usrp/mboard_props.hpp>
#include <boost/bind.hpp>
#include <iostream>

using namespace uhd;
using namespace uhd::usrp;

/***********************************************************************
 * Mboard Initialization
 **********************************************************************/
void usrp1_impl::mboard_init(void)
{
    _mboard_proxy = wax_obj_proxy::make(
                     boost::bind(&usrp1_impl::mboard_get, this, _1, _2),
                     boost::bind(&usrp1_impl::mboard_set, this, _1, _2));

    /*
     * Basic initialization 
     */
    _iface->poke32( 13, 0x00000000); //FR_MODE
    _iface->poke32( 14, 0x00000000); //FR_DEBUG_EN
    _iface->poke32(  1, 0x00000001); //FR_RX_SAMPLE_RATE_DEV
    _iface->poke32(  0, 0x00000003); //FR_TX_SAMPLE_RATE_DEV
    _iface->poke32( 15, 0x0000000f); //FR_DC_OFFSET_CL_EN

    /*
     * Reset codecs 
     */
    _iface->poke32( 16, 0x00000000); //FR_ADC_OFFSET_0
    _iface->poke32( 17, 0x00000000); //FR_ADC_OFFSET_1
    _iface->poke32( 18, 0x00000000); //FR_ADC_OFFSET_2
    _iface->poke32( 19, 0x00000000); //FR_ADC_OFFSET_3

    /*
     * Reset GPIO masks 
     */
    _iface->poke32(  6, 0xffff0000); //FR_OE_1
    _iface->poke32( 10, 0xffff0000); //FR_IO_1
    _iface->poke32(  8, 0xffff0000); //FR_OE_3
    _iface->poke32( 12, 0xffff0000); //FR_IO_3

    /*
     * Disable ATR masks and reset state registers
     */
    _iface->poke32( 23, 0x00000000); //FR_ATR_MASK_1
    _iface->poke32( 24, 0x00000000); //FR_ATR_TXVAL_1
    _iface->poke32( 25, 0x00000000); //FR_ATR_RXVAL_1
    _iface->poke32( 29, 0x00000000); //FR_ATR_MASK_3
    _iface->poke32( 30, 0x00000000); //FR_ATR_TXVAL_3
    _iface->poke32( 31, 0x00000000); //FR_ATR_RXVAL_3

    /*
     * Set defaults for RX format, decimation, and mux 
     */
    _iface->poke32( 49, 0x00000300); //FR_RX_FORMAT
    _iface->poke32( 38, 0x000e4e41); //FR_RX_MUX

    /*
     * Set defaults for TX format, interpolation, and mux 
     */
    _iface->poke32( 48, 0x00000000); //FR_TX_FORMAT
    _iface->poke32( 39, 0x00000981); //FR_TX_MUX

    /*
     * Reset DDC registers 
     */
    _iface->poke32( 34, 0x00000000); //FR_RX_FREQ_0
    _iface->poke32( 44, 0x00000000); //FR_RX_PHASE_0
    _iface->poke32( 35, 0x00000000); //FR_RX_FREQ_1
    _iface->poke32( 45, 0x00000000); //FR_RX_PHASE_1
    _iface->poke32( 36, 0x00000000); //FR_RX_FREQ_2
    _iface->poke32( 46, 0x00000000); //FR_RX_PHASE_2
    _iface->poke32( 37, 0x00000000); //FR_RX_FREQ_3
    _iface->poke32( 47, 0x00000000); //FR_RX_PHASE_3

}

void usrp1_impl::issue_stream_cmd(const stream_cmd_t &stream_cmd)
{
    if (stream_cmd.stream_mode == stream_cmd_t::STREAM_MODE_START_CONTINUOUS) {
        _iface->write_firmware_cmd(VRQ_FPGA_SET_RX_ENABLE, true, 0, 0, 0);
    }

    if (stream_cmd.stream_mode == stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS) {
        _iface->write_firmware_cmd(VRQ_FPGA_SET_RX_ENABLE, false, 0, 0, 0);
    }
}

/***********************************************************************
 * Mboard Get
 **********************************************************************/
void usrp1_impl::mboard_get(const wax::obj &key_, wax::obj &val)
{
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<mboard_prop_t>()){
    case MBOARD_PROP_NAME:
        val = std::string("usrp1 mboard");
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

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

/***********************************************************************
 * Mboard Set
 **********************************************************************/
void usrp1_impl::mboard_set(const wax::obj &key, const wax::obj &val)
{
    //handle the get request conditioned on the key
    switch(key.as<mboard_prop_t>()){

    case MBOARD_PROP_STREAM_CMD:
        issue_stream_cmd(val.as<stream_cmd_t>());
        return;

    case MBOARD_PROP_TIME_NOW:
    case MBOARD_PROP_TIME_NEXT_PPS:
    default: UHD_THROW_PROP_SET_ERROR();
    }
}
