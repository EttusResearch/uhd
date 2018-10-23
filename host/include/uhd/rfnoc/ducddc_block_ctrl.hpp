//
// Copyright 2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RFNOC_DUCDDC_BLOCK_CTRL_HPP
#define INCLUDED_LIBUHD_RFNOC_DUCDDC_BLOCK_CTRL_HPP

#include <uhd/rfnoc/source_block_ctrl_base.hpp>
#include <uhd/rfnoc/sink_block_ctrl_base.hpp>
#include <uhd/rfnoc/rate_node_ctrl.hpp>

namespace uhd {
    namespace rfnoc {

/*! \brief DUCDDC block controller
 *
 * This block provides basic M:N rate-changing operations.
 * It includes a DDC subcomponent, followed by a DUC. Both components
 * are wrapped with an axi_rate_change to handle corresponding rate changes.
 *
 * While frequency-shift logic exists in the DUC, this block is not
 * intended to perform frequency shifts (just rate changes)
 */
class UHD_RFNOC_API ducddc_block_ctrl :
    public source_block_ctrl_base,
    public sink_block_ctrl_base,
    public rate_node_ctrl
{
public:
    UHD_RFNOC_BLOCK_OBJECT(ducddc_block_ctrl)

}; /* class ducddc_block_ctrl*/

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_DUCDDC_BLOCK_CTRL_HPP */

