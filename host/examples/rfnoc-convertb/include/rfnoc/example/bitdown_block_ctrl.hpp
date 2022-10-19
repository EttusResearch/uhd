/* -*- c++ -*- */
/* 
 * Copyright 2022 <+YOU OR YOUR COMPANY+>.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */


#pragma once

#include <uhd/config.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>
#include <uhd/types/stream_cmd.hpp>
namespace uhd {
    namespace rfnoc {

/*! \brief Block controller for the standard copy RFNoC block.
 *
 */
class UHD_API bitdown_block_ctrl : public noc_block_base
{
public:
    RFNOC_DECLARE_BLOCK(bitdown_block_ctrl)

    static const uint32_t REG_BITDOWN_ADDR;
    static const uint32_t REG_BITDOWN_DEFAULT;

    /*! Set the gain value
     */
    virtual void set_bitdown_value(const uint32_t bitdown) = 0;

    /*! Get the current gain value (read it from the device)
     */
    virtual uint32_t get_bitdown_value() = 0;

}; /* class bitdown_block_ctrl*/

}} /* namespace uhd::rfnoc */

