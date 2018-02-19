//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/stream_sig.hpp>
#include <uhd/convert.hpp>
#include <boost/format.hpp>

using namespace uhd::rfnoc;

stream_sig_t::stream_sig_t() :
    item_type(""),
    vlen(0),
    packet_size(0),
    is_bursty(false)
{
    // nop
}

std::string stream_sig_t::to_string()
{
    return str(
        boost::format(
            "%s,vlen=%d,packet_size=%d"
        ) % item_type % vlen % packet_size
    );
}

std::string stream_sig_t::to_pp_string()
{
    return str(
        boost::format(
            "Data type: %s  | Vector Length: %d | Packet size: %d"
        ) % item_type % vlen % packet_size
    );
}

size_t stream_sig_t::get_bytes_per_item() const
{
    if (item_type == "") {
        return 0;
    }

    return uhd::convert::get_bytes_per_item(item_type);
}

bool stream_sig_t::is_compatible(const stream_sig_t &output_sig, const stream_sig_t &input_sig)
{
    /// Item types:
    if (not (input_sig.item_type.empty() or output_sig.item_type.empty())
        and input_sig.item_type != output_sig.item_type) {
        return false;
    }

    /// Vector lengths
    if (output_sig.vlen and input_sig.vlen) {
        if (input_sig.vlen != output_sig.vlen) {
            return false;
        }
    }

    /// Packet sizes
    if (output_sig.packet_size and input_sig.packet_size) {
        if (input_sig.packet_size != output_sig.packet_size) {
            return false;
        }
    }

    // You may pass
    return true;
}
// vim: sw=4 et:
