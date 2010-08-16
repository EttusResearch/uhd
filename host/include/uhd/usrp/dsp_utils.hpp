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

#ifndef INCLUDED_UHD_USRP_DSP_UTILS_HPP
#define INCLUDED_UHD_USRP_DSP_UTILS_HPP

#include <uhd/config.hpp>
#include <uhd/types/stream_cmd.hpp>
#include <uhd/usrp/subdev_props.hpp>
#include <boost/cstdint.hpp>

namespace uhd{ namespace usrp{

namespace dsp_type1{

    /*!
     * Calculate the rx mux word from properties.
     * \param subdev_conn the subdev connection type
     * \return the 32-bit rx mux control word
     */
    UHD_API boost::uint32_t calc_rx_mux_word(subdev_conn_t subdev_conn);

    /*!
     * Calculate the tx mux word from properties.
     * \param subdev_conn the subdev connection type
     * \return the 32-bit tx mux control word
     */
    UHD_API boost::uint32_t calc_tx_mux_word(subdev_conn_t subdev_conn);

    /*!
     * Calculate the cordic word from the frequency and clock rate.
     * The frequency will be set to the actual (possible) frequency.
     *
     * \param freq the requested frequency in Hz
     * \param codec_rate the dsp codec rate in Hz
     * \return the 32-bit cordic control word
     */
    UHD_API boost::uint32_t calc_cordic_word_and_update(
        double &freq, double codec_rate
    );

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
    UHD_API boost::uint32_t calc_cic_filter_word(unsigned rate);

    /*!
     * Calculate the IQ scale factor word from I and Q components.
     * \param i the I component of the scalar
     * \param q the Q component of the scalar
     * \return the 32-bit scale factor control word
     */
    UHD_API boost::uint32_t calc_iq_scale_word(
        boost::int16_t i, boost::int16_t q
    );

    /*!
     * Calculate the IQ scale factor word from the rate.
     * \param rate the requested rate in Sps
     * \return the 32-bit scale factor control word
     */
    UHD_API boost::uint32_t calc_iq_scale_word(unsigned rate);

    /*!
     * Calculate the stream command word from the stream command struct.
     * \param stream_cmd the requested stream command with mode, flags, timestamp
     * \param num_samps_continuous number of samples to request in continuous mode
     * \return the 32-bit stream command word
     */
    UHD_API boost::uint32_t calc_stream_cmd_word(
        const stream_cmd_t &stream_cmd, size_t num_samps_continuous
    );

} //namespace dsp_type1

}} //namespace

#endif /* INCLUDED_UHD_USRP_DSP_UTILS_HPP */
