//
// Copyright 2014-2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RFNOC_NULL_BLOCK_CTRL_HPP
#define INCLUDED_LIBUHD_RFNOC_NULL_BLOCK_CTRL_HPP

#include <uhd/rfnoc/source_block_ctrl_base.hpp>
#include <uhd/rfnoc/sink_block_ctrl_base.hpp>

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
 * C++ classes to control your block.
 *
 * As a true source, it understands the following stream commands:
 * - STREAM_MODE_START_CONTINUOUS
 * - STREAM_MODE_STOP_CONTINUOUS
 *
 * Other stream commands are not understood and issue_stream_cmd()
 * will throw if it receives them.
 */
class null_block_ctrl : public source_block_ctrl_base, public sink_block_ctrl_base
{
public:
    // This macro must always be at the top of the public section in an RFNoC block class
    UHD_RFNOC_BLOCK_OBJECT(null_block_ctrl)

    //! Set this register to number of lines per packet
    static const uint32_t SR_LINES_PER_PACKET = 129;
    //! Set this register to number of cycles between producing a line
    static const uint32_t SR_LINE_RATE = 130;
    //! Set this register to non-zero to start producing data
    static const uint32_t SR_ENABLE_STREAM = 131;

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

}; /* class null_block_ctrl*/

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_NULL_BLOCK_CTRL_HPP */
// vim: sw=4 et:
