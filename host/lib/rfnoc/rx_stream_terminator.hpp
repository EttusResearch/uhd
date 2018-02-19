//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RFNOC_TERMINATOR_RECV_HPP
#define INCLUDED_LIBUHD_RFNOC_TERMINATOR_RECV_HPP

#include <uhd/rfnoc/sink_node_ctrl.hpp>
#include <uhd/rfnoc/rate_node_ctrl.hpp>
#include <uhd/rfnoc/tick_node_ctrl.hpp>
#include <uhd/rfnoc/scalar_node_ctrl.hpp>
#include <uhd/rfnoc/terminator_node_ctrl.hpp>
#include <uhd/rfnoc/block_ctrl_base.hpp> // For the block macros
#include <mutex>

namespace uhd {
    namespace rfnoc {

/*! \brief Terminator node for Rx streamers.
 *
 * This node is only used by rx_streamers. It terminates the flow graph
 * inside the streamer and does not have a counterpart on the FPGA.
 */
class rx_stream_terminator :
    public sink_node_ctrl,
    public rate_node_ctrl,
    public tick_node_ctrl,
    public scalar_node_ctrl,
    public terminator_node_ctrl
{
public:
    UHD_RFNOC_BLOCK_OBJECT(rx_stream_terminator)

    static sptr make()
    {
        return sptr(new rx_stream_terminator);
    }

    // If this is called, then by a send terminator at the other end
    // of a flow graph.
    double get_input_samp_rate(size_t) { return _samp_rate; };

    // Same for the scaling factor
    double get_input_scale_factor(size_t) { return scalar_node_ctrl::SCALE_UNDEFINED; };

    std::string unique_id() const;

    void set_rx_streamer(bool active, const size_t port);

    void set_tx_streamer(bool active, const size_t port);

    virtual ~rx_stream_terminator();

    void handle_overrun(boost::weak_ptr<uhd::rx_streamer>, const size_t);

protected:
    rx_stream_terminator();

    virtual double _get_tick_rate() { return _tick_rate; };

private:
    //! Every terminator has a unique index
    const size_t _term_index;
    static size_t _count;

    double _samp_rate;
    double _tick_rate;

    std::mutex _overrun_handler_mutex;

}; /* class rx_stream_terminator */

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_TERMINATOR_RECV_HPP */
// vim: sw=4 et:
