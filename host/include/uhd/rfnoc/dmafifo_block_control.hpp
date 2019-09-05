//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_DMAFIFO_BLOCK_CONTROL_HPP
#define INCLUDED_LIBUHD_DMAFIFO_BLOCK_CONTROL_HPP

#include <uhd/config.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>

namespace uhd { namespace rfnoc {

/*! DMA FIFO Block Control Class
 */
class UHD_API dmafifo_block_control : public noc_block_base
{
public:
    RFNOC_DECLARE_BLOCK(dmafifo_block_control)
};

}} // namespace uhd::rfnoc

#endif /* INCLUDED_LIBUHD_DMAFIFO_BLOCK_CONTROL_HPP */
