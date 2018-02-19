//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RFNOC_BLOCK_CTRL_HPP
#define INCLUDED_LIBUHD_RFNOC_BLOCK_CTRL_HPP

#include <uhd/rfnoc/source_block_ctrl_base.hpp>
#include <uhd/rfnoc/sink_block_ctrl_base.hpp>

namespace uhd {
    namespace rfnoc {

/*! \brief This is the default implementation of a block_ctrl_base.
 *
 * For most blocks, this will be a sufficient implementation. All registers
 * can be set by sr_write(). The default behaviour of functions is documented
 * in uhd::rfnoc::block_ctrl_base.
 */
class UHD_RFNOC_API block_ctrl : public source_block_ctrl_base, public sink_block_ctrl_base
{
public:
    // Required macro in RFNoC block classes
    UHD_RFNOC_BLOCK_OBJECT(block_ctrl)

    // Nothing else here -- all function definitions are in block_ctrl_base,
    // source_block_ctrl_base and sink_block_ctrl_base

}; /* class block_ctrl*/

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_BLOCK_CTRL_HPP */
// vim: sw=4 et:
