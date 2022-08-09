//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>
#include <uhd/types/stream_cmd.hpp>

namespace uhd { namespace rfnoc {

/*! Null block: Bit bucket or -sink
 *
 * \ingroup rfnoc_blocks
 */
class UHD_API null_block_control : public noc_block_base
{
public:
    RFNOC_DECLARE_BLOCK(null_block_control)

    enum port_type_t { SINK, SOURCE, LOOP };
    enum count_type_t { LINES, PACKETS };

    static const uint32_t REG_CTRL_STATUS;
    static const uint32_t REG_SRC_LINES_PER_PKT;
    static const uint32_t REG_SRC_BYTES_PER_PKT;
    static const uint32_t REG_SRC_THROTTLE_CYC;
    static const uint32_t REG_SNK_LINE_CNT_LO;
    static const uint32_t REG_SNK_LINE_CNT_HI;
    static const uint32_t REG_SNK_PKT_CNT_LO;
    static const uint32_t REG_SNK_PKT_CNT_HI;
    static const uint32_t REG_SRC_LINE_CNT_LO;
    static const uint32_t REG_SRC_LINE_CNT_HI;
    static const uint32_t REG_SRC_PKT_CNT_LO;
    static const uint32_t REG_SRC_PKT_CNT_HI;
    static const uint32_t REG_LOOP_LINE_CNT_LO;
    static const uint32_t REG_LOOP_LINE_CNT_HI;
    static const uint32_t REG_LOOP_PKT_CNT_LO;
    static const uint32_t REG_LOOP_PKT_CNT_HI;

    /*! Start/stop the null source (port 0)
     */
    virtual void issue_stream_cmd(const uhd::stream_cmd_t& stream_cmd) = 0;

    /*! Reset the counters
     */
    virtual void reset_counters() = 0;

    /*! Set bytes per packet
     *
     * Note: This sets the entire packet size, including header.
     */
    virtual void set_bytes_per_packet(const uint32_t bpp) = 0;

    /*! Set throttle cycles
     *
     * The block will wait this many cycles between packets. Useful for reducing
     * the data output.
     */
    virtual void set_throttle_cycles(const uint32_t cycs) = 0;

    /*! Get item width (ITEM_W)
     */
    virtual uint32_t get_item_width() = 0;

    /*! Get number of items per clock (NIPC)
     */
    virtual uint32_t get_nipc() = 0;

    /*! Get lines per packet (including the header!)
     */
    virtual uint32_t get_lines_per_packet() = 0;

    /*! Get bytes per packet
     */
    virtual uint32_t get_bytes_per_packet() = 0;

    /*! Get throttle cycles
     */
    virtual uint32_t get_throttle_cycles() = 0;

    /*! Returns the number of lines that have been consumed on port 0 since the
     * last reset
     *
     * \param port_type The port for which the counter value should be returned,
     *        sink, source, or loop.
     * \param count_type If we want the packet count, or the line count
     */
    virtual uint64_t get_count(
        const port_type_t port_type, const count_type_t count_type) = 0;
};

}} // namespace uhd::rfnoc
