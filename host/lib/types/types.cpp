//
// Copyright 2011 Ettus Research LLC
//
// SPDX-License-Identifier: GPL-3.0
//

#include <uhd/types/stream_cmd.hpp>
#include <uhd/types/metadata.hpp>

using namespace uhd;

/***********************************************************************
 * stream command
 **********************************************************************/
stream_cmd_t::stream_cmd_t(const stream_mode_t &stream_mode):
    stream_mode(stream_mode),
    num_samps(0),
    stream_now(true)
{
    /* NOP */
}

/***********************************************************************
 * metadata
 **********************************************************************/
tx_metadata_t::tx_metadata_t(void):
    has_time_spec(false),
    time_spec(time_spec_t()),
    start_of_burst(false),
    end_of_burst(false)
{
    /* NOP */
}
