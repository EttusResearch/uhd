//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhdlib/rfnoc/chdr_ctrl_xport.hpp>
#include <uhdlib/rfnoc/chdr_packet_writer.hpp>
#include <uhdlib/rfnoc/ctrlport_endpoint.hpp>
#include <functional>
#include <memory>

namespace uhd { namespace rfnoc {

/*! A software interface that represents a CHDR control endpoint
 *
 * This object is assigned an endpoint ID (EPID) and is responsible for sending/
 * receiving CHDR control packets, typically from a single device (in other
 * words, we spawn one of these for every USRP in our UHD session).
 *
 * All the RFNoC blocks on this device (as well as client zero) can create a
 * ctrlport_endpoint interface from this object (the ctrlport_endpoint object
 * is the thing that can do the actual peek/poke operations). This class will
 * handle the packaging and addressing of CHDR packets.
 *
 * The only place where we create a chdr_ctrl_endpoint (outside of tests) is
 * in link_stream_manager.cpp.
 *
 * Incoming packets (that originate from the device and reach this chdr_ctrl_endpoint)
 * are mapped to their intended destination (the desired ctrlport_endpoint
 * instance) by their source EPID and the destination control crossbar port.
 *
 *  The endpoint is capable of sending/receiving CHDR packets
 *  and creating multiple ctrlport_endpoint interfaces
 *
 *  Typically, there is one chdr_ctrl_endpoint for every device.
 */
class chdr_ctrl_endpoint
{
public:
    using uptr = std::unique_ptr<chdr_ctrl_endpoint>;

    virtual ~chdr_ctrl_endpoint() = 0;

    //! Creates a new register interface for the specified port
    //
    // \param dst_epid The endpoint ID of the remote block for which this
    //                 controlport endpoint object is designed for
    // \param dst_port The port number on the control crossbar
    // \param buff_capacity The buffer capacity of the downstream buff in 32-bit words
    // \param max_outstanding_async_msgs Max outstanding async messages allowed
    // \param client_clk Frequency of the clock driving the ctrlport logic
    // \param timebase_clk Frequency of the timebase (for timed commands)
    //
    virtual ctrlport_endpoint::sptr get_ctrlport_ep(sep_id_t dst_epid,
        uint16_t dst_port,
        size_t buff_capacity,
        size_t max_outstanding_async_msgs,
        const clock_iface& client_clk,
        const clock_iface& timebase_clk) = 0;

    //! Returns the number of dropped packets due to misclassification
    //
    // This includes packets that are not valid CHDR as well as packets that
    // are valid CHDR, but have no known/valid endpoint. Correctly classified
    // packets may still be invalid, but they don't get counted here, they are
    // handled in ctrlport_endpoint.
    virtual size_t get_num_drops() const = 0;

    //! Creates a control endpoint object
    //
    // \param xport The transport used to send and recv packets
    // \param pkt_factor An instance of the CHDR packet factory
    // \param my_epid The endpoint ID of this software endpoint
    //
    static uptr make(chdr_ctrl_xport::sptr xport,
        const chdr::chdr_packet_factory& pkt_factory,
        sep_id_t my_epid);

}; // class chdr_ctrl_endpoint

}} /* namespace uhd::rfnoc */
