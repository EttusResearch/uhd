//
// Copyright 2016 Ettus Research
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
