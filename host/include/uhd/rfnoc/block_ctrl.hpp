//
// Copyright 2014 Ettus Research LLC
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
