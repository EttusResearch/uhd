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
     * Includes time specification, fragmentation flags, burst flags, and error codes.
     * The receive routines will convert IF data headers into metadata.
     */
    struct UHD_API rx_metadata_t{
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
         */
        bool start_of_burst;
        bool end_of_burst;

        /*!
         * Error conditions:
         * - none: no error associated with this metadata
         * - timeout: no packet received, underlying code timed-out
         * - late command: a stream command was issued in the past
         * - broken chain: expected another stream command
         * - overflow: an internal receive buffer has filled
         * - bad packet: the buffer was unrecognizable as a vrt packet
         *
         * Note: When an overrun occurs in continuous streaming mode,
         * the device will continue to send samples to the host.
         * For other streaming modes, streaming will discontinue
         * until the user issues a new stream command.
         *
         * Note: The metadata fields have meaning for the following error codes:
         * - none
         * - late command
         * - broken chain
         * - overflow
         */
        enum error_code_t {
            ERROR_CODE_NONE         = 0x0,
            ERROR_CODE_TIMEOUT      = 0x1,
            ERROR_CODE_LATE_COMMAND = 0x2,
            ERROR_CODE_BROKEN_CHAIN = 0x4,
            ERROR_CODE_OVERFLOW     = 0x8,
            ERROR_CODE_BAD_PACKET   = 0xf
        } error_code;
    };

    /*!
     * TX metadata structure for describing received IF data.
     * Includes time specification, and start and stop burst flags.
     * The send routines will convert the metadata to IF data headers.
     */
    struct UHD_API tx_metadata_t{
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

    /*!
     * Async metadata structure for describing transmit related events.
     */
    struct UHD_API async_metadata_t{
        //! The channel number in a mimo configuration
        size_t channel;

        /*!
         * Time specification: when the async event occurred.
         */
        bool has_time_spec;
        time_spec_t time_spec;

        /*!
         * Event codes:
         * - success: a packet was successfully transmitted
         * - underflow: an internal send buffer has emptied
         * - sequence error: packet loss between host and device
         * - time error: packet had time that was late (or too early)
         * - underflow in packet: underflow occurred inside a packet
         * - sequence error in burst: packet loss within a burst
         */
        enum event_code_t {
            EVENT_CODE_SUCCESS    = 0x1,
            EVENT_CODE_UNDERFLOW  = 0x2,
            EVENT_CODE_SEQ_ERROR  = 0x4,
            EVENT_CODE_TIME_ERROR = 0x8,
            EVENT_CODE_UNDERFLOW_IN_PACKET = 0x10,
            EVENT_CODE_SEQ_ERROR_IN_BURST  = 0x20
        } event_code;
    };

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_METADATA_HPP */
