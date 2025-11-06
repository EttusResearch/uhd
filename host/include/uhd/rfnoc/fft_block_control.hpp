//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>

namespace uhd { namespace rfnoc {

enum class fft_shift { NORMAL, REVERSE, NATURAL, BIT_REVERSE };
enum class fft_direction { REVERSE, FORWARD };
enum class fft_magnitude { COMPLEX, MAGNITUDE, MAGNITUDE_SQUARED };

// Custom property keys
static const std::string PROP_KEY_MAGNITUDE          = "magnitude";
static const std::string PROP_KEY_DIRECTION          = "direction";
static const std::string PROP_KEY_LENGTH             = "length";
static const std::string PROP_KEY_FFT_SCALING        = "fft_scaling";
static const std::string PROP_KEY_FFT_SCALING_FACTOR = "fft_scaling_factor";
static const std::string PROP_KEY_SHIFT_CONFIG       = "shift_config";
static const std::string PROP_KEY_BYPASS_MODE        = "bypass_mode";
static const std::string PROP_KEY_CP_INSERTION_LIST  = "cp_insertion_list";
static const std::string PROP_KEY_CP_REMOVAL_LIST    = "cp_removal_list";
static const std::string PROP_KEY_NIPC               = "nipc";
static const std::string PROP_KEY_MAX_LENGTH         = "max_length";
static const std::string PROP_KEY_MAX_CP_LENGTH      = "max_cp_length";
static const std::string PROP_KEY_MAX_CP_INSERTION_LIST_LENGTH =
    "max_cp_insertion_list_length";
static const std::string PROP_KEY_MAX_CP_REMOVAL_LIST_LENGTH =
    "max_cp_removal_list_length";

/*! FFT Block (with Cyclic prefix insertion and removal) Control Class
 *
 * \ingroup rfnoc_blocks
 *
 * The FFT block is an RFNoC block that accepts signed complex 16-bit data
 * at its input and computes the forward or reverse FFT of the input data,
 * outputting signed complex 16-bit data at its output.
 *
 * The FFT length is configured via the length parameter, up to a maximum
 * which depends on the instantiation on the FPGA. Use the function
 * get_max_fft_length to determine the maximum supported FFT length.
 *
 * The length will be coerced to the closest power of two which is smaller
 * than length. The block will output packets of the same length in the
 * desired format as configured via the API.
 *
 * The block can be configured to add cyclic prefixes (typically when
 * performing an inverse FFT, i.e. repeating a part of the generated time domain
 * signal) or to remove cyclic prefixes (typically when performing a forward
 * FFT, i.e. removing a part of the input time domain signal). This feature
 * makes this block suitable for OFDM (de-)modulation.
 */
class UHD_API fft_block_control : public uhd::rfnoc::noc_block_base
{
public:
    RFNOC_DECLARE_BLOCK(fft_block_control)

    //! Register addresses
    static const uint32_t REG_COMPAT_ADDR;
    static const uint32_t REG_PORT_CONFIG_ADDR;
    static const uint32_t REG_CAPABILITIES_ADDR;
    static const uint32_t REG_CAPABILITIES2_ADDR;
    static const uint32_t REG_RESET_ADDR;
    static const uint32_t REG_LENGTH_LOG2_ADDR;
    static const uint32_t REG_SCALING_ADDR;
    static const uint32_t REG_DIRECTION_ADDR;
    static const uint32_t REG_CP_INS_LEN_ADDR;
    static const uint32_t REG_CP_INS_LIST_LOAD_ADDR;
    static const uint32_t REG_CP_INS_LIST_CLR_ADDR;
    static const uint32_t REG_CP_INS_LIST_OCC_ADDR;
    static const uint32_t REG_CP_REM_LEN_ADDR;
    static const uint32_t REG_CP_REM_LIST_LOAD_ADDR;
    static const uint32_t REG_CP_REM_LIST_CLR_ADDR;
    static const uint32_t REG_CP_REM_LIST_OCC_ADDR;
    static const uint32_t REG_OVERFLOW_ADDR;
    static const uint32_t REG_BYPASS_ADDR;
    static const uint32_t REG_ORDER_ADDR;
    static const uint32_t REG_MAGNITUDE_ADDR;

    //! Register addresses of the FFT block version 1
    static const uint32_t REG_RESET_ADDR_V1;
    static const uint32_t REG_LENGTH_LOG2_ADDR_V1;
    static const uint32_t REG_MAGNITUDE_ADDR_V1;
    static const uint32_t REG_DIRECTION_ADDR_V1;
    static const uint32_t REG_SCALING_ADDR_V1;
    static const uint32_t REG_ORDER_ADDR_V1;

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
     *     * Bit-reversed order (BIT_REVERSE). This is typically the native
     *       order output by the FFT. In other words, selecting this mode may
     *       mean that the data from the FFT is not reordered. In this mode,
     *       the indices of the FFT are bit-reversed. For example, for a size
     *       16 FFT, instead of outputting bins in the order 0000, 0001, 0010,
     *       0011, etc., it outputs bins 0000, 1000, 0100, 1100, etc.
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

    /*! Set the scaling factor for the FFT block
     *
     * This is a convenience function which can be used instead of
     * set_scaling(). Based on the given factor, it automatically sets the
     * scaling mask by evenly distributing the scaling across the active FFT
     * stages
     *
     * Examples:
     * - factor = 1.0 -> no scaling
     * - factor = 0.5 (1/2) -> the scaling will be set to 0b000000000001
     *   -> scale by 2 in the first FFT stage
     * - factor = 0.0625 (1/16) -> the scaling will be set to 0b000000001010
     *   -> scale by 4 in both the first and the second FFT stage
     * - factor = 0.03125 (1/32) -> the scaling will be set to 0b000000011010
     *   -> scale by 4 in both the first and the second FFT stage
     *   and by 2 in the third FFT stage
     *
     * \param factor Desired scaling factor
     */
    virtual void set_scaling_factor(const double factor) = 0;

    /*! Set the scaling schedule for the FFT block
     *
     * Sets the scaling for each stage of the FFT. This value maps directly
     * to the scale schedule field in the configuration channel data that is
     * passed to the Xilinx AXI FFT IP. For more information on the format
     * of this data, see Xilinx document PG109, Fast Fourier Transform
     * LogiCORE IP Product Guide.
     *
     * Examples:
     * - scaling = 0b000000000000 -> no scaling
     * - scaling = 0b000000000001 -> scale by 2 in the first FFT stage
     * - scaling = 0b000000001010 -> scale by 4 in both the first and the second
     *   FFT stage
     * - scaling = 0b000000011010 -> scale by 4 in both the first and the second
     *   FFT stage and by 2 in the third FFT stage
     *
     * \param scaling Scaling schedule for the FFT block
     */
    virtual void set_scaling(const uint32_t scaling) = 0;

    /*! Get the scaling schedule for the FFT block
     *
     * Returns the current scaling schedule for the FFT block.
     *
     * \returns Scaling schedule for the FFT block
     */
    virtual uint32_t get_scaling() const = 0;

    /*! Set the length of the FFT
     *
     * Sets the length of the FFT in number of samples. Note that the FFT
     * IP requires a power-of-two number of samples; the incoming value will
     * be coerced to the closest smaller power of two.
     *
     * \param length Desired FFT length
     */
    virtual void set_length(const uint32_t length) = 0;

    /*! Get the length of the FFT
     *
     * Returns the current length of the FFT.
     *
     * \returns Current FFT length
     */
    virtual uint32_t get_length() const = 0;

    /*! Set the bypass mode of the FFT
     *
     * Enable FFT bypass mode. Set true to enable, false to disable. When
     * enabled, the data is passed through without any FFT processing. Note that
     * cyclic prefix insertion will not work in bypass mode, because insertion
     * is handled by the FFT core, but cyclic prefix removal will work.
     *
     * \param bypass FFT bypass moe
     */
    virtual void set_bypass_mode(const bool bypass) = 0;

    /*! Get the bypass mode of the FFT
     *
     * Returns the current bypass mode.
     *
     * \returns Current FFT bypass mode
     */
    virtual bool get_bypass_mode() const = 0;

    /*! Get the number of items per clock cycle (NIPC)
     *
     * Returns the number of items per clock cycle (NIPC) that this block is
     * configured to process. Packet sizes and cyclic prefix lengths must a
     * multiple of this value.
     *
     * \returns NIPC
     */
    virtual uint32_t get_nipc() const = 0;

    /*! Get the maximum supported length of the FFT
     *
     * Returns the maximum supported length of the FFT.
     *
     * \returns Maximum supported FFT length
     */
    virtual uint32_t get_max_length() const = 0;

    /*! Get the maximum supported cyclic prefix length
     *
     * Returns the maximum supported cyclic prefix length.
     *
     * \returns Maximum supported cyclic prefix length
     */
    virtual uint32_t get_max_cp_length() const = 0;

    /*! Get the maximum supported number of values that can be written to the
     * cyclic prefix removal list
     *
     * Returns the maximum supported number of values that can be written to
     * the cyclic prefix removal list.
     *
     * \returns Maximum number of values for the cyclic prefix removal list
     */
    virtual uint32_t get_max_cp_removal_list_length() const = 0;

    /*! Get the maximum supported number of values that can be written to the
     * cyclic prefix insertion list
     *
     * Returns the maximum supported number of values that can be written to
     * the cyclic prefix insertion list.
     *
     * \returns Maximum number of values for the cyclic prefix insertion list
     */
    virtual uint32_t get_max_cp_insertion_list_length() const = 0;

    /*! Load values to the cyclic prefix insertion list.
     *
     * Loads values to the cyclic prefix insertion list. Each value represents
     * the length of a cyclic prefix that is prepended to the output of the
     * (typically inverse) FFT operation (typically the time domain signal). If
     * the length of the cyclic prefix insertion list is m, then the cyclic
     * prefix length that is added to the output signal of symbol n is:
     *
     * cp_length[n] = cp_length[n mod m]
     *
     * \param cp_lengths The cyclic prefix lengths to be written to the list
     */
    virtual void set_cp_insertion_list(const std::vector<uint32_t> cp_lengths) = 0;

    /*! Gets the values from the cyclic prefix insertion list.
     *
     * Gets values to the cyclic prefix insertion list. After initialization,
     * no CP insertion values are configured. Use the function
     * set_cp_insertion_list to set the values.
     */
    virtual std::vector<uint32_t> get_cp_insertion_list() const = 0;

    /*! Load values to the cyclic prefix removal list.
     *
     * Loads values to the cyclic prefix removal list. Each value represents
     * the length of a cyclic prefix that is removed from the input signal
     * (typically the time domain signal) before performing the (typically
     * forward) FFT operation. If the length of the cyclic prefix removal list
     * is m, then the cyclic prefix length that is removed from the input signal
     * of symbol n is:
     *
     * cp_length[n] = cp_length[n mod m]
     *
     * \param cp_lengths The cyclic prefix lengths to be written to the list
     */
    virtual void set_cp_removal_list(const std::vector<uint32_t> cp_lengths) = 0;

    /*! Gets the values from the cyclic prefix removal list.
     *
     * Gets values to the cyclic prefix removal list. After initialization,
     * no CP removal values are configured. Use the function
     * set_cp_removal_list to set the values.
     */
    virtual std::vector<uint32_t> get_cp_removal_list() const = 0;
};

}} // namespace uhd::rfnoc
