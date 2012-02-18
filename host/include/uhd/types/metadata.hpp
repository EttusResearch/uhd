//
// Copyright 2010-2012 Ettus Research LLC
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
#include <uhd/types/time_spec.hpp>
#include <boost/cstdint.hpp>

namespace uhd{

    /*!
     * RX metadata structure for describing sent IF data.
     * Includes time specification, fragmentation flags, burst flags, and error codes.
     * The receive routines will convert IF data headers into metadata.
     */
    struct UHD_API rx_metadata_t{
        //! Has time specification?
        bool has_time_spec;

        //! Time of the first sample.
        time_spec_t time_spec;

        /*!
         * Fragmentation flag:
         * Similar to IPv4 fragmentation: http://en.wikipedia.org/wiki/IPv4#Fragmentation_and_reassembly
         * More fragments is true when the input buffer has insufficient size to fit
         * an entire received packet. More fragments will be false for the last fragment.
         */
        bool more_fragments;

        /*!
         * Fragmentation offset:
         * The fragment offset is the sample number at the start of the receive buffer.
         * For non-fragmented receives, the fragment offset should always be zero.
         */
        size_t fragment_offset;

        //! Start of burst will be true for the first packet in the chain.
        bool start_of_burst;

        //! End of burst will be true for the last packet in the chain.
        bool end_of_burst;

        /*!
         * The error condition on a receive call.
         *
         * Note: When an overrun occurs in continuous streaming mode,
         * the device will continue to send samples to the host.
         * For other streaming modes, streaming will discontinue
         * until the user issues a new stream command.
         *
         * The metadata fields have meaning for the following error codes:
         * - none
         * - late command
         * - broken chain
         * - overflow
         */
        enum error_code_t {
            //! No error associated with this metadata.
            ERROR_CODE_NONE         = 0x0,
            //! No packet received, implementation timed-out.
            ERROR_CODE_TIMEOUT      = 0x1,
            //! A stream command was issued in the past.
            ERROR_CODE_LATE_COMMAND = 0x2,
            //! Expected another stream command.
            ERROR_CODE_BROKEN_CHAIN = 0x4,
            //! An internal receive buffer has filled.
            ERROR_CODE_OVERFLOW     = 0x8,
            //! Multi-channel alignment failed.
            ERROR_CODE_ALIGNMENT    = 0xc,
            //! The packet could not be parsed.
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
         * Has time specification?
         * - Set false to send immediately.
         * - Set true to send at the time specified by time spec.
         */
        bool has_time_spec;

        //! When to send the first sample.
        time_spec_t time_spec;

        //! Set start of burst to true for the first packet in the chain.
        bool start_of_burst;

        //! Set end of burst to true for the last packet in the chain.
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

        //! Has time specification?
        bool has_time_spec;

        //! When the async event occurred.
        time_spec_t time_spec;

        /*!
         * The type of event for a receive async message call.
         */
        enum event_code_t {
            //! A burst was successfully transmitted.
            EVENT_CODE_BURST_ACK  = 0x1,
            //! An internal send buffer has emptied.
            EVENT_CODE_UNDERFLOW  = 0x2,
            //! Packet loss between host and device.
            EVENT_CODE_SEQ_ERROR  = 0x4,
            //! Packet had time that was late.
            EVENT_CODE_TIME_ERROR = 0x8,
            //! Underflow occurred inside a packet.
            EVENT_CODE_UNDERFLOW_IN_PACKET = 0x10,
            //! Packet loss within a burst.
            EVENT_CODE_SEQ_ERROR_IN_BURST  = 0x20,
            //! Some kind of custom user payload
            EVENT_CODE_USER_PAYLOAD = 0x40
        } event_code;

        /*!
         * A special payload populated by custom FPGA fabric.
         */
        boost::uint32_t user_payload[4];

    };

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_METADATA_HPP */
