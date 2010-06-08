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
#include <uhd/utils/assert.hpp>
#include <boost/cstdint.hpp>
#include <boost/math/special_functions/round.hpp>

namespace uhd{ namespace usrp{

namespace dsp_type1{

    template <class T> T ceil_log2(T num){
        return std::ceil(std::log(num)/std::log(T(2)));
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

} //namespace dsp_type1

}} //namespace

#endif /* INCLUDED_LIBUHD_USRP_DSP_UTILS_HPP */
