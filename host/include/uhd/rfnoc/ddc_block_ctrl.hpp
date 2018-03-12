//
// Copyright 2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RFNOC_DDC_BLOCK_CTRL_HPP
#define INCLUDED_LIBUHD_RFNOC_DDC_BLOCK_CTRL_HPP

#include <uhd/rfnoc/source_block_ctrl_base.hpp>
#include <uhd/rfnoc/sink_block_ctrl_base.hpp>
#include <uhd/rfnoc/rate_node_ctrl.hpp>
#include <uhd/rfnoc/scalar_node_ctrl.hpp>

namespace uhd {
    namespace rfnoc {

/*! \brief DDC block controller
 *
 * This block provides DSP for Rx operations.
 * Its main component is a DDC chain, which can decimate over a wide range
 * of decimation rates (using a CIC and halfband filters).
 *
 * It also includes a CORDIC component to shift signals in frequency.
 */
class UHD_RFNOC_API ddc_block_ctrl :
    public source_block_ctrl_base,
    public sink_block_ctrl_base,
    public rate_node_ctrl,
    public scalar_node_ctrl
{
public:
    UHD_RFNOC_BLOCK_OBJECT(ddc_block_ctrl)

}; /* class ddc_block_ctrl*/

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_DDC_BLOCK_CTRL_HPP */
