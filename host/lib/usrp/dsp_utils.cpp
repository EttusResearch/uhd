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

#include <uhd/usrp/dsp_utils.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/exception.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/math/special_functions/round.hpp>
#include <boost/math/special_functions/sign.hpp>
#include <algorithm>
#include <cmath>

using namespace uhd;
using namespace uhd::usrp;

template <class T> T ceil_log2(T num){
    return std::ceil(std::log(num)/std::log(T(2)));
}

/*!
 *     3                   2                   1                   0
 *   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-------------------------------+-------+-------+-------+-------+
 *  |                                               | DDC0Q | DDC0I |
 *  +-------------------------------+-------+-------+-------+-------+
 */
boost::uint32_t dsp_type1::calc_rx_mux_word(subdev_conn_t subdev_conn){
    switch(subdev_conn){
    case SUBDEV_CONN_COMPLEX_IQ: return (0x1 << 4) | (0x0 << 0); //DDC0Q=ADC0Q, DDC0I=ADC0I
    case SUBDEV_CONN_COMPLEX_QI: return (0x0 << 4) | (0x1 << 0); //DDC0Q=ADC0I, DDC0I=ADC0Q
    case SUBDEV_CONN_REAL_I:     return (0xf << 4) | (0x0 << 0); //DDC0Q=ZERO,  DDC0I=ADC0I
    case SUBDEV_CONN_REAL_Q:     return (0xf << 4) | (0x1 << 0); //DDC0Q=ZERO,  DDC0I=ADC0Q
    default:                     UHD_THROW_INVALID_CODE_PATH();
    }
}

/*!
 *     3                   2                   1                   0
 *   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-------------------------------+-------+-------+-------+-------+
 *  |                                               | DAC0Q | DAC0I |
 *  +-------------------------------+-------+-------+-------+-------+
 */
boost::uint32_t dsp_type1::calc_tx_mux_word(subdev_conn_t subdev_conn){
    switch(subdev_conn){
    case SUBDEV_CONN_COMPLEX_IQ: return (0x1 << 4) | (0x0 << 0); //DAC0Q=DUC0Q, DAC0I=DUC0I
    case SUBDEV_CONN_COMPLEX_QI: return (0x0 << 4) | (0x1 << 0); //DAC0Q=DUC0I, DAC0I=DUC0Q
    case SUBDEV_CONN_REAL_I:     return (0xf << 4) | (0x0 << 0); //DAC0Q=ZERO,  DAC0I=DUC0I
    case SUBDEV_CONN_REAL_Q:     return (0x0 << 4) | (0xf << 0); //DAC0Q=DUC0I, DAC0I=ZERO
    default:                     UHD_THROW_INVALID_CODE_PATH();
    }
}

boost::uint32_t dsp_type1::calc_cordic_word_and_update(
    double &freq, double codec_rate
){
    //correct for outside of rate (wrap around)
    freq = std::fmod(freq, codec_rate);
    if (std::abs(freq) > codec_rate/2.0)
        freq -= boost::math::sign(freq)*codec_rate;

    //calculate the freq register word (signed)
    UHD_ASSERT_THROW(std::abs(freq) <= codec_rate/2.0);
    static const double scale_factor = std::pow(2.0, 32);
    boost::int32_t freq_word = boost::int32_t(boost::math::round((freq / codec_rate) * scale_factor));

    //update the actual frequency
    freq = (double(freq_word) / scale_factor) * codec_rate;

    return boost::uint32_t(freq_word);
}

boost::uint32_t dsp_type1::calc_cic_filter_word(unsigned rate){
    int hb0 = 0, hb1 = 0;
    if (not (rate & 0x1)){
        hb0 = 1;
        rate /= 2;
    }
    if (not (rate & 0x1)){
        hb1 = 1;
        rate /= 2;
    }
    return (hb1 << 9) | (hb0 << 8) | (rate & 0xff);
}

boost::uint32_t dsp_type1::calc_iq_scale_word(
    boost::int16_t i, boost::int16_t q
){
    return (boost::uint32_t(i) << 16) | (boost::uint32_t(q) << 0);
}

boost::uint32_t dsp_type1::calc_iq_scale_word(unsigned rate){
    // Calculate CIC interpolation (i.e., without halfband interpolators)
    unsigned tmp_rate = calc_cic_filter_word(rate) & 0xff;

    // Calculate closest multiplier constant to reverse gain absent scale multipliers
    double rate_cubed = std::pow(double(tmp_rate), 3);
    boost::int16_t scale = boost::math::iround((4096*std::pow(2, ceil_log2(rate_cubed)))/(1.65*rate_cubed));
    return calc_iq_scale_word(scale, scale);
}

boost::uint32_t dsp_type1::calc_stream_cmd_word(const stream_cmd_t &stream_cmd){
    UHD_ASSERT_THROW(stream_cmd.num_samps <= 0x3fffffff);

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

    //calculate the word from flags and length
    boost::uint32_t word = 0;
    word |= boost::uint32_t((stream_cmd.stream_now)? 1 : 0) << 31;
    word |= boost::uint32_t((inst_chain)?            1 : 0) << 30;
    word |= boost::uint32_t((inst_reload)?           1 : 0) << 29;
    word |= (inst_samps)? stream_cmd.num_samps : ((inst_chain)? 1 : 0);
    return word;
}
