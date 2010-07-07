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

#ifndef INCLUDED_LIBUHD_USRP_DSP_UTILS_HPP
#define INCLUDED_LIBUHD_USRP_DSP_UTILS_HPP

#include <uhd/config.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/utils/assert.hpp>
#include <uhd/types/stream_cmd.hpp>
#include <boost/cstdint.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/math/special_functions/round.hpp>

namespace uhd{ namespace usrp{

namespace dsp_type1{

    template <class T> T ceil_log2(T num){
        return std::ceil(std::log(num)/std::log(T(2)));
    }

    /*!
     * Calculate the rx mux word from properties.
     * \param is_quadrature true if the subdev is complex
     * \param is_iq_swapped true if the i and q are reversed
     * \param the 32-bit rx mux control word
     */
    static inline boost::uint32_t calc_rx_mux_word(
        bool is_quadrature,
        bool is_iq_swapped
    ){
        boost::uint32_t rx_mux = 0;
        if (is_quadrature){
            rx_mux = (0x01 << 2) | (0x00 << 0); //Q=ADC1, I=ADC0
        }else{
            rx_mux = (0x11 << 2) | (0x00 << 0); //Q=ZERO, I=ADC0
        }
        if (is_iq_swapped){
            rx_mux = (rx_mux << 2) | (rx_mux >> 2);
        }
        return rx_mux;
    }

    /*!
     * Calculate the tx mux word from properties.
     * \param is_iq_swapped true if the i and q are reversed
     * \param the 32-bit tx mux control word
     */
    static inline boost::uint32_t calc_tx_mux_word(bool is_iq_swapped){
        boost::uint32_t tx_mux = 0x10;
        if (is_iq_swapped){
            tx_mux = (tx_mux << 4) | (tx_mux >> 4);
        }
        return tx_mux;
    }

    /*!
     * Calculate the cordic word from the frequency and clock rate.
     * The frequency will be set to the actual (possible) frequency.
     *
     * \param freq the requested frequency in Hz
     * \param codec_rate the dsp codec rate in Hz
     * \param the 32-bit cordic control word
     */
    static inline boost::uint32_t calc_cordic_word_and_update(
        double &freq,
        double codec_rate
    ){
        UHD_ASSERT_THROW(std::abs(freq) < codec_rate/2.0);
        static const double scale_factor = std::pow(2.0, 32);

        //calculate the freq register word
        boost::uint32_t freq_word = boost::math::iround((freq / codec_rate) * scale_factor);

        //update the actual frequency
        freq = (double(freq_word) / scale_factor) * codec_rate;

        return freq_word;
    }

    /*!
     * Calculate the CIC filter word from the rate.
     * Check if requested decim/interp rate is:
     *      multiple of 4, enable two halfband filters
     *      multiple of 2, enable one halfband filter
     *      handle remainder in CIC
     *
     * \param rate the requested rate in Sps
     * \return the 32-bit cic filter control word
     */
    template <typename dsp_rate_type>
    static inline boost::uint32_t calc_cic_filter_word(dsp_rate_type rate){
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

    /*!
     * Calculate the IQ scale factor word from I and Q components.
     * \param i the I component of the scalar
     * \param q the Q component of the scalar
     * \return the 32-bit scale factor control word
     */
    static inline boost::uint32_t calc_iq_scale_word(
        boost::int16_t i, boost::int16_t q
    ){
        return (boost::uint32_t(i) << 16) | (boost::uint32_t(q) << 0);
    }

    /*!
     * Calculate the IQ scale factor word from the rate.
     * \param rate the requested rate in Sps
     * \return the 32-bit scale factor control word
     */
    template <typename dsp_rate_type>
    static inline boost::uint32_t calc_iq_scale_word(dsp_rate_type rate){
        // Calculate CIC interpolation (i.e., without halfband interpolators)
        dsp_rate_type tmp_rate = calc_cic_filter_word(rate) & 0xff;

        // Calculate closest multiplier constant to reverse gain absent scale multipliers
        double rate_cubed = std::pow(double(tmp_rate), 3);
        boost::int16_t scale = boost::math::iround((4096*std::pow(2, ceil_log2(rate_cubed)))/(1.65*rate_cubed));
        return calc_iq_scale_word(scale, scale);
    }

    /*!
     * Calculate the stream command word from the stream command struct.
     * \param stream_cmd the requested stream command with mode, flags, timestamp
     * \param num_samps_continuous number of samples to request in continuous mode
     * \return the 32-bit stream command word
     */
    static inline boost::uint32_t calc_stream_cmd_word(
        const stream_cmd_t &stream_cmd, size_t num_samps_continuous
    ){
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
        word |= (inst_samps)? stream_cmd.num_samps : ((inst_chain)? num_samps_continuous : 1);
        return word;
    }

} //namespace dsp_type1

}} //namespace

#endif /* INCLUDED_LIBUHD_USRP_DSP_UTILS_HPP */
