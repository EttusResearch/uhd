//
// Copyright 2012-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RFNOC_CTRL_IFACE_HPP
#define INCLUDED_LIBUHD_RFNOC_CTRL_IFACE_HPP

#include "xports.hpp"
#include <boost/shared_ptr.hpp>
#include <string>

namespace uhd { namespace rfnoc {

/*!
 * Provide read/write access to registers on an RFNoC block via Noc-Shell.
 */
class ctrl_iface
{
public:
    typedef boost::shared_ptr<ctrl_iface> sptr;
    virtual ~ctrl_iface(void) {}

    /*! Make a new control object
     *
     * \param xports Bidirectional transport object to the RFNoC block port.
     * \param name Optional name for better identification in error messages.
     */
    static sptr make(
        const both_xports_t &xports,
        const std::string &name="0"
    );

    /*! Send a command packet.
     *
     * \param addr Register address. This is the value that gets put into the
     *             command packet, its interpretation is defined on the FPGA.
     * \param data Register value to write.
     * \param readback If true, assume the command packet is for a readback,
     *                 and wait for a response packet to return. The return
     *                 value will then be the 64-bit payload of that response
     *                 packet. If false, the return value is the payload of
     *                 any outstanding ACK packet.
     * \param timestamp Optional timestamp. The command packet will include this
     *                  timestamp. Depending on the block configuration, this
     *                  can trigger timed commands.
     *                  A value of zero indicates that no timestamp will be
     *                  applied. It is not possible to request anything to
     *                  happen at time zero.
     *
     * \throws uhd::io_error if the response is malformed; uhd::runtime_error if
     *         no packet could be sent.
     */
    virtual uint64_t send_cmd_pkt(
            const size_t addr,
            const size_t data,
            const bool readback=false,
            const uint64_t timestamp=0
    ) = 0;
};

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_CTRL_IFACE_HPP */
