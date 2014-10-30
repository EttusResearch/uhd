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

#ifndef INCLUDED_LIBUHD_RFNOC_NULL_BLOCK_CTRL_HPP
#define INCLUDED_LIBUHD_RFNOC_NULL_BLOCK_CTRL_HPP

#include <uhd/usrp/rfnoc/rx_block_ctrl_base.hpp>

namespace uhd {
    namespace rfnoc {

/*! \brief Provide access to a 'null block'.
 *
 * A 'null block' is a specific block, which comes with a couple
 * of features useful for testing:
 * - It can produce data at a given line rate, with a configurable
 *   packet size.
 * - It can be used to dump packets ("null sink", "bit bucket")
 *
 * This block also serves as an example of how to create your own
 * C++ classes to control your block. This class both has functions
 * that inherit from uhd::rfnoc::block_ctrl_base, as well as
 * functions that are specific to this block.
 */
class null_block_ctrl : public rx_block_ctrl_base
{
public:
    // This macro must always be at the top of the public section in an RFNoC block class
    UHD_RFNOC_BLOCK_OBJECT(null_block_ctrl)

    //! Set this register to number of lines per packet
    static const boost::uint32_t SR_LINES_PER_PACKET = 9;
    //! Set this register to number of cycles between producing a line
    static const boost::uint32_t SR_LINE_RATE = 10;
    //! Set this register to non-zero to start producing data
    static const boost::uint32_t SR_ENABLE_STREAM = 11;

    static const size_t DEFAULT_LINES_PER_PACKET = 32;
    static const size_t BYTES_PER_LINE = 8;

    //! Custom function to set the rate at which data is produced.
    // Note: This is 'cycles per line', so the bit rate is actually
    // 64 times this value (byte/s is 8*rate etc.)
    //
    // Equivalent to writing to line_rate/value in the property tree.
    //
    // \param The rate you want to set this to
    // \param The clock rate of this block's clock domain
    // \returns the actual line rate (will find closest possible).
    virtual double set_line_rate(double rate, double clock_rate=166.6e6) = 0;

    //! Return the current line rate. Equivalent to reading line_rate/value
    // from the property tree.
    virtual double get_line_rate(double clock_rate=166.6e6) const = 0;

    //! This block can actually initiate streaming, so we need
    // override this. It supports STREAM_MODE_START_CONTINUOUS and
    // STREAM_MODE_STOP_CONTINUOUS. Other stream modes will cause
    // a uhd::not_implemented_error to be thrown.
    virtual void issue_stream_cmd(const uhd::stream_cmd_t &stream_cmd) = 0;

    virtual bool set_output_signature(const stream_sig_t &, size_t port=0) = 0;

    //! This must be overridden because as a true source, we must also
    // set the source address.
    virtual void set_destination(boost::uint32_t next_address, size_t output_block_port = 0) = 0;

}; /* class null_block_ctrl*/

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_NULL_BLOCK_CTRL_HPP */
// vim: sw=4 et:
