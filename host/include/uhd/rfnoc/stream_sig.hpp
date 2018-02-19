//
// Copyright 2014-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RFNOC_STREAMSIG_HPP
#define INCLUDED_LIBUHD_RFNOC_STREAMSIG_HPP

#include <iostream>
#include <uhd/config.hpp>

namespace uhd { namespace rfnoc {

/*! Describes a stream signature for data going to or coming from
 * RFNoC ports.
 *
 * The stream signature may depend on a block's configuration. Even
 * so, some attributes may be left undefined (e.g., a FIFO block
 * works for any item type, so it doesn't need to set it).
 */
class UHD_RFNOC_API stream_sig_t {
 public:
    /***********************************************************************
     * Structors
     ***********************************************************************/
    stream_sig_t();

    /***********************************************************************
     * The stream signature attributes
     ***********************************************************************/
    //! The data type of the individual items (e.g. 'sc16'). If undefined, set
    // to empty.
    std::string item_type;

    //! The vector length in multiples of items. If undefined, set to zero.
    size_t vlen;

    //! Packet size in bytes. If undefined, set to zero.
    size_t packet_size;

    bool is_bursty;

    /***********************************************************************
     * Helpers
     ***********************************************************************/
    //! Compact string representation
    std::string to_string();
    //! Pretty-print string representation
    std::string to_pp_string();

    //! Returns the number of bytes necessary to store one item.
    // Note: The vector length is *not* considered here.
    //
    // \returns Number of bytes per item or 0 if the item type is
    //          undefined.
    // \throws uhd::key_error if the item type is invalid.
    size_t get_bytes_per_item() const;

    /*! Check if an output with signature \p output_sig could
     * stream to an input signature \p input_sig.
     *
     * \return true if streams are compatible
     */
    static bool is_compatible(const stream_sig_t &output_sig, const stream_sig_t &input_sig);
};

//! Shortcut for << stream_sig.to_string()
UHD_INLINE std::ostream& operator<< (std::ostream& out, stream_sig_t stream_sig) {
    out << stream_sig.to_string().c_str();
    return out;
}

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_STREAMSIG_HPP */
// vim: sw=4 et:
