//
// Copyright 2014-2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RFNOC_SIGGEN_BLOCK_CTRL_HPP
#define INCLUDED_LIBUHD_RFNOC_SIGGEN_BLOCK_CTRL_HPP

#include <uhd/rfnoc/source_block_ctrl_base.hpp>
#include <uhd/rfnoc/sink_block_ctrl_base.hpp>

namespace uhd {
    namespace rfnoc {

class UHD_RFNOC_API siggen_block_ctrl : public source_block_ctrl_base, public sink_block_ctrl_base
{
public:
    UHD_RFNOC_BLOCK_OBJECT(siggen_block_ctrl)

}; /* class siggen_block_ctrl*/

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_SIGGEN_BLOCK_CTRL_HPP */
