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

#include <uhd/config.hpp>
#include <uhd/exception.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/utils/algorithm.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>
#include <utility>
#include <vector>
#include <string>

#ifndef INCLUDED_USRP1_CALC_MUX_HPP
#define INCLUDED_USRP1_CALC_MUX_HPP

//db_name, conn_type for the mux calculations below...
typedef std::pair<std::string, std::string> mapping_pair_t;

/***********************************************************************
 * Calculate the RX mux value:
 *    The I and Q mux values are intentionally reversed to flip I and Q
 *    to account for the reversal in the type conversion routines.
 **********************************************************************/
static int calc_rx_mux_pair(int adc_for_i, int adc_for_q){
    return (adc_for_i << 0) | (adc_for_q << 2);
}

/*!
 *    3                   2                   1                   0
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-----------------------+-------+-------+-------+-------+-+-----+
 * |      must be zero     | Q3| I3| Q2| I2| Q1| I1| Q0| I0|Z| NCH |
 * +-----------------------+-------+-------+-------+-------+-+-----+
 */
static boost::uint32_t calc_rx_mux(const std::vector<mapping_pair_t> &mapping){
    //create look-up-table for mapping dboard name and connection type to ADC flags
    static const int ADC0 = 0, ADC1 = 1, ADC2 = 2, ADC3 = 3;
    static const uhd::dict<std::string, uhd::dict<std::string, int> > name_to_conn_to_flag = boost::assign::map_list_of
        ("A", boost::assign::map_list_of
            ("IQ", calc_rx_mux_pair(ADC0, ADC1)) //I and Q
            ("QI", calc_rx_mux_pair(ADC1, ADC0)) //I and Q
            ("I",  calc_rx_mux_pair(ADC0, ADC0)) //I and Q (Q identical but ignored Z=1)
            ("Q",  calc_rx_mux_pair(ADC1, ADC1)) //I and Q (Q identical but ignored Z=1)
        )
        ("B", boost::assign::map_list_of
            ("IQ", calc_rx_mux_pair(ADC2, ADC3)) //I and Q
            ("QI", calc_rx_mux_pair(ADC3, ADC2)) //I and Q
            ("I",  calc_rx_mux_pair(ADC2, ADC2)) //I and Q (Q identical but ignored Z=1)
            ("Q",  calc_rx_mux_pair(ADC3, ADC3)) //I and Q (Q identical but ignored Z=1)
        )
    ;

    //extract the number of channels
    const size_t nchan = mapping.size();

    //calculate the channel flags
    int channel_flags = 0;
    size_t num_reals = 0, num_quads = 0;
    BOOST_FOREACH(const mapping_pair_t &pair, uhd::reversed(mapping)){
        const std::string name = pair.first, conn = pair.second;
        if (conn == "IQ" or conn == "QI") num_quads++;
        if (conn == "I" or conn == "Q") num_reals++;
        channel_flags = (channel_flags << 4) | name_to_conn_to_flag[name][conn];
    }

    //calculate Z:
    //    for all real sources: Z = 1
    //    for all quadrature sources: Z = 0
    //    for mixed sources: warning + Z = 0
    int Z = (num_quads > 0)? 0 : 1;
    if (num_quads != 0 and num_reals != 0) UHD_MSG(warning) << boost::format(
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
    return (chn_for_i << 0) | (chn_for_q << 4);
}

/*!
 *    3                   2                   1                   0
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-----------------------+-------+-------+-------+-------+-+-----+
 * |                       | DAC1Q | DAC1I | DAC0Q | DAC0I |0| NCH |
 * +-----------------------------------------------+-------+-+-----+
 */
static boost::uint32_t calc_tx_mux(const std::vector<mapping_pair_t> &mapping){
    //create look-up-table for mapping channel number and connection type to flags
    static const int ENB = 1 << 3, CHAN_I0 = 0, CHAN_Q0 = 1, CHAN_I1 = 2, CHAN_Q1 = 3;
    static const uhd::dict<size_t, uhd::dict<std::string, int> > chan_to_conn_to_flag = boost::assign::map_list_of
        (0, boost::assign::map_list_of
            ("IQ", calc_tx_mux_pair(CHAN_I0 | ENB, CHAN_Q0 | ENB))
            ("QI", calc_tx_mux_pair(CHAN_Q0 | ENB, CHAN_I0 | ENB))
            ("I",  calc_tx_mux_pair(CHAN_I0 | ENB, 0            ))
            ("Q",  calc_tx_mux_pair(0,             CHAN_I0 | ENB))
        )
        (1, boost::assign::map_list_of
            ("IQ", calc_tx_mux_pair(CHAN_I1 | ENB, CHAN_Q1 | ENB))
            ("QI", calc_tx_mux_pair(CHAN_Q1 | ENB, CHAN_I1 | ENB))
            ("I",  calc_tx_mux_pair(CHAN_I1 | ENB, 0            ))
            ("Q",  calc_tx_mux_pair(0,             CHAN_I1 | ENB))
        )
    ;

    //extract the number of channels
    size_t nchan = mapping.size();

    //calculate the channel flags
    int channel_flags = 0, chan = 0;
    uhd::dict<std::string, int> slot_to_chan_count = boost::assign::map_list_of("A", 0)("B", 0);
    BOOST_FOREACH(const mapping_pair_t &pair, mapping){
        const std::string name = pair.first, conn = pair.second;

        //combine the channel flags: shift for slot A vs B
        if (name == "A") channel_flags |= chan_to_conn_to_flag[chan][conn] << 0;
        if (name == "B") channel_flags |= chan_to_conn_to_flag[chan][conn] << 8;

        //sanity check, only 1 channel per slot
        slot_to_chan_count[name]++;
        if (slot_to_chan_count[name] > 1) throw uhd::value_error(
            "cannot assign dboard slot to multiple channels: " + name
        );

        //increment for the next channel
        chan++;
    }

    //calculate the tx mux value
    return ((channel_flags & 0xffff) << 4) | ((nchan & 0x7) << 0);
}

#endif /* INCLUDED_USRP1_CALC_MUX_HPP */
