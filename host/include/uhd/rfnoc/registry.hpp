//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RFNOC_REGISTRY_HPP
#define INCLUDED_LIBUHD_RFNOC_REGISTRY_HPP

#include <uhd/config.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>
#include <uhd/utils/static.hpp>
#include <functional>
#include <string>

//! This macro must be placed inside a block implementation file
// after the class definition
#define UHD_RFNOC_BLOCK_REGISTER_DIRECT(CLASS_NAME, NOC_ID, BLOCK_NAME)   \
    uhd::rfnoc::noc_block_base::sptr CLASS_NAME##_make(                   \
        uhd::rfnoc::noc_block_base::make_args_ptr make_args)              \
    {                                                                     \
        return std::make_shared<CLASS_NAME##_impl>(std::move(make_args)); \
    }                                                                     \
    UHD_STATIC_BLOCK(register_rfnoc_##CLASS_NAME)                         \
    {                                                                     \
        uhd::rfnoc::registry::register_block_direct(                      \
            NOC_ID, BLOCK_NAME, &CLASS_NAME##_make);                      \
    }

#define UHD_RFNOC_BLOCK_REQUEST_MB_ACCESS(NOC_ID)              \
    UHD_STATIC_BLOCK(rfnoc_block_##NOC_ID##_request_mb_access) \
    {                                                          \
        uhd::rfnoc::registry::request_mb_access(NOC_ID);       \
    }

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
     * \param noc_id The 32-bit Noc-ID for this block (e.g. 0xDDC00000)
     * \param block_name The name used for the block ID (e.g. "Radio")
     * \param factory_fn A factory function that returns a reference to the
     *                   block
     */
    static void register_block_direct(noc_block_base::noc_id_t noc_id,
        const std::string& block_name,
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
    static void register_block_descriptor(const std::string& block_key,
        factory_t factory_fn);

    /*! Call this after registering a block if it requires access to the
     * mb_controller
     *
     * Note: This is a request to the framework, and may be denied.
     *
     * \param noc_id Noc-ID of the block that requires access to the mb_controller
     */
    static void request_mb_access(noc_block_base::noc_id_t noc_id);

    /*! Call this after registering a block if it requires access to the
     * mb_controller
     *
     * Note: This is a request to the framework, and may be denied.
     *
     * \param noc_id Noc-ID of the block that requires access to the mb_controller
     */
    static void request_mb_access(const std::string& block_key);

};

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_REGISTRY_HPP */
