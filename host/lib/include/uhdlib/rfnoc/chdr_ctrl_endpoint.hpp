//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RFNOC_CHDR_CTRL_ENDPOINT_HPP
#define INCLUDED_LIBUHD_RFNOC_CHDR_CTRL_ENDPOINT_HPP

#include <uhdlib/rfnoc/chdr/chdr_packet.hpp>
#include <uhdlib/rfnoc/ctrlport_endpoint.hpp>
#include <uhdlib/rfnoc/xports.hpp>
#include <functional>
#include <memory>

namespace uhd { namespace rfnoc {

/*! A software interface that represents a CHDR control endpoint
 *
 *  The endpoint is capable of sending/receiving CHDR packets
 *  and creating multiple ctrlport_endpoint interfaces
 */
class chdr_ctrl_endpoint
{
public:
    using uptr = std::unique_ptr<chdr_ctrl_endpoint>;

    virtual ~chdr_ctrl_endpoint() = 0;

    //! Creates a new register interface for the specified port
    //
    // \param port The port number on the control crossbar
    // \param buff_capacity The buffer capacity of the downstream buff in 32-bit words
    // \param max_outstanding_async_msgs Max outstanding async messages allowed
    // \param ctrl_clk_freq Frequency of the clock driving the ctrlport logic
    // \param timebase_freq Frequency of the timebase (for timed commands)
    //
    virtual ctrlport_endpoint::sptr get_ctrlport_ep(uint16_t port,
        size_t buff_capacity,
        size_t max_outstanding_async_msgs,
        double ctrl_clk_freq,
        double timebase_freq) = 0;

    //! Returns the number of dropped packets due to misclassification
    virtual size_t get_num_drops() const = 0;

    //! Creates a control endpoint object
    //
    // \param xports The transports used to send and recv packets
    // \param pkt_factor An instance of the CHDR packet factory
    // \param my_epid The endpoint ID of this software endpoint
    //
    static uptr make(const both_xports_t& xports,
        const chdr::chdr_packet_factory& pkt_factory,
        sep_id_t dst_epid,
        sep_id_t my_epid);

}; // class chdr_ctrl_endpoint

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_CHDR_CTRL_ENDPOINT_HPP */
