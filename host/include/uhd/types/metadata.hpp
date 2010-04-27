//
// Copyright 2010 Ettus Research LLC
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

#ifndef INCLUDED_UHD_TYPES_METADATA_HPP
#define INCLUDED_UHD_TYPES_METADATA_HPP

#include <uhd/config.hpp>
#include <boost/cstdint.hpp>
#include <uhd/types/time_spec.hpp>

namespace uhd{

    /*!
     * RX metadata structure for describing sent IF data.
     * Includes stream ID, time specification, and fragmentation flags.
     * The receive routines will convert IF data headers into metadata.
     */
    struct UHD_API rx_metadata_t{
        /*!
         * Stream IDs may be used to identify source DSP units.
         * --Not currently used in any known device implementation.--
         */
        bool has_stream_id;
        boost::uint32_t stream_id;

        /*!
         * Time specification:
         * Set from timestamps on incoming data when provided.
         */
        bool has_time_spec;
        time_spec_t time_spec;

        /*!
         * Fragmentation flag and offset:
         * Similar to IPv4 fragmentation: http://en.wikipedia.org/wiki/IPv4#Fragmentation_and_reassembly
         * More fragments is true when the input buffer has insufficient size to fit
         * an entire received packet. More fragments will be false for the last fragment.
         * The fragment offset is the sample number at the start of the receive buffer.
         * For non-fragmented receives, the fragment offset should always be zero.
         */
        bool more_fragments;
        size_t fragment_offset;

        /*!
         * Burst flags:
         * Start of burst will be true for the first packet in the chain.
         * End of burst will be true for the last packet in the chain.
         * --Not currently used in any known device implementation.--
         */
        //bool start_of_burst;
        //bool end_of_burst;

        /*!
         * Error conditions (TODO):
         * Previous packets dropped?
         * Timed-out on receive?
         */

        /*!
         * The default constructor:
         * Sets the fields to default values (flags set to false).
         */
        rx_metadata_t(void);
    };

    /*!
     * TX metadata structure for describing received IF data.
     * Includes stream ID, time specification, and burst flags.
     * The send routines will convert the metadata to IF data headers.
     */
    struct UHD_API tx_metadata_t{
        /*!
         * Stream IDs may be used to identify destination DSP units.
         * --Not currently used in any known device implementation.--
         */
        bool has_stream_id;
        boost::uint32_t stream_id;

        /*!
         * Time specification:
         * Set has time spec to false to perform a send "now".
         * Or, set to true, and fill in time spec for a send "at".
         */
        bool has_time_spec;
        time_spec_t time_spec;

        /*!
         * Burst flags:
         * Set start of burst to true for the first packet in the chain.
         * Set end of burst to true for the last packet in the chain.
         */
        bool start_of_burst;
        bool end_of_burst;

        /*!
         * The default constructor:
         * Sets the fields to default values (flags set to false).
         */
        tx_metadata_t(void);
    };

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_METADATA_HPP */
