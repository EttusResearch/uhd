//
// Copyright 2014-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

// Provides streaming-related functions which are used by device3 objects.

#include "device3_flow_ctrl.hpp"
#include "device3_impl.hpp"
#include <uhd/rfnoc/constants.hpp>
#include <uhd/rfnoc/radio_ctrl.hpp>
#include <uhd/rfnoc/rate_node_ctrl.hpp>
#include <uhd/rfnoc/sink_block_ctrl_base.hpp>
#include <uhd/rfnoc/source_block_ctrl_base.hpp>
#include <uhd/transport/zero_copy_flow_ctrl.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/rfnoc/rx_stream_terminator.hpp>
#include <uhdlib/rfnoc/tx_stream_terminator.hpp>
#include <uhdlib/usrp/common/async_packet_handler.hpp>
#include <boost/atomic.hpp>

#define UHD_TX_STREAMER_LOG() UHD_LOGGER_TRACE("STREAMER")
#define UHD_RX_STREAMER_LOG() UHD_LOGGER_TRACE("STREAMER")

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;

/***********************************************************************
 * Helper functions for get_?x_stream()
 **********************************************************************/
static uhd::stream_args_t sanitize_stream_args(const uhd::stream_args_t& args_)
{
    uhd::stream_args_t args = args_;
    if (args.channels.empty()) {
        args.channels = std::vector<size_t>(1, 0);
    }

    return args;
}

static void check_stream_sig_compatible(
    const rfnoc::stream_sig_t& stream_sig, stream_args_t& args, const std::string& tx_rx)
{
    if (args.otw_format.empty()) {
        if (stream_sig.item_type.empty()) {
            throw uhd::runtime_error(
                str(boost::format("[%s Streamer] No otw_format defined!") % tx_rx));
        } else {
            args.otw_format = stream_sig.item_type;
        }
    } else if (not stream_sig.item_type.empty()
               and stream_sig.item_type != args.otw_format) {
        throw uhd::runtime_error(
            str(boost::format("[%s Streamer] Conflicting OTW types defined: "
                              "args.otw_format = '%s' <=> stream_sig.item_type = '%s'")
                % tx_rx % args.otw_format % stream_sig.item_type));
    }
    const size_t bpi = convert::get_bytes_per_item(args.otw_format); // bytes per item
    if (stream_sig.packet_size) {
        if (args.args.has_key("spp")) {
            size_t args_spp = args.args.cast<size_t>("spp", 0);
            if (args_spp * bpi != stream_sig.packet_size) {
                throw uhd::runtime_error(
                    str(boost::format(
                            "[%s Streamer] Conflicting packet sizes defined: args yields "
                            "%d bytes but stream_sig.packet_size is %d bytes")
                        % tx_rx % (args_spp * bpi) % stream_sig.packet_size));
            }
        } else {
            args.args["spp"] = str(boost::format("%d") % (stream_sig.packet_size / bpi));
        }
    }
}

/*! \brief Returns a list of rx or tx channels for a streamer.
 *
 * If the given stream args contain instructions to set up channels,
 * those are used. Otherwise, the current device's channel definition
 * is consulted.
 *
 * \param args_ Stream args.
 * \param[out] chan_list The list of channels in the correct order.
 * \param[out] chan_args Channel args for every channel. `chan_args.size() ==
 * chan_list.size()`
 */
void generate_channel_list(const uhd::stream_args_t& args_,
    std::vector<uhd::rfnoc::block_id_t>& chan_list,
    std::vector<device_addr_t>& chan_args)
{
    uhd::stream_args_t args = args_;
    std::vector<uhd::rfnoc::block_id_t> chan_list_(args.channels.size());
    std::vector<device_addr_t> chan_args_(args.channels.size());

    for (size_t i = 0; i < args.channels.size(); i++) {
        // Extract block ID
        size_t chan_idx = args.channels[i];
        std::string key = str(boost::format("block_id%d") % chan_idx);
        if (args.args.has_key(key)) {
            chan_list_[i] = args.args.pop(key);
        } else if (args.args.has_key("block_id")) {
            chan_list_[i] = args.args["block_id"];
        } else {
            throw uhd::runtime_error(
                str(boost::format(
                        "Cannot create streamers: No block_id specified for channel %d.")
                    % chan_idx));
        }

        // Split off known channel specific args
        key = str(boost::format("block_port%d") % chan_idx);
        if (args.args.has_key(key)) {
            chan_args_[i]["block_port"] = args.args.pop(key);
        }
        key = str(boost::format("radio_id%d") % chan_idx);
        if (args.args.has_key(key)) {
            chan_args_[i]["radio_id"] = args.args.pop(key);
        }
        key = str(boost::format("radio_port%d") % chan_idx);
        if (args.args.has_key(key)) {
            chan_args_[i]["radio_port"] = args.args.pop(key);
        }
    }

    // Add all remaining args to all channel args
    for (device_addr_t& chan_arg : chan_args_) {
        chan_arg = chan_arg.to_string() + "," + args.args.to_string();
    }

    chan_list = chan_list_;
    chan_args = chan_args_;
}


/***********************************************************************
 * RX Flow Control Functions
 **********************************************************************/
/*! Determine the size of the flow control window in number of packets.
 *
 * This value depends on three things:
 * - The packet size (in bytes), P
 * - The size of the software buffer (in bytes), B
 * - The desired buffer fullness, F
 *
 * The FC window size is thus X = floor(B*F/P).
 *
 * \param pkt_size The maximum packet size in bytes
 * \param sw_buff_size Software buffer size in bytes
 * \param rx_args If this has a key 'recv_buff_fullness', this value will
 *                be used for said fullness. Must be between 0.01 and 1.
 *
 *  \returns The size of the flow control window in number of packets
 */
static size_t get_rx_flow_control_window(
    size_t pkt_size, size_t sw_buff_size, const device_addr_t& rx_args)
{
    double fullness_factor = rx_args.cast<double>(
        "recv_buff_fullness", uhd::rfnoc::DEFAULT_FC_RX_SW_BUFF_FULL_FACTOR);

    if (fullness_factor < 0.01 || fullness_factor > 1) {
        throw uhd::value_error(
            "recv_buff_fullness must be in [0.01, 1] inclusive (1% to 100%)");
    }

    size_t window_in_bytes = (static_cast<size_t>(fullness_factor * sw_buff_size));
    if (rx_args.has_key("max_recv_window")) {
        window_in_bytes = std::min(
            window_in_bytes,
            pkt_size * rx_args.cast<size_t>("max_recv_window", 1)
        );
    }
    if (window_in_bytes < pkt_size) {
        throw uhd::value_error("recv_buff_size must be larger than the recv_frame_size.");
    }
    UHD_ASSERT_THROW(size_t(sw_buff_size * fullness_factor) >= window_in_bytes);
    return window_in_bytes;
}


/***********************************************************************
 * TX Async Message Functions
 **********************************************************************/
#define DEVICE3_ASYNC_EVENT_CODE_FLOW_CTRL 0

struct async_tx_info_t
{
    size_t stream_channel;
    size_t device_channel;
    boost::shared_ptr<device3_impl::async_md_type> async_queue;
    boost::shared_ptr<device3_impl::async_md_type> old_async_queue;
};

/*! Handle incoming messages.
 *  Send them to the async message queue for the user to poll.
 *
 * This is run inside a uhd::task as long as this streamer lives.
 */
static void handle_tx_async_msgs(boost::shared_ptr<async_tx_info_t> async_info,
    zero_copy_if::sptr xport,
    uint32_t (*to_host)(uint32_t),
    void (*unpack)(const uint32_t* packet_buff, vrt::if_packet_info_t&),
    boost::function<double(void)> get_tick_rate)
{
    managed_recv_buffer::sptr buff = xport->get_recv_buff();
    if (not buff) {
        return;
    }

    // extract packet info
    vrt::if_packet_info_t if_packet_info;
    if_packet_info.num_packet_words32 = buff->size() / sizeof(uint32_t);
    const uint32_t* packet_buff       = buff->cast<const uint32_t*>();

    // unpacking can fail
    try {
        unpack(packet_buff, if_packet_info);
    } catch (const std::exception& ex) {
        UHD_LOGGER_ERROR("STREAMER")
            << "Error parsing async message packet: " << ex.what();
        return;
    }

    double tick_rate = get_tick_rate();
    if (tick_rate == rfnoc::tick_node_ctrl::RATE_UNDEFINED) {
        tick_rate = 1;
    }

    // fill in the async metadata
    async_metadata_t metadata;
    load_metadata_from_buff(to_host,
        metadata,
        if_packet_info,
        packet_buff,
        tick_rate,
        async_info->stream_channel);

    // Filter out any flow control messages and cache the rest
    if (metadata.event_code == DEVICE3_ASYNC_EVENT_CODE_FLOW_CTRL) {
        UHD_LOGGER_ERROR("TX ASYNC MSG")
            << "Unexpected flow control message found in async message handling"
            << std::endl;
    } else {
        async_info->async_queue->push_with_pop_on_full(metadata);
        metadata.channel = async_info->device_channel;
        async_info->old_async_queue->push_with_pop_on_full(metadata);
        standard_async_msg_prints(metadata);
    }
}

bool device3_impl::recv_async_msg(async_metadata_t& async_metadata, double timeout)
{
    return _async_md->pop_with_timed_wait(async_metadata, timeout);
}

/***********************************************************************
 * Receive streamer
 **********************************************************************/
void device3_impl::update_rx_streamers()
{
    for (const std::string& block_id : _rx_streamers.keys()) {
        UHD_RX_STREAMER_LOG() << "updating RX streamer to " << block_id;
        boost::shared_ptr<device3_recv_packet_streamer> my_streamer =
            boost::dynamic_pointer_cast<device3_recv_packet_streamer>(
                _rx_streamers[block_id].lock());
        if (my_streamer) {
            double tick_rate = my_streamer->get_terminator()->get_tick_rate();
            if (tick_rate == rfnoc::tick_node_ctrl::RATE_UNDEFINED) {
                tick_rate = 1.0;
            }
            double samp_rate = my_streamer->get_terminator()->get_output_samp_rate();
            if (samp_rate == rfnoc::rate_node_ctrl::RATE_UNDEFINED) {
                samp_rate = 1.0;
            }
            double scaling = my_streamer->get_terminator()->get_output_scale_factor();
            if (scaling == rfnoc::scalar_node_ctrl::SCALE_UNDEFINED) {
                scaling = 1 / 32767.;
            }
            UHD_RX_STREAMER_LOG()
                << "  New tick_rate == " << tick_rate
                << "  New samp_rate == " << samp_rate << " New scaling == " << scaling;

            my_streamer->set_tick_rate(tick_rate);
            my_streamer->set_samp_rate(samp_rate);
            my_streamer->set_scale_factor(scaling);
        }
    }
}

rx_streamer::sptr device3_impl::get_rx_stream(const stream_args_t& args_)
{
    boost::mutex::scoped_lock lock(_transport_setup_mutex);
    stream_args_t args = sanitize_stream_args(args_);

    // I. Generate the channel list
    std::vector<uhd::rfnoc::block_id_t> chan_list;
    std::vector<device_addr_t> chan_args;
    generate_channel_list(args, chan_list, chan_args);
    // Note: All 'args.args' are merged into chan_args now.

    // II. Iterate over all channels
    boost::shared_ptr<device3_recv_packet_streamer> my_streamer;
    // The terminator's lifetime is coupled to the streamer.
    // There is only one terminator. If the streamer has multiple channels,
    // it will be connected to each upstream block.
    rfnoc::rx_stream_terminator::sptr recv_terminator =
        rfnoc::rx_stream_terminator::make();
    for (size_t stream_i = 0; stream_i < chan_list.size(); stream_i++) {
        // First, configure blocks and create transport

        // Get block ID and mb index
        uhd::rfnoc::block_id_t block_id = chan_list[stream_i];
        UHD_RX_STREAMER_LOG() << "chan " << stream_i << " connecting to " << block_id;
        // Update args so args.args is always valid for this particular channel:
        args.args       = chan_args[stream_i];
        size_t mb_index = block_id.get_device_no();
        size_t suggested_block_port =
            args.args.cast<size_t>("block_port", rfnoc::ANY_PORT);

        // Access to this channel's block control
        uhd::rfnoc::source_block_ctrl_base::sptr blk_ctrl =
            boost::dynamic_pointer_cast<uhd::rfnoc::source_block_ctrl_base>(
                get_block_ctrl(block_id));

        // Connect the terminator with this channel's block.
        size_t block_port = blk_ctrl->connect_downstream(
            recv_terminator, suggested_block_port, args.args);
        const size_t terminator_port = recv_terminator->connect_upstream(blk_ctrl);
        blk_ctrl->set_downstream_port(block_port, terminator_port);
        recv_terminator->set_upstream_port(terminator_port, block_port);

        // Check if the block connection is compatible (spp and item type)
        check_stream_sig_compatible(
            blk_ctrl->get_output_signature(block_port), args, "RX");

        // Setup the DSP transport hints
        device_addr_t rx_hints = get_rx_hints(mb_index);

        // Search the device and all nodes for lowest MTU
        size_t mtu = std::min(
            get_mtu(mb_index, uhd::direction_t::RX_DIRECTION),
            blk_ctrl->get_mtu(block_port));
        UHD_RX_STREAMER_LOG() << "Maximum MTU supported by "
                              <<  blk_ctrl->unique_id()
                              << ": " << blk_ctrl->get_mtu(block_port);
        std::vector<boost::shared_ptr<uhd::rfnoc::source_block_ctrl_base>>
            upstream_source_nodes =
                blk_ctrl->find_upstream_node<uhd::rfnoc::source_block_ctrl_base>();
        for (const boost::shared_ptr<uhd::rfnoc::source_block_ctrl_base>& node :
            upstream_source_nodes) {
            // Get MTU from Port 0 of the upstream nodes. This is okay for now as
            // currently we use port 0 of a block in case of channel 1.
            UHD_RX_STREAMER_LOG() << "Maximum MTU supported by " << node->unique_id()
                                  << ": " << node->get_mtu(0);
            mtu = std::min(mtu, node->get_mtu(0));
        }
        rx_hints["mtu"] = std::to_string(mtu);

        // Make sure user supplied recv_frame_size is less than the MTU
        if (rx_hints.cast<size_t>("recv_frame_size", mtu) > mtu) {
            UHD_LOGGER_WARNING("STREAMER")
                << "Requested recv_frame_size of "
                << rx_hints["recv_frame_size"]
                << " exceeds the maximum possible on this stream. Using "
                << mtu;
            rx_hints["recv_frame_size"] = std::to_string(mtu);
        }

        // allocate sid and create transport
        uhd::sid_t stream_address = blk_ctrl->get_address(block_port);
        UHD_RX_STREAMER_LOG() << "creating rx stream " << rx_hints.to_string();
        both_xports_t xport = make_transport(stream_address, RX_DATA, rx_hints);
        UHD_RX_STREAMER_LOG() << std::hex << "data_sid = " << xport.send_sid << std::dec
                              << " actual recv_buff_size = " << xport.recv_buff_size;

        // Configure the block
        // Flow control setup
        const size_t pkt_size = xport.recv->get_recv_frame_size();
        // Leave one pkt_size space for overrun packets - TODO make this obsolete
        const size_t fc_window =
            get_rx_flow_control_window(pkt_size, xport.recv_buff_size, rx_hints)
            - pkt_size;
        const size_t fc_handle_window =
            std::max<size_t>(pkt_size, fc_window / stream_options.rx_fc_request_freq);
        UHD_RX_STREAMER_LOG() << "Flow Control Window = " << (fc_window)
                              << ", Flow Control Handler Window = " << fc_handle_window;
        blk_ctrl->configure_flow_control_out(true,
            xport.lossless,
            fc_window,
            rx_hints.cast<size_t>("recv_pkt_limit",
                0), // On rfnoc-devel, update e300_impl::get_rx_hints() to set this to 32
            block_port);

        // Add flow control transport
        boost::shared_ptr<rx_fc_cache_t> fc_cache(new rx_fc_cache_t());
        fc_cache->sid      = xport.send_sid;
        fc_cache->xport    = xport.send;
        fc_cache->interval = fc_handle_window;
        if (xport.endianness == ENDIANNESS_BIG) {
            fc_cache->to_host   = uhd::ntohx<uint32_t>;
            fc_cache->from_host = uhd::htonx<uint32_t>;
            fc_cache->pack      = vrt::chdr::if_hdr_pack_be;
            fc_cache->unpack    = vrt::chdr::if_hdr_unpack_be;
        } else {
            fc_cache->to_host   = uhd::wtohx<uint32_t>;
            fc_cache->from_host = uhd::htowx<uint32_t>;
            fc_cache->pack      = vrt::chdr::if_hdr_pack_le;
            fc_cache->unpack    = vrt::chdr::if_hdr_unpack_le;
        }
        xport.recv = zero_copy_flow_ctrl::make(
            xport.recv, 0, [fc_cache](managed_buffer::sptr buff) {
                return rx_flow_ctrl(fc_cache, buff);
            });

        // Configure the block
        // Note: We need to set_destination() after writing to SR_CLEAR_TX_FC.
        // See noc_shell.v, in the section called Stream Source for details.
        // Setting SR_CLEAR_TX_FC will actually also clear the destination and
        // other settings.
        blk_ctrl->sr_write(uhd::rfnoc::SR_CLEAR_TX_FC, 0x1, block_port);
        blk_ctrl->sr_write(uhd::rfnoc::SR_CLEAR_TX_FC, 0x0, block_port);
        // Configure routing for data
        blk_ctrl->set_destination(xport.send_sid.get_src(), block_port);

        // Configure routing for responses
        blk_ctrl->sr_write(
            uhd::rfnoc::SR_RESP_OUT_DST_SID, xport.send_sid.get_src(), block_port);
        UHD_RX_STREAMER_LOG() << "resp_out_dst_sid == " << xport.send_sid.get_src();

        // Find all upstream radio nodes and set their response in SID to the host
        std::vector<boost::shared_ptr<uhd::rfnoc::radio_ctrl>> upstream_radio_nodes =
            blk_ctrl->find_upstream_node<uhd::rfnoc::radio_ctrl>();
        UHD_RX_STREAMER_LOG() << "Number of upstream radio nodes: "
                              << upstream_radio_nodes.size();
        for (const boost::shared_ptr<uhd::rfnoc::radio_ctrl>& node :
            upstream_radio_nodes) {
            node->sr_write(
                uhd::rfnoc::SR_RESP_OUT_DST_SID, xport.send_sid.get_src(), block_port);
        }

        // Second, configure the streamer

        // make the new streamer given the samples per packet
        if (not my_streamer) {
            // To calculate the max number of samples per packet, we assume the maximum
            // header length to avoid fragmentation should the entire header be used.
            const size_t bpp =
                pkt_size - stream_options.rx_max_len_hdr; // bytes per packet
            const size_t bpi =
                convert::get_bytes_per_item(args.otw_format); // bytes per item
            const size_t spp = std::min(args.args.cast<size_t>("spp", bpp / bpi),
                bpp / bpi); // samples per packet
            UHD_RX_STREAMER_LOG() << "bpp == " << bpp << ", bpi == " << bpi << ", spp == " << spp;

            my_streamer = boost::make_shared<device3_recv_packet_streamer>(
                spp, recv_terminator, xport);
            my_streamer->resize(chan_list.size());
        }

        // init some streamer stuff
        std::string conv_endianness;
        if (xport.endianness == ENDIANNESS_BIG) {
            my_streamer->set_vrt_unpacker(&vrt::chdr::if_hdr_unpack_be);
            conv_endianness = "be";
        } else {
            my_streamer->set_vrt_unpacker(&vrt::chdr::if_hdr_unpack_le);
            conv_endianness = "le";
        }

        // set the converter
        uhd::convert::id_type id;
        id.input_format  = args.otw_format + "_item32_" + conv_endianness;
        id.num_inputs    = 1;
        id.output_format = args.cpu_format;
        id.num_outputs   = 1;
        my_streamer->set_converter(id);

        // Give the streamer a functor to handle flow control ACK messages
        my_streamer->set_xport_handle_flowctrl_ack(
            stream_i, [fc_cache](const uint32_t* payload) {
                handle_rx_flowctrl_ack(fc_cache, payload);
            });

        // Give the streamer a functor to get the recv_buffer
        my_streamer->set_xport_chan_get_buff(stream_i,
            [xport](double timeout) { return xport.recv->get_recv_buff(timeout); },
            true /*flush*/
        );

        // Give the streamer a functor to handle overruns
        // bind requires a weak_ptr to break the a streamer->streamer circular dependency
        // Using "this" is OK because we know that this device3_impl will outlive the
        // streamer
        boost::weak_ptr<uhd::rx_streamer> weak_ptr(my_streamer);
        my_streamer->set_overflow_handler(
            stream_i, [recv_terminator, weak_ptr, stream_i]() {
                recv_terminator->handle_overrun(weak_ptr, stream_i);
            });

        // Give the streamer a functor issue stream cmd
        my_streamer->set_issue_stream_cmd(
            stream_i, [blk_ctrl, block_port](const stream_cmd_t& stream_cmd) {
                blk_ctrl->issue_stream_cmd(stream_cmd, block_port);
            });
    }

    // Notify all blocks in this chain that they are connected to an active streamer
    recv_terminator->set_rx_streamer(true, 0);

    // Store a weak pointer to prevent a streamer->device3_impl->streamer circular
    // dependency. Note that we store the streamer only once, and use its terminator's ID
    // to do so.
    _rx_streamers[recv_terminator->unique_id()] =
        boost::weak_ptr<uhd::rx_streamer>(my_streamer);

    // Sets tick rate, samp rate and scaling on this streamer.
    // A registered terminator is required to do this.
    update_rx_streamers();

    post_streamer_hooks(RX_DIRECTION);
    return my_streamer;
}

/***********************************************************************
 * Transmit streamer
 **********************************************************************/
void device3_impl::update_tx_streamers()
{
    for (const std::string& block_id : _tx_streamers.keys()) {
        UHD_TX_STREAMER_LOG() << "updating TX streamer: " << block_id;
        boost::shared_ptr<device3_send_packet_streamer> my_streamer =
            boost::dynamic_pointer_cast<device3_send_packet_streamer>(
                _tx_streamers[block_id].lock());
        if (my_streamer) {
            double tick_rate = my_streamer->get_terminator()->get_tick_rate();
            if (tick_rate == rfnoc::tick_node_ctrl::RATE_UNDEFINED) {
                tick_rate = 1.0;
            }
            double samp_rate = my_streamer->get_terminator()->get_input_samp_rate();
            if (samp_rate == rfnoc::rate_node_ctrl::RATE_UNDEFINED) {
                samp_rate = 1.0;
            }
            double scaling = my_streamer->get_terminator()->get_input_scale_factor();
            if (scaling == rfnoc::scalar_node_ctrl::SCALE_UNDEFINED) {
                scaling = 32767.;
            }
            UHD_TX_STREAMER_LOG()
                << "New tick_rate == " << tick_rate << "  New samp_rate == " << samp_rate
                << " New scaling == " << scaling;
            my_streamer->set_tick_rate(tick_rate);
            my_streamer->set_samp_rate(samp_rate);
            my_streamer->set_scale_factor(scaling);
        }
    }
}

tx_streamer::sptr device3_impl::get_tx_stream(const uhd::stream_args_t& args_)
{
    boost::mutex::scoped_lock lock(_transport_setup_mutex);
    stream_args_t args = sanitize_stream_args(args_);

    // I. Generate the channel list
    std::vector<uhd::rfnoc::block_id_t> chan_list;
    std::vector<device_addr_t> chan_args;
    generate_channel_list(args, chan_list, chan_args);
    // Note: All 'args.args' are merged into chan_args now.

    // shared async queue for all channels in streamer
    boost::shared_ptr<async_md_type> async_md(new async_md_type(1000 /*messages deep*/));

    // II. Iterate over all channels
    boost::shared_ptr<device3_send_packet_streamer> my_streamer;
    // The terminator's lifetime is coupled to the streamer.
    // There is only one terminator. If the streamer has multiple channels,
    // it will be connected to each downstream block.
    rfnoc::tx_stream_terminator::sptr send_terminator =
        rfnoc::tx_stream_terminator::make();
    for (size_t stream_i = 0; stream_i < chan_list.size(); stream_i++) {
        // First, configure the downstream blocks and create the transports

        // Get block ID and mb index
        uhd::rfnoc::block_id_t block_id = chan_list[stream_i];
        // Update args so args.args is always valid for this particular channel:
        args.args       = chan_args[stream_i];
        size_t mb_index = block_id.get_device_no();
        size_t suggested_block_port =
            args.args.cast<size_t>("block_port", rfnoc::ANY_PORT);

        // Access to this channel's block control
        uhd::rfnoc::sink_block_ctrl_base::sptr blk_ctrl =
            boost::dynamic_pointer_cast<uhd::rfnoc::sink_block_ctrl_base>(
                get_block_ctrl(block_id));

        // Connect the terminator with this channel's block.
        // This will throw if the connection is not possible.
        size_t block_port =
            blk_ctrl->connect_upstream(send_terminator, suggested_block_port, args.args);
        const size_t terminator_port = send_terminator->connect_downstream(blk_ctrl);
        blk_ctrl->set_upstream_port(block_port, terminator_port);
        send_terminator->set_downstream_port(terminator_port, block_port);

        // Check if the block connection is compatible (spp and item type)
        check_stream_sig_compatible(
            blk_ctrl->get_input_signature(block_port), args, "TX");

        // Setup the dsp transport hints
        device_addr_t tx_hints = get_tx_hints(mb_index);

        // Search the device and all nodes for lowest MTU
        size_t mtu = std::min(
            get_mtu(mb_index, uhd::direction_t::TX_DIRECTION),
            blk_ctrl->get_mtu(block_port));
        UHD_TX_STREAMER_LOG() << "Maximum MTU supported by "
                              <<  blk_ctrl->unique_id() << ": "
                              << blk_ctrl->get_mtu(block_port);
        std::vector<boost::shared_ptr<uhd::rfnoc::sink_block_ctrl_base>>
            downstream_sink_nodes =
                blk_ctrl->find_downstream_node<uhd::rfnoc::sink_block_ctrl_base>();
        for (const boost::shared_ptr<uhd::rfnoc::sink_block_ctrl_base>& node :
            downstream_sink_nodes) {
            // Get MTU from Port 0 of the downstream nodes. This is okay for now as
            // currently we use port 0 of a block in case of channel 1.
            UHD_TX_STREAMER_LOG() << "Maximum MTU supported by "
                                  <<  node->unique_id() << ": "
                                  << node->get_mtu(0);
            mtu = std::min(mtu, node->get_mtu(0));
        }
        tx_hints["mtu"] = std::to_string(mtu);

        // Make sure user supplied send_frame_size is less than the MTU
        if (tx_hints.cast<size_t>("send_frame_size", mtu) > mtu) {
            UHD_LOGGER_WARNING("STREAMER")
                << "Requested send_frame_size of "
                << tx_hints["send_frame_size"]
                << " exceeds the maximum possible on this stream. Using "
                << mtu;
            tx_hints["send_frame_size"] = std::to_string(mtu);
        }

        const size_t fifo_size = blk_ctrl->get_fifo_size(block_port);
        // Allocate sid and create transport
        uhd::sid_t stream_address = blk_ctrl->get_address(block_port);
        UHD_TX_STREAMER_LOG() << "creating tx stream " << tx_hints.to_string();
        both_xports_t xport = make_transport(stream_address, TX_DATA, tx_hints);
        both_xports_t async_xport =
            make_transport(stream_address, ASYNC_MSG, device_addr_t(""));
        UHD_TX_STREAMER_LOG() << std::hex << "data_sid = " << xport.send_sid << std::dec;

        // Configure flow control
        // This disables the FC module's output, do this before configuring flow control
        blk_ctrl->sr_write(uhd::rfnoc::SR_CLEAR_RX_FC, 0x1, block_port);
        blk_ctrl->sr_write(uhd::rfnoc::SR_CLEAR_RX_FC, 0x0, block_port);
        // Configure flow control on downstream block
        const size_t pkt_size = xport.send->get_send_frame_size();
        const size_t fc_window =
            std::min(tx_hints.cast<size_t>("send_buff_size", fifo_size), fifo_size);
        const size_t fc_handle_window =
            std::max<size_t>(pkt_size, fc_window / stream_options.tx_fc_response_freq);
        UHD_TX_STREAMER_LOG() << "Flow Control Window = " << fc_window
                              << ", Flow Control Handler Window = " << fc_handle_window
                              << ", FIFO size = " << fifo_size;
        blk_ctrl->configure_flow_control_in(fc_handle_window, /*bytes*/
            block_port);
        // Add flow control transport
        boost::shared_ptr<tx_fc_cache_t> fc_cache(new tx_fc_cache_t(fc_window));
        if (xport.endianness == ENDIANNESS_BIG) {
            fc_cache->to_host   = uhd::ntohx<uint32_t>;
            fc_cache->from_host = uhd::htonx<uint32_t>;
            fc_cache->pack      = vrt::chdr::if_hdr_pack_be;
            fc_cache->unpack    = vrt::chdr::if_hdr_unpack_be;
        } else {
            fc_cache->to_host   = uhd::wtohx<uint32_t>;
            fc_cache->from_host = uhd::htowx<uint32_t>;
            fc_cache->pack      = vrt::chdr::if_hdr_pack_le;
            fc_cache->unpack    = vrt::chdr::if_hdr_unpack_le;
        }
        xport.send = zero_copy_flow_ctrl::make(xport.send,
            [fc_cache, xport](managed_buffer::sptr buff) {
                return tx_flow_ctrl(fc_cache, xport.recv, buff);
            },
            0);

        // Configure return path for async messages
        blk_ctrl->sr_write(
            uhd::rfnoc::SR_RESP_IN_DST_SID, async_xport.recv_sid.get_dst(), block_port);
        UHD_TX_STREAMER_LOG() << "resp_in_dst_sid == "
                              << boost::format("0x%04X") % xport.recv_sid.get_dst();

        // FIXME: Once there is a better way to map the radio block and port
        // to the channel or another way to receive asynchronous messages that
        // is not in-band, this should be removed.
        if (args.args.has_key("radio_id") and args.args.has_key("radio_port")) {
            // Find downstream radio node and set the response SID to the host
            uhd::rfnoc::block_id_t radio_id(args.args["radio_id"]);
            size_t radio_port = args.args.cast<size_t>("radio_port", 0);
            std::vector<boost::shared_ptr<uhd::rfnoc::radio_ctrl>>
                downstream_radio_nodes =
                    blk_ctrl->find_downstream_node<uhd::rfnoc::radio_ctrl>();
            UHD_TX_STREAMER_LOG()
                << "Number of downstream radio nodes: " << downstream_radio_nodes.size();
            for (const boost::shared_ptr<uhd::rfnoc::radio_ctrl>& node :
                downstream_radio_nodes) {
                if (node->get_block_id() == radio_id) {
                    node->sr_write(uhd::rfnoc::SR_RESP_IN_DST_SID,
                        async_xport.recv_sid.get_dst(),
                        radio_port);
                }
            }
        } else {
            // FIXME:  This block is preserved for legacy behavior where the
            // radio_id and radio_port are not provided.  It fails if more
            // than one radio is visible downstream or the port on the radio
            // is not the same as the block_port.  It should be removed as
            // soon as possible.
            // Find all downstream radio nodes and set their response SID to the host
            std::vector<boost::shared_ptr<uhd::rfnoc::radio_ctrl>>
                downstream_radio_nodes =
                    blk_ctrl->find_downstream_node<uhd::rfnoc::radio_ctrl>();
            UHD_TX_STREAMER_LOG()
                << "Number of downstream radio nodes: " << downstream_radio_nodes.size();
            for (const boost::shared_ptr<uhd::rfnoc::radio_ctrl>& node :
                downstream_radio_nodes) {
                node->sr_write(uhd::rfnoc::SR_RESP_IN_DST_SID,
                    async_xport.recv_sid.get_dst(),
                    block_port);
            }
        }

        // Second, configure the streamer now that the blocks and transports are
        // configured

        // make the new streamer given the samples per packet
        if (not my_streamer) {
            // To calculate the max number of samples per packet, we assume the maximum
            // header length to avoid fragmentation should the entire header be used.
            const size_t bpp =
                tx_hints.cast<size_t>("bpp", pkt_size) -
                stream_options.tx_max_len_hdr;
            const size_t bpi =
                convert::get_bytes_per_item(args.otw_format); // bytes per item
            const size_t spp = std::min(args.args.cast<size_t>("spp", bpp / bpi),
                bpp / bpi); // samples per packet
            UHD_TX_STREAMER_LOG()
                << "bpp == " << bpp
                << ", bpi == " << bpi
                << ", spp == " << spp;

            my_streamer = boost::make_shared<device3_send_packet_streamer>(
                spp, send_terminator, xport, async_xport);
            my_streamer->resize(chan_list.size());
        }

        // init some streamer stuff
        std::string conv_endianness;
        if (xport.endianness == ENDIANNESS_BIG) {
            my_streamer->set_vrt_packer(&vrt::chdr::if_hdr_pack_be);
            conv_endianness = "be";
        } else {
            my_streamer->set_vrt_packer(&vrt::chdr::if_hdr_pack_le);
            conv_endianness = "le";
        }

        // set the converter
        uhd::convert::id_type id;
        id.input_format  = args.cpu_format;
        id.num_inputs    = 1;
        id.output_format = args.otw_format + "_item32_" + conv_endianness;
        id.num_outputs   = 1;
        my_streamer->set_converter(id);

        boost::shared_ptr<async_tx_info_t> async_tx_info(new async_tx_info_t());
        async_tx_info->stream_channel  = args.channels[stream_i];
        async_tx_info->device_channel  = mb_index;
        async_tx_info->async_queue     = async_md;
        async_tx_info->old_async_queue = _async_md;

        task::sptr async_task =
            task::make([async_tx_info, async_xport, xport, send_terminator]() {
                handle_tx_async_msgs(async_tx_info,
                    async_xport.recv,
                    xport.endianness == ENDIANNESS_BIG ? uhd::ntohx<uint32_t>
                                                       : uhd::wtohx<uint32_t>,
                    xport.endianness == ENDIANNESS_BIG ? vrt::chdr::if_hdr_unpack_be
                                                       : vrt::chdr::if_hdr_unpack_le,
                    [send_terminator]() { return send_terminator->get_tick_rate(); });
            });
        my_streamer->add_async_msg_task(async_task);

        // Give the streamer a functor to get the send buffer
        my_streamer->set_xport_chan_get_buff(stream_i,
            [xport](const double timeout) { return xport.send->get_send_buff(timeout); });
        // Give the streamer a functor handled received async messages
        my_streamer->set_async_receiver(
            [async_md](uhd::async_metadata_t& md, const double timeout) {
                return async_md->pop_with_timed_wait(md, timeout);
            });
        my_streamer->set_xport_chan_sid(stream_i, true, xport.send_sid);
        // CHDR does not support trailers
        my_streamer->set_enable_trailer(false);

        // Avoid sending FC ACKs if the transport is lossless or the user
        // has explictly requested not to send them
        if (not(xport.lossless or tx_hints.has_key("send_no_fc_acks"))) {
            my_streamer->set_xport_chan_post_send_cb(stream_i, [fc_cache, xport]() {
                tx_flow_ctrl_ack(fc_cache, xport.send, xport.send_sid);
            });
        }
    }

    // Notify all blocks in this chain that they are connected to an active streamer
    send_terminator->set_tx_streamer(true, 0);

    // Store a weak pointer to prevent a streamer->device3_impl->streamer circular
    // dependency. Note that we store the streamer only once, and use its terminator's ID
    // to do so.
    _tx_streamers[send_terminator->unique_id()] =
        boost::weak_ptr<uhd::tx_streamer>(my_streamer);

    // Sets tick rate, samp rate and scaling on this streamer
    // A registered terminator is required to do this.
    update_tx_streamers();

    post_streamer_hooks(TX_DIRECTION);
    return my_streamer;
}
