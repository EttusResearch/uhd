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

#include <boost/format.hpp>
#include <uhd/exception.hpp>
#include <uhd/usrp/rfnoc/stream_sig.hpp>
#include <uhd/convert.hpp>

using namespace uhd::rfnoc;

const std::string stream_sig_t::PASSTHRU_TYPE = "pass";

stream_sig_t::stream_sig_t() :
    vlen(0),
    packet_size(0),
    is_bursty(false),
    _item_type("sc16")
{
    // nop
}
 
stream_sig_t::stream_sig_t(std::string type_, size_t vlen_, size_t packet_size_, bool is_bursty_) :
    vlen(vlen_),
    packet_size(packet_size_),
    is_bursty(is_bursty_),
    _item_type(type_)
{
    // nop
}

// Getters

std::string stream_sig_t::to_string()
{
    return str(
        boost::format(
            "%s,vlen=%d,packet_size=%d%s"
        ) % get_item_type() % vlen % packet_size % (is_bursty ? ",bursty" : "")
    );
}

std::string stream_sig_t::to_pp_string()
{
    return str(
        boost::format(
            "Data type: %s  | Vector Length: %d | Bursty: %s"
        ) % _item_type % vlen % (is_bursty ? "Yes" : "No")
    );
}

size_t stream_sig_t::get_bytes_per_item() const
{
    if (_item_type == "pass") {
        return 0;
    }

    return uhd::convert::get_bytes_per_item(_item_type);
}

// Setters

void stream_sig_t::set_item_type(const std::string &type)
{
    if (type == PASSTHRU_TYPE) {
        _item_type = PASSTHRU_TYPE;
        return;
    }

    // This will throw if type is not a valid type
    uhd::convert::get_bytes_per_item(type);

    _item_type = type;
}

// Utils

bool stream_sig_t::is_compatible(const stream_sig_t &output_sig) const
{
    // Item type must either be equal, or this signature must
    // be of type 'pass'
    if (output_sig.get_item_type() == PASSTHRU_TYPE) {
        throw uhd::runtime_error("Cannot check IO signature compatibility if upstream block has no type defined.");
    }

    // Either our own item type is not set (pass through), or
    // it's equal to the output_sig
    if (not(this->get_item_type() == PASSTHRU_TYPE
            or (this->get_item_type() == output_sig.get_item_type()))
    ) {
        return false;
    }

    // If our vlen value is set, then one of these must be true:
    // - Both vlens must be equal
    // - The packet size of the upstream block is equivalent to the vector size
    if (vlen != 0
        and (vlen != output_sig.vlen)
        and (vlen * get_bytes_per_item() != output_sig.get_packet_size())) {
        return false;
    }

    // If packet sizes are set on both signatures, they must be equal
    if (this->packet_size != 0
        and output_sig.packet_size != 0
        and (this->packet_size != output_sig.packet_size)) {
        return false;
    }

    // You may pass
    return true;
}
// vim: sw=4 et:
