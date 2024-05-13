//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_OFDM_BLOCK_CONTROL_HPP
#define INCLUDED_OFDM_BLOCK_CONTROL_HPP

#include <uhd/config.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>
#include <uhd/types/stream_cmd.hpp>

namespace uhd { namespace rfnoc {

// FIXME: This is already defined in the FFT block controller as fft_direction
enum class ofdm_direction { REVERSE, FORWARD };

/*! TODO: Block info
 *
 * TODO: Block description
 */
class UHD_API ofdm_block_control : public uhd::rfnoc::noc_block_base
{
public:
    RFNOC_DECLARE_BLOCK(ofdm_block_control)

    static const uint16_t           MAJOR_COMPAT;
    static const uint16_t           MINOR_COMPAT;

    static const std::string PROP_KEY_FFT_SIZE;
    static const std::string PROP_KEY_FFT_SCALING;
    static const std::string PROP_KEY_FFT_DIRECTION;

    static const int            DEFAULT_FFT_SIZE;
    static const int            DEFAULT_FFT_SCALING;
    static const ofdm_direction DEFAULT_FFT_DIRECTION;
    static const uint32_t       DEFAULT_CP_LEN;

    //! Register addresses
    static const uint32_t REG_COMPAT_ADDR;
    static const uint32_t REG_CAPABILITIES_ADDR;
    static const uint32_t REG_USER_RESET_ADDR;
    static const uint32_t REG_FFT_SIZE_LOG2_ADDR;
    static const uint32_t REG_FFT_SCALING_ADDR;
    static const uint32_t REG_FFT_DIRECTION_ADDR;
    static const uint32_t REG_CP_INS_LEN_ADDR;
    static const uint32_t REG_CP_INS_CP_LEN_FIFO_LOAD_ADDR;
    static const uint32_t REG_CP_INS_CP_LEN_FIFO_CLR_ADDR;
    static const uint32_t REG_CP_INS_CP_LEN_FIFO_OCC_ADDR;
    static const uint32_t REG_CP_REM_LEN_ADDR;
    static const uint32_t REG_CP_REM_CP_LEN_FIFO_LOAD_ADDR;
    static const uint32_t REG_CP_REM_CP_LEN_FIFO_CLR_ADDR;
    static const uint32_t REG_CP_REM_CP_LEN_FIFO_OCC_ADDR;

    virtual void write_reg(const uint32_t addr, const uint32_t wr_val) = 0;
    virtual uint32_t read_reg(const uint32_t addr) = 0;
    virtual void write_and_check_reg(const uint32_t addr, const uint32_t wr_val) = 0;
    virtual void set_fft_size(const uint32_t fft_size) = 0;
    virtual uint32_t get_fft_size() = 0;
    virtual void set_fft_scaling(const uint32_t fft_scaling) = 0;
    virtual uint32_t get_fft_scaling() = 0;
    virtual void set_fft_direction(const ofdm_direction direction) = 0;
    virtual ofdm_direction get_fft_direction() = 0;
    virtual uint32_t get_max_fft_size() = 0;
    virtual uint32_t get_max_cp_length() = 0;
    virtual uint32_t get_max_cp_rem_list_length() = 0;
    virtual uint32_t get_max_cp_ins_list_length() = 0;
    virtual void load_cp_insertion_fifo(const std::vector<uint32_t> cp_lengths) = 0;
    virtual void clear_cp_insertion_fifo() = 0;
    virtual void load_cp_removal_fifo(const std::vector<uint32_t> cp_lengths) = 0;
    virtual void clear_cp_removal_fifo() = 0;
};

}} // namespace uhd::rfnoc

#endif // INCLUDED_OFDM_BLOCK_CONTROL_HPP
