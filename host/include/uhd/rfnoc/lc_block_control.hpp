//
// Copyright 2023 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>
#include <vector>

namespace uhd { namespace rfnoc {

/*! License Checker Block Control Class
 *
 * \ingroup rfnoc_blocks
 *
 * The License Checker Block is a block for RFNoC without any streaming ports.
 * Its sole purpose is to read and evaluate license keys.
 */
class UHD_API lc_block_control : public noc_block_base
{
public:
    RFNOC_DECLARE_BLOCK(lc_block_control)

    // Block registers
    static const uint32_t REG_COMPAT_NUM;
    static const uint32_t REG_FEATURE_ID;
    static const uint32_t REG_USER_KEY;
    static const uint32_t REG_FEATURE_ENABLE_RB;
    static const uint32_t REG_FEATURE_LIST_RB;

    static const uint16_t MAJOR_COMPAT;
    static const uint16_t MINOR_COMPAT;

    /*! Load a license key into the checker
     *
     * Get the maximum number of window coefficients supported by this
     * block.
     *
     * \param key The channel to retrieve the maximum number of coefficients from
     * \returns True if the key was successfully loaded. This does not mean that
     *          the key was valid, only that loading the key worked.
     */
    virtual bool load_key(const std::string& key) = 0;

    /*! Return a list of feature IDs this block is managing
     */
    virtual std::vector<uint32_t> get_feature_ids() = 0;
};

}} // namespace uhd::rfnoc
