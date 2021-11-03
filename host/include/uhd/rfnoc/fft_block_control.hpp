//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>

namespace uhd { namespace rfnoc {

enum class fft_shift { NORMAL, REVERSE, NATURAL };
enum class fft_direction { REVERSE, FORWARD };
enum class fft_magnitude { COMPLEX, MAGNITUDE, MAGNITUDE_SQUARED };

// Custom property keys
static const std::string PROP_KEY_MAGNITUDE    = "magnitude";
static const std::string PROP_KEY_DIRECTION    = "direction";
static const std::string PROP_KEY_LENGTH       = "length";
static const std::string PROP_KEY_FFT_SCALING  = "fft_scaling";
static const std::string PROP_KEY_SHIFT_CONFIG = "shift_config";

/*! FFT Block Control Class
 *
 * \ingroup rfnoc_blocks
 *
 * The FFT block is an RFNoC block that accepts signed complex 16-bit data
 * at its input and computes the forward or reverse FFT of the input data,
 * outputting signed complex 16-bit data at its output. The output data
 * may be configured as complex, magnitude, or mag-squared values, its
 * spectrum shifted and/or reversed, and scaled by a scaled factor.
 *
 * The FFT length is configured via the length parameter, up to a maximum
 * of 2048 samples. The FFT IP requires a power-of-two number of samples;
 * the length will be coerced to the closest power of two which is smaller
 * than length. The block will output packets of the same length in the
 * desired format as configured via the API.
 */
class UHD_API fft_block_control : public noc_block_base
{
public:
    RFNOC_DECLARE_BLOCK(fft_block_control)

    static const uint32_t REG_RESET_ADDR;
    static const uint32_t REG_LENGTH_LOG2_ADDR;
    static const uint32_t REG_MAGNITUDE_OUT_ADDR;
    static const uint32_t REG_DIRECTION_ADDR;
    static const uint32_t REG_SCALING_ADDR;
    static const uint32_t REG_SHIFT_CONFIG_ADDR;

    /*! Set the FFT direction
     *
     * Sets the direction of the FFT, either forward (FORWARD) or inverse
     * (REVERSE).
     *
     * \param direction FFT direction
     */
    virtual void set_direction(const fft_direction direction) = 0;

    /*! Get the FFT direction
     *
     * Returns the current direction of the FFT.
     *
     * \returns FFT direction
     */
    virtual fft_direction get_direction() const = 0;

    /*! Set the format of the returned FFT output data
     *
     * Sets the format in which the FFT output data is returned. The following
     * formats are supported:
     *
     *     * amplitude/phase data (COMPLEX)
     *     * magnitude data (MAGNITUDE)
     *     * mag-squared data (MAGNITUDE_SQUARED)
     *
     * \param magnitude Format of the returned FFT output data
     */
    virtual void set_magnitude(const fft_magnitude magnitude) = 0;

    /*! Get the format of the returned FFT output data
     *
     * Returns the current output format of the FFT data.
     *
     * \returns Format of the returned FFT output data
     */
    virtual fft_magnitude get_magnitude() const = 0;

    /*! Set the shift configuration of the output FFT data
     *
     * Sets how the FFT output data is shifted (to get the zero frequency bin
     * to the center of the output data). The following output data shift
     * configurations are supported:
     *
     *     * Negative frequencies first, then positive frequencies (NORMAL)
     *     * Positive frequencies first, then negative frequencies (REVERSE)
     *     * Bypass the shift altogether, leaving the zero frequency bin
     *       returned first (NATURAL).
     *
     * \param shift Configuration for shifting FFT output data
     */
    virtual void set_shift_config(const fft_shift shift) = 0;

    /*! Get the shift configuration of the output FFT data
     *
     * Returns the current shift configuration of the output FFT data.
     *
     * \returns Shift configuration of the output FFT data
     */
    virtual fft_shift get_shift_config() const = 0;

    /*! Set the scaling schedule for the FFT block
     *
     * Sets the scaling for each stage of the FFT. This value maps directly
     * to the scale schedule field in the configuration channel data that is
     * passed to the Xilinx AXI FFT IP. For more information on the format
     * of this data, see Xilinx document PG109, Fast Fourier Transform
     * LogiCORE IP Product Guide.
     *
     * \param scaling Scaling schedule for the FFT block
     */
    virtual void set_scaling(const uint16_t scaling) = 0;

    /*! Get the scaling schedule for the FFT block
     *
     * Returns the current scaling schedule for the FFT block.
     *
     * \returns Scaling schedule for the FFT block
     */
    virtual uint16_t get_scaling() const = 0;

    /*! Set the length of the FFT
     *
     * Sets the length of the FFT in number of samples. Note that the FFT
     * IP requires a power-of-two number of samples; the incoming value will
     * be coerced to the closest smaller power of two.
     *
     * \param length Desired FFT length
     */
    virtual void set_length(const size_t length) = 0;

    /*! Get the length of the FFT
     *
     * Returns the current length of the FFT.
     *
     * \returns Current FFT length
     */
    virtual size_t get_length() const = 0;
};

}} // namespace uhd::rfnoc
