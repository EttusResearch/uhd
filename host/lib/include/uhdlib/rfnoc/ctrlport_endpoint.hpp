//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/rfnoc/chdr_types.hpp>
#include <uhd/rfnoc/register_iface.hpp>
#include <uhdlib/rfnoc/clock_iface.hpp>
#include <memory>

namespace uhd { namespace rfnoc {

/*!  A software interface that represents a ControlPort endpoint
 *
 * This interface supports the following:
 * - All capabilities of register_iface
 * - A function to handle received packets
 * - A static factory class to create these endpoints
 */
class ctrlport_endpoint : public register_iface
{
public:
    using sptr = std::shared_ptr<ctrlport_endpoint>;

    //! The function to call when sending a packet to a remote device
    using send_fn_t = std::function<void(const chdr::ctrl_payload&, double)>;

    ~ctrlport_endpoint() override = 0;

    //! Handles an incoming control packet (request and response)
    //
    // \param rx_ctrl The control payload of the received packet
    //
    virtual void handle_recv(const chdr::ctrl_payload& rx_ctrl) = 0;

    //! Creates a new register interface (ctrl_portendpoint)
    //
    // \param handle_send The function to call to send a control packet
    // \param my_epid The endpoint ID of the SW control stream endpoint
    // \param port The port number on the control crossbar
    // \param buff_capacity The buffer capacity of the downstream buff in 32-bit words
    // \param max_outstanding_async_msgs Max outstanding async messages allowed
    // \param ctrl_clk_freq Frequency of the clock driving the ctrlport logic
    // \param timebase_freq Frequency of the timebase (for timed commands)
    //
    static sptr make(const send_fn_t& handle_send,
        sep_id_t my_epid,
        uint16_t local_port,
        size_t buff_capacity,
        size_t max_outstanding_async_msgs,
        const clock_iface& client_clk,
        const clock_iface& timebase_clk);

}; // class ctrlport_endpoint

}} /* namespace uhd::rfnoc */
