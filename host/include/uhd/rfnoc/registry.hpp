//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>
#include <uhd/utils/static.hpp>
#include <functional>
#include <string>

//! This macro must be placed inside a block implementation file
// after the class definition
#define UHD_RFNOC_BLOCK_REGISTER_FOR_DEVICE_DIRECT(                             \
    CLASS_NAME, NOC_ID, DEVICE_ID, BLOCK_NAME, MB_ACCESS, TB_CLOCK, CTRL_CLOCK) \
    uhd::rfnoc::noc_block_base::sptr CLASS_NAME##_make(                         \
        uhd::rfnoc::noc_block_base::make_args_ptr make_args)                    \
    {                                                                           \
        return std::make_shared<CLASS_NAME##_impl>(std::move(make_args));       \
    }                                                                           \
    UHD_STATIC_BLOCK(register_rfnoc_##CLASS_NAME)                               \
    {                                                                           \
        uhd::rfnoc::registry::register_block_direct(NOC_ID,                     \
            DEVICE_ID,                                                          \
            BLOCK_NAME,                                                         \
            MB_ACCESS,                                                          \
            TB_CLOCK,                                                           \
            CTRL_CLOCK,                                                         \
            &CLASS_NAME##_make);                                                \
    }

#define UHD_RFNOC_BLOCK_REGISTER_DIRECT(                  \
    CLASS_NAME, NOC_ID, BLOCK_NAME, TB_CLOCK, CTRL_CLOCK) \
    UHD_RFNOC_BLOCK_REGISTER_FOR_DEVICE_DIRECT(           \
        CLASS_NAME, NOC_ID, ANY_DEVICE, BLOCK_NAME, false, TB_CLOCK, CTRL_CLOCK)

#define UHD_RFNOC_BLOCK_REGISTER_DIRECT_MB_ACCESS(        \
    CLASS_NAME, NOC_ID, BLOCK_NAME, TB_CLOCK, CTRL_CLOCK) \
    UHD_RFNOC_BLOCK_REGISTER_FOR_DEVICE_DIRECT(           \
        CLASS_NAME, NOC_ID, ANY_DEVICE, BLOCK_NAME, true, TB_CLOCK, CTRL_CLOCK)

namespace uhd { namespace rfnoc {

/*! RFNoC Block Registry
 *
 * A container for various functions to register blocks
 */
class UHD_API registry
{
public:
    using factory_t = std::function<noc_block_base::sptr(noc_block_base::make_args_ptr)>;

    /*! Register a block that does not use a block descriptor file
     *
     * Note: It is highly recommended to use the UHD_RFNOC_BLOCK_REGISTER_DIRECT()
     * macro instead of calling this function.
     *
     * Use this registry function for blocks that do not read from a textual
     * description (block descriptor file).
     *
     * If the Noc-ID is already registered, it will print an error to stderr and
     * ignore the new block.
     *
     * \param noc_id The 32-bit Noc-ID for this block (e.g. 0xDDC00000).
     * \param device_id The 16-bit Device-ID for this block
     *        (ANY_DEVICE for device agnostic blocks).
     * \param block_name The name used for the block ID (e.g. "Radio").
     * \param mb_access Set this to true to request full access to the
     *                  motherboard in the block controller. Radio blocks, for
     *                  example, require this. If set, UHD may grant access to
     *                  the underlying motherboard controller to the block.
     *                  See also uhd::rfnoc::noc_block_base::get_mb_controller().
     * \param timebase_clock The name of the clock that is used for time
     *                       reference on timed commands. Blocks that derive
     *                       time from the graph (e.g. DDC, DUC blocks) should
     *                       set this to uhd::rfnoc::CLOCK_KEY_GRAPH.
     * \param ctrlport_clock The name of the clock that is driving the control
     *                       port. This is device-dependent. A typical value
     *                       is `"bus_clk"`.
     * \param factory_fn A factory function that returns a reference to the
     *                   block.
     */
    static void register_block_direct(noc_id_t noc_id,
        device_type_t device_id,
        const std::string& block_name,
        bool mb_access,
        const std::string& timebase_clock,
        const std::string& ctrlport_clock,
        factory_t factory_fn);

    /*! Same as above, but with multiple noc_ids
     */
    static void register_block_direct(std::vector<noc_id_t> noc_ids,
        device_type_t device_id,
        const std::string& block_name,
        bool mb_access,
        const std::string& timebase_clock,
        const std::string& ctrlport_clock,
        factory_t factory_fn);

    /*! Register a block that does use a block descriptor file
     *
     * Use this registry function for blocks that also have a textual
     * description (block descriptor file).
     *
     * For these blocks, the framework will first look up the Noc-ID from the
     * blocks on the FPGA, and then find the corresponding block key by
     * searching all the availble block descriptor files. When such a key is
     * found, it will be used to find a block that was previously registered
     * here.
     */
    static void register_block_descriptor(
        const std::string& block_key, factory_t factory_fn);
};

}} /* namespace uhd::rfnoc */
