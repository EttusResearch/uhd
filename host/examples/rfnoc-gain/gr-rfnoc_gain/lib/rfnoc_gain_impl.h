/* -*- c++ -*- */
/*
 * Copyright 2024 Ettus Research, an NI Brand.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_RFNOC_GAIN_RFNOC_GAIN_IMPL_H
#define INCLUDED_RFNOC_GAIN_RFNOC_GAIN_IMPL_H

#include <gnuradio/rfnoc_gain/rfnoc_gain.h>
#include <rfnoc/gain/gain_block_control.hpp>

namespace gr {
namespace rfnoc_gain {

class rfnoc_gain_impl : public rfnoc_gain
{
public:
    rfnoc_gain_impl(::uhd::rfnoc::noc_block_base::sptr block_ref, const uint32_t gain);
    ~rfnoc_gain_impl();

    /*** API *****************************************************************/
    void set_gain(const uint32_t gain);
    uint32_t get_gain();

private:
    ::rfnoc::gain::gain_block_control::sptr d_gainblk_ref;
};

} // namespace rfnoc_gain
} // namespace gr

#endif /* INCLUDED_RFNOC_GAIN_RFNOC_GAIN_IMPL_H */
