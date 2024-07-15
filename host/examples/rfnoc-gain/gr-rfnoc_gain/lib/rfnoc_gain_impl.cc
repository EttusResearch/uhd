/* -*- c++ -*- */
/*
 * Copyright 2024 Ettus Research, an NI Brand.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "rfnoc_gain_impl.h"
#include <gnuradio/io_signature.h>

namespace gr {
namespace rfnoc_gain {

rfnoc_gain::sptr rfnoc_gain::make(uhd::rfnoc_graph::sptr graph,
                                  const uint32_t gain,
                                  const ::uhd::device_addr_t& block_args,
                                  const int device_select,
                                  const int instance)
{
    return gnuradio::make_block_sptr<rfnoc_gain_impl>(
        uhd::rfnoc_block::make_block_ref(
            graph, block_args, "Gain", device_select, instance),
        gain);
}


rfnoc_gain_impl::rfnoc_gain_impl(::uhd::rfnoc::noc_block_base::sptr block_ref,
                                 const uint32_t gain)
    : rfnoc_block(block_ref),
      d_gainblk_ref(get_block_ref<::rfnoc::gain::gain_block_control>())
{
    d_gainblk_ref->set_gain_value(gain);
}

rfnoc_gain_impl::~rfnoc_gain_impl() {}

/******************************************************************************
 * rfnoc_gain API
 *****************************************************************************/
void rfnoc_gain_impl::set_gain(const uint32_t gain)
{
    d_gainblk_ref->set_gain_value(gain);
}

uint32_t rfnoc_gain_impl::get_gain() { return d_gainblk_ref->get_gain_value(); }

} /* namespace rfnoc_gain */
} /* namespace gr */
