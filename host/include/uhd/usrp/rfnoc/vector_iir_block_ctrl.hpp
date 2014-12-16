//
// Copyright 2014 Ettus Research LLC
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

#ifndef INCLUDED_LIBUHD_RFNOC_VECTOR_IIR_BLOCK_CTRL_HPP
#define INCLUDED_LIBUHD_RFNOC_VECTOR_IIR_BLOCK_CTRL_HPP

#include <uhd/usrp/rfnoc/source_block_ctrl_base.hpp>
#include <uhd/usrp/rfnoc/sink_block_ctrl_base.hpp>

namespace uhd {
    namespace rfnoc {

/*! \brief Block controller for the Vector IIR RFNoC block.
 *
 * Filters across vectors of input samples with a single pole IIR filter. One typical application
 * is to pair this block with a FFT to filter output bins and reduce noise.
 * - One input / output block port
 * - Supports data type sc16 (16-Bit fix-point complex samples)
 * - Configurable size
 * - Accepts sc16, but the real & imag samples are filtered individually
 *
 * H(z) = Alpha/(1 - Beta*z^-1)
 * Typically Beta = 1 - Alpha
 *
 * This block requires packets to be the same size as the Vector IIR length.
 */
class UHD_API vector_iir_block_ctrl : public source_block_ctrl_base, public sink_block_ctrl_base
{
public:
    UHD_RFNOC_BLOCK_OBJECT(vector_iir_block_ctrl)

    static const size_t DEFAULT_VECTOR_LEN      = 256;
    static const double DEFAULT_ALPHA           = 0.9;
    static const double DEFAULT_BETA            = 0.1;
    static const boost::uint32_t SR_VECTOR_LEN  = 129;
    static const boost::uint32_t SR_ALPHA       = 130;
    static const boost::uint32_t SR_BETA        = 131;

    //! Configure the vector length.
    //
    // This will not only configure the Vector IIR block, but also sets
    // the packet size in the in- and output signatures.
    //
    // Note that not all vector lengths are valid for this block. Throws if
    // an invalid value is provided.
    virtual void set_vector_len(size_t vector_len) = 0;

    //! Returns the current vector length.
    //
    // Used after calling set_vector_len() to get the current vector length.
    virtual size_t get_vector_len() const = 0;

    //! Configure the Alpha constant.
    //
    // Alpha should be between [-1.0,1.0]. Typically Beta = 1 - Alpha.
    // Alpha will be converted to a 32-bit fixed point value (Q1.31), although
    // the actual precision is dependent on the block's HDL parameters.
    // Care should be taken changing Alpha while running, as instability
    // can occur.
    //
    // Throws if an invalid value is provided.
    virtual void set_alpha(double alpha) = 0;

    //! Returns the current value for the Alpha constant.
    //
    // Used after calling set_alpha() to get the current value for Alpha.
    virtual double get_alpha() const = 0;

    //! Configure the Beta constant.
    //
    // Beta should be between [-1.0,1.0). Typically Beta = 1 - Alpha.
    // Beta will be converted to a 32-bit fixed point value (Q1.31), although
    // the actual precision is dependent on the block's HDL parameters.
    // Care should be taken changing Beta while running, as instability
    // can occur.
    //
    // Throws if an invalid value is provided.
    virtual void set_beta(double alpha) = 0;

    //! Returns the current value for the Beta constant.
    //
    // Used after calling set_beta() to get the current value for Beta.
    virtual double get_beta() const = 0;

}; /* class vector_iir_block_ctrl*/

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_VECTOR_IIR_BLOCK_CTRL_HPP */
