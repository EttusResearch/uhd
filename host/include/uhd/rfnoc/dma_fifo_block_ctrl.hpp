//
// Copyright 2016 Ettus Research LLC
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

#ifndef INCLUDED_LIBUHD_RFNOC_DMA_FIFO_BLOCK_HPP
#define INCLUDED_LIBUHD_RFNOC_DMA_FIFO_BLOCK_HPP

#include <uhd/rfnoc/source_block_ctrl_base.hpp>
#include <uhd/rfnoc/sink_block_ctrl_base.hpp>

namespace uhd {
    namespace rfnoc {

/*! \brief Block controller for a DMA FIFO block.
 *
 * The DMA FIFO block has the following features:
 * - One input- and output-port (type agnostic)
 * - Configurable base address and FIFO depth
 * - The base storage for the FIFO can be device
 *   specific. Usually it will be an off-chip SDRAM
 *   bank.
 *
 */
class UHD_RFNOC_API dma_fifo_block_ctrl : public source_block_ctrl_base, public sink_block_ctrl_base
{
public:
    UHD_RFNOC_BLOCK_OBJECT(dma_fifo_block_ctrl)

    //! Configure the base address and depth of the FIFO (in bytes).
    virtual void resize(const uint32_t base_addr, const uint32_t depth, const size_t chan) = 0;

    //! Returns the base address of the FIFO (in bytes).
    uint32_t get_base_addr(const size_t chan) const;

    //! Returns the depth of the FIFO (in bytes).
    uint32_t get_depth(const size_t chan) const;

}; /* class dma_fifo_block_ctrl*/

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_DMA_FIFO_BLOCK_HPP */
