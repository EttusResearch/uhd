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
static const std::string PROP_KEY_MAGNITUDE   = "magnitude";
static const std::string PROP_KEY_DIRECTION   = "direction";
static const std::string PROP_KEY_FFT_LEN     = "fft_len";
static const std::string PROP_KEY_FFT_SCALING = "fft_scaling";
static const std::string PROP_KEY_FFT_SHIFT   = "fft_shift";

/*! FFT Block Control Class
 */
class UHD_API fft_block_control : public noc_block_base
{
public:
    RFNOC_DECLARE_BLOCK(fft_block_control)

    // Readback addresses
    static const uint32_t RB_FFT_RESET;
    static const uint32_t RB_MAGNITUDE_OUT;
    static const uint32_t RB_FFT_SIZE_LOG2;
    static const uint32_t RB_FFT_DIRECTION;
    static const uint32_t RB_FFT_SCALING;
    static const uint32_t RB_FFT_SHIFT_CONFIG;
    // Write addresses
    static const uint32_t SR_FFT_RESET;
    static const uint32_t SR_FFT_SIZE_LOG2;
    static const uint32_t SR_MAGNITUDE_OUT;
    static const uint32_t SR_FFT_DIRECTION;
    static const uint32_t SR_FFT_SCALING;
    static const uint32_t SR_FFT_SHIFT_CONFIG;
};

}} // namespace uhd::rfnoc
