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

#ifndef INCLUDED_LIBUHD_RFNOC_STREAMSIG_HPP
#define INCLUDED_LIBUHD_RFNOC_STREAMSIG_HPP

#include <iostream>
#include <uhd/config.hpp>

namespace uhd { namespace rfnoc {

/*! Describes a stream signature for data going to or coming from
 * RFNoC ports.
 */
class UHD_API stream_sig_t {
 public:
     //! A special value for the data type, if this is selected,
     // a transparent type is assumed (i.e. we use whatever is
     // upstream).
    static const std::string PASSTHRU_TYPE;
    stream_sig_t();
    stream_sig_t(std::string, size_t vlen, size_t packet_size=0, bool is_bursty = false);

    // Getters

    //! Compact string representation
    std::string to_string();
    //! Pretty-print string representation
    std::string to_pp_string();

    //! Returns the number of bytes necessary to store one item.
    // Note: The vector length is *not* considered here.
    size_t get_bytes_per_item() const;
    std::string get_item_type() const { return _item_type; };

    size_t get_packet_size() const { return packet_size; };

    // Setters

    //! Will throw if \p type is invalid.
    void set_item_type(const std::string &type);

    // Utilities

    /*! Check if an output with signature \p output_sig could
     * stream to input with this signature.
     */
    bool is_compatible(const stream_sig_t &output_sig) const;

    // Attributes

    size_t vlen;
    //! Packet size in bytes
    size_t packet_size;
    bool is_bursty;

 private:
    std::string _item_type;
};

//! Shortcut for << stream_sig.to_string()
inline std::ostream& operator<< (std::ostream& out, stream_sig_t stream_sig) {
    out << stream_sig.to_string();
    return out;
}

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_STREAMSIG_HPP */
// vim: sw=4 et:
