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
#include "fpga_regs_standard.h"
#include "fpga_regs_common.h"
#include "usrp_i2c_addr.h"
#include <uhd/usrp/misc_utils.hpp>
#include <uhd/usrp/mboard_props.hpp>
#include <uhd/usrp/dboard_props.hpp>
#include <uhd/usrp/subdev_props.hpp>
#include <uhd/utils/warning.hpp>
#include <uhd/utils/assert.hpp>
#include <uhd/utils/images.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/foreach.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <iostream>

using namespace uhd;
using namespace uhd::usrp;

static const bool usrp1_mboard_verbose = false;

/***********************************************************************
 * Calculate the RX mux value:
 *    The I and Q mux values are intentionally reversed to flip I and Q
 *    to account for the reversal in the type conversion routines.
 **********************************************************************/
static int calc_rx_mux_pair(int adc_for_i, int adc_for_q){
    return (adc_for_i << 2) | (adc_for_q << 0); //shift reversal here
}

/*!
 *    3                   2                   1                   0
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-----------------------+-------+-------+-------+-------+-+-----+
 * |      must be zero     | Q3| I3| Q2| I2| Q1| I1| Q0| I0|Z| NCH |
 * +-----------------------+-------+-------+-------+-------+-+-----+
 */
static boost::uint32_t calc_rx_mux(
    const subdev_spec_t &subdev_spec, wax::obj mboard
){
    //create look-up-table for mapping dboard name and connection type to ADC flags
    static const int ADC0 = 0, ADC1 = 1, ADC2 = 2, ADC3 = 3;
    static const uhd::dict<std::string, uhd::dict<subdev_conn_t, int> > name_to_conn_to_flag = boost::assign::map_list_of
        ("A", boost::assign::map_list_of
            (SUBDEV_CONN_COMPLEX_IQ, calc_rx_mux_pair(ADC0, ADC1)) //I and Q
            (SUBDEV_CONN_COMPLEX_QI, calc_rx_mux_pair(ADC1, ADC0)) //I and Q
            (SUBDEV_CONN_REAL_I,     calc_rx_mux_pair(ADC0, ADC0)) //I and Q (Q identical but ignored Z=1)
            (SUBDEV_CONN_REAL_Q,     calc_rx_mux_pair(ADC1, ADC1)) //I and Q (Q identical but ignored Z=1)
        )
        ("B", boost::assign::map_list_of
            (SUBDEV_CONN_COMPLEX_IQ, calc_rx_mux_pair(ADC2, ADC3)) //I and Q
            (SUBDEV_CONN_COMPLEX_QI, calc_rx_mux_pair(ADC3, ADC2)) //I and Q
            (SUBDEV_CONN_REAL_I,     calc_rx_mux_pair(ADC2, ADC2)) //I and Q (Q identical but ignored Z=1)
            (SUBDEV_CONN_REAL_Q,     calc_rx_mux_pair(ADC3, ADC3)) //I and Q (Q identical but ignored Z=1)
        )
    ;

    //extract the number of channels
    size_t nchan = subdev_spec.size();

    //calculate the channel flags
    int channel_flags = 0;
    size_t num_reals = 0, num_quads = 0;
    BOOST_FOREACH(const subdev_spec_pair_t &pair, subdev_spec){
        wax::obj dboard = mboard[named_prop_t(MBOARD_PROP_RX_DBOARD, pair.db_name)];
        wax::obj subdev = dboard[named_prop_t(DBOARD_PROP_SUBDEV, pair.sd_name)];
        subdev_conn_t conn = subdev[SUBDEV_PROP_CONNECTION].as<subdev_conn_t>();
        switch(conn){
        case SUBDEV_CONN_COMPLEX_IQ:
        case SUBDEV_CONN_COMPLEX_QI: num_quads++; break;
        case SUBDEV_CONN_REAL_I:
        case SUBDEV_CONN_REAL_Q:     num_reals++; break;
        }
        channel_flags = (channel_flags << 4) | name_to_conn_to_flag[pair.db_name][conn];
    }

    //calculate Z:
    //    for all real sources: Z = 1
    //    for all quadrature sources: Z = 0
    //    for mixed sources: warning + Z = 0
    int Z = (num_quads > 0)? 0 : 1;
    if (num_quads != 0 and num_reals != 0) uhd::print_warning(
        "Mixing real and quadrature rx subdevices is not supported.\n"
        "The Q input to the real source(s) will be non-zero.\n"
    );

    //calculate the rx mux value
    return ((channel_flags & 0xffff) << 4) | ((Z & 0x1) << 3) | ((nchan & 0x7) << 0);
}

/***********************************************************************
 * Calculate the TX mux value:
 *    The I and Q mux values are intentionally reversed to flip I and Q
 *    to account for the reversal in the type conversion routines.
 **********************************************************************/
static int calc_tx_mux_pair(int chn_for_i, int chn_for_q){
    return (chn_for_i << 4) | (chn_for_q << 0); //shift reversal here
}

/*!
 *    3                   2                   1                   0
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-----------------------+-------+-------+-------+-------+-+-----+
 * |                       | DAC1Q | DAC1I | DAC0Q | DAC0I |0| NCH |
 * +-----------------------------------------------+-------+-+-----+
 */
static boost::uint32_t calc_tx_mux(
    const subdev_spec_t &subdev_spec, wax::obj mboard
){
    //create look-up-table for mapping channel number and connection type to flags
    static const int ENB = 1 << 3, CHAN_I0 = 0, CHAN_Q0 = 1, CHAN_I1 = 2, CHAN_Q1 = 3;
    static const uhd::dict<size_t, uhd::dict<subdev_conn_t, int> > chan_to_conn_to_flag = boost::assign::map_list_of
        (0, boost::assign::map_list_of
            (SUBDEV_CONN_COMPLEX_IQ, calc_tx_mux_pair(CHAN_I0 | ENB, CHAN_Q0 | ENB))
            (SUBDEV_CONN_COMPLEX_QI, calc_tx_mux_pair(CHAN_Q0 | ENB, CHAN_I0 | ENB))
            (SUBDEV_CONN_REAL_I,     calc_tx_mux_pair(CHAN_I0 | ENB, 0            ))
            (SUBDEV_CONN_REAL_Q,     calc_tx_mux_pair(0,             CHAN_I0 | ENB))
        )
        (1, boost::assign::map_list_of
            (SUBDEV_CONN_COMPLEX_IQ, calc_tx_mux_pair(CHAN_I1 | ENB, CHAN_Q1 | ENB))
            (SUBDEV_CONN_COMPLEX_QI, calc_tx_mux_pair(CHAN_Q1 | ENB, CHAN_I1 | ENB))
            (SUBDEV_CONN_REAL_I,     calc_tx_mux_pair(CHAN_I1 | ENB, 0            ))
            (SUBDEV_CONN_REAL_Q,     calc_tx_mux_pair(0,             CHAN_I1 | ENB))
        )
    ;

    //extract the number of channels
    size_t nchan = subdev_spec.size();

    //calculate the channel flags
    int channel_flags = 0, chan = 0;
    BOOST_FOREACH(const subdev_spec_pair_t &pair, subdev_spec){
        wax::obj dboard = mboard[named_prop_t(MBOARD_PROP_TX_DBOARD, pair.db_name)];
        wax::obj subdev = dboard[named_prop_t(DBOARD_PROP_SUBDEV, pair.sd_name)];
        subdev_conn_t conn = subdev[SUBDEV_PROP_CONNECTION].as<subdev_conn_t>();

        //combine the channel flags: shift for slot A vs B
        if (pair.db_name == "A") channel_flags |= chan_to_conn_to_flag[chan][conn] << 0;
        if (pair.db_name == "B") channel_flags |= chan_to_conn_to_flag[chan][conn] << 8;

        //increment for the next channel
        chan++;
    }

    //calculate the tx mux value
    return ((channel_flags & 0xffff) << 4) | ((nchan & 0x7) << 0);
}

/*!
 * Capabilities Register
 *
 *    3                   2                   1                   0
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-----------------------------------------------+-+-----+-+-----+
 * |               Reserved                        |T|DUCs |R|DDCs |
 * +-----------------------------------------------+-+-----+-+-----+
 */
size_t usrp1_impl::get_num_ddcs(void){
    boost::uint32_t regval = _iface->peek32(FR_RB_CAPS);
    return (regval >> 0) & 0x0007;
}

size_t usrp1_impl::get_num_ducs(void){
    boost::uint32_t regval = _iface->peek32(FR_RB_CAPS);
    return (regval >> 4) & 0x0007;
}

bool usrp1_impl::has_rx_halfband(void){
    boost::uint32_t regval = _iface->peek32(FR_RB_CAPS);
    return (regval >> 3) & 0x0001;
}

bool usrp1_impl::has_tx_halfband(void){
    boost::uint32_t regval = _iface->peek32(FR_RB_CAPS);
    return (regval >> 7) & 0x0001;
}

/***********************************************************************
 * Mboard Initialization
 **********************************************************************/
void usrp1_impl::mboard_init(void)
{
    _mboard_proxy = wax_obj_proxy::make(
                     boost::bind(&usrp1_impl::mboard_get, this, _1, _2),
                     boost::bind(&usrp1_impl::mboard_set, this, _1, _2));

    // Normal mode with no loopback or Rx counting
    _iface->poke32(FR_MODE, 0x00000000);
    _iface->poke32(FR_DEBUG_EN, 0x00000000);
    _iface->poke32(FR_RX_SAMPLE_RATE_DIV, 0x00000001);
    _iface->poke32(FR_TX_SAMPLE_RATE_DIV, 0x00000003);
    _iface->poke32(FR_DC_OFFSET_CL_EN, 0x0000000f);

    // Reset offset correction registers
    _iface->poke32(FR_ADC_OFFSET_0, 0x00000000);
    _iface->poke32(FR_ADC_OFFSET_1, 0x00000000);
    _iface->poke32(FR_ADC_OFFSET_2, 0x00000000);
    _iface->poke32(FR_ADC_OFFSET_3, 0x00000000);

    // Set default for RX format to 16-bit I&Q and no half-band filter bypass
    _iface->poke32(FR_RX_FORMAT, 0x00000300);

    // Set default for TX format to 16-bit I&Q
    _iface->poke32(FR_TX_FORMAT, 0x00000000);

    if (usrp1_mboard_verbose){
        std::cout << "USRP1 Capabilities" << std::endl;
        std::cout << "    number of duc's: " << get_num_ddcs() << std::endl;
        std::cout << "    number of ddc's: " << get_num_ducs() << std::endl;
        std::cout << "    rx halfband:     " << has_rx_halfband() << std::endl;
        std::cout << "    tx halfband:     " << has_tx_halfband() << std::endl;
    }
}

void usrp1_impl::issue_stream_cmd(const stream_cmd_t &stream_cmd)
{
    switch(stream_cmd.stream_mode){
    case stream_cmd_t::STREAM_MODE_START_CONTINUOUS:
        return _iface->write_firmware_cmd(VRQ_FPGA_SET_RX_ENABLE, true, 0, 0, 0);

    case stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS:
        return _iface->write_firmware_cmd(VRQ_FPGA_SET_RX_ENABLE, false, 0, 0, 0);

    default: throw std::runtime_error("unsupported stream command type for USRP1");
    }
}

/***********************************************************************
 * Mboard Get
 **********************************************************************/
static prop_names_t dboard_names = boost::assign::list_of("A")("B");

void usrp1_impl::mboard_get(const wax::obj &key_, wax::obj &val)
{
    named_prop_t key = named_prop_t::extract(key_);

    if(key_.type() == typeid(std::string)) {
      if(key.as<std::string>() == "serial") {
        uhd::byte_vector_t buf;
        buf.insert(buf.begin(), 248);
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        _iface->write_i2c(I2C_DEV_EEPROM, buf);
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        buf = _iface->read_i2c(I2C_DEV_EEPROM, 8);
        val = std::string(buf.begin(), buf.end());
      }

      return;
   	}

    //handle the get request conditioned on the key
    switch(key.as<mboard_prop_t>()){
    case MBOARD_PROP_NAME:
        val = std::string("usrp1 mboard - " + (*_mboard_proxy)[std::string("serial")].as<std::string>());
        return;

    case MBOARD_PROP_OTHERS:
        val = prop_names_t();
        return;

    case MBOARD_PROP_RX_DBOARD:
        uhd::assert_has(dboard_names, key.name, "dboard name");
        if (key.name == "A") val = _rx_dboard_proxies[DBOARD_SLOT_A]->get_link();
        if (key.name == "B") val = _rx_dboard_proxies[DBOARD_SLOT_B]->get_link();
        return;

    case MBOARD_PROP_RX_DBOARD_NAMES:
        val = dboard_names;
        return;

    case MBOARD_PROP_TX_DBOARD:
        uhd::assert_has(dboard_names, key.name, "dboard name");
        if (key.name == "A") val = _tx_dboard_proxies[DBOARD_SLOT_A]->get_link();
        if (key.name == "B") val = _tx_dboard_proxies[DBOARD_SLOT_B]->get_link();
        return;

    case MBOARD_PROP_TX_DBOARD_NAMES:
        val = dboard_names;
        return;

    case MBOARD_PROP_RX_DSP:
        UHD_ASSERT_THROW(key.name == "");
        val = _rx_dsp_proxy->get_link();
        return;

    case MBOARD_PROP_RX_DSP_NAMES:
        val = prop_names_t(1, "");
        return;

    case MBOARD_PROP_TX_DSP:
        UHD_ASSERT_THROW(key.name == "");
        val = _tx_dsp_proxy->get_link();
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
void usrp1_impl::mboard_set(const wax::obj &key, const wax::obj &val)
{
    if(key.type() == typeid(std::string)) {
      if(key.as<std::string>() == "load_eeprom") {
        std::string usrp1_eeprom_image = val.as<std::string>();
        std::cout << "USRP1 EEPROM image: " << usrp1_eeprom_image << std::endl;
        _ctrl_transport->usrp_load_eeprom(val.as<std::string>());
      }

      if(key.as<std::string>() == "serial") {
        std::string sernum = val.as<std::string>();
        uhd::byte_vector_t buf(sernum.begin(), sernum.end());
        buf.insert(buf.begin(), 248);
        _iface->write_i2c(I2C_DEV_EEPROM, buf);
      }

      return;
   	}

    //handle the get request conditioned on the key
    switch(key.as<mboard_prop_t>()){

    case MBOARD_PROP_STREAM_CMD:
        issue_stream_cmd(val.as<stream_cmd_t>());
        return;

    case MBOARD_PROP_RX_SUBDEV_SPEC:
        _rx_subdev_spec = val.as<subdev_spec_t>();
        if (_rx_subdev_spec.size() > this->get_num_ddcs()){
            throw std::runtime_error(str(boost::format(
                "USRP1 suports up to %u RX channels.\n"
                "However, this RX subdev spec requires %u channels\n"
            ) % this->get_num_ddcs() % _rx_subdev_spec.size()));
        }
        verify_rx_subdev_spec(_rx_subdev_spec, _mboard_proxy->get_link());
        //set the mux and set the number of rx channels
        _iface->poke32(FR_RX_MUX, calc_rx_mux(_rx_subdev_spec, _mboard_proxy->get_link()));
        return;

    case MBOARD_PROP_TX_SUBDEV_SPEC:
        _tx_subdev_spec = val.as<subdev_spec_t>();
        if (_tx_subdev_spec.size() > this->get_num_ducs()){
            throw std::runtime_error(str(boost::format(
                "USRP1 suports up to %u TX channels.\n"
                "However, this TX subdev spec requires %u channels\n"
            ) % this->get_num_ducs() % _tx_subdev_spec.size()));
        }
        verify_tx_subdev_spec(_tx_subdev_spec, _mboard_proxy->get_link());
        //set the mux and set the number of tx channels
        _iface->poke32(FR_TX_MUX, calc_tx_mux(_tx_subdev_spec, _mboard_proxy->get_link()));
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}
