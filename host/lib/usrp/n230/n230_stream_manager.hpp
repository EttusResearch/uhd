//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_N230_STREAM_MANAGER_HPP
#define INCLUDED_N230_STREAM_MANAGER_HPP

#include "n230_device_args.hpp"
#include "n230_resource_manager.hpp"

#include <uhdlib/usrp/cores/time_core_3000.hpp>
#include <uhdlib/usrp/cores/rx_vita_core_3000.hpp>
#include <uhd/types/sid.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/types/metadata.hpp>
#include <uhd/transport/zero_copy.hpp>
#include <uhd/transport/bounded_buffer.hpp>
#include <uhd/transport/vrt_if_packet.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/utils/tasks.hpp>
#include <boost/smart_ptr.hpp>

namespace uhd { namespace usrp { namespace n230 {

class n230_stream_manager : public boost::noncopyable
{
public:     //Methods
    n230_stream_manager(
        const n230_device_args_t& dev_args,
        boost::shared_ptr<n230_resource_manager> resource_mgr,
        boost::weak_ptr<property_tree> prop_tree);
    virtual ~n230_stream_manager();

    rx_streamer::sptr get_rx_stream(
        const uhd::stream_args_t &args);

    tx_streamer::sptr get_tx_stream(
        const uhd::stream_args_t &args_);

    bool recv_async_msg(
        async_metadata_t &async_metadata,
        double timeout);

    void update_stream_states();

    void update_rx_samp_rate(
        const size_t dspno, const double rate);

    void update_tx_samp_rate(
        const size_t dspno, const double rate);

    void update_tick_rate(
        const double rate);

private:
    typedef transport::bounded_buffer<async_metadata_t> async_md_queue_t;

    struct rx_fc_cache_t
    {
        rx_fc_cache_t():
            last_seq_in(0){}
        size_t last_seq_in;
    };

    struct tx_fc_cache_t
    {
        tx_fc_cache_t():
            stream_channel(0),
            device_channel(0),
            last_seq_out(0),
            last_seq_ack(0),
            seq_queue(1){}
        size_t stream_channel;
        size_t device_channel;
        size_t last_seq_out;
        size_t last_seq_ack;
        transport::bounded_buffer<size_t> seq_queue;
        boost::shared_ptr<async_md_queue_t> async_queue;
        boost::shared_ptr<async_md_queue_t> old_async_queue;
    };

    typedef boost::function<double(void)> tick_rate_retriever_t;

    void _handle_overflow(const size_t i);

    double _get_tick_rate();

    static size_t _get_rx_flow_control_window(
        size_t frame_size, size_t sw_buff_size);

    static void _handle_rx_flowctrl(
        const sid_t& sid,
        transport::zero_copy_if::sptr xport,
        boost::shared_ptr<rx_fc_cache_t> fc_cache,
        const size_t last_seq);

    static void _handle_tx_async_msgs(
        boost::shared_ptr<tx_fc_cache_t> guts,
        transport::zero_copy_if::sptr xport,
        tick_rate_retriever_t get_tick_rate);

    static transport::managed_send_buffer::sptr _get_tx_buff_with_flowctrl(
        task::sptr /*holds ref*/,
        boost::shared_ptr<tx_fc_cache_t> guts,
        transport::zero_copy_if::sptr xport,
        size_t fc_pkt_window,
        const double timeout);

    static size_t _get_tx_flow_control_window(
        size_t payload_size,
        size_t hw_buff_size);

    static void _cvita_hdr_unpack(
        const uint32_t *packet_buff,
        transport::vrt::if_packet_info_t &if_packet_info);

    static void _cvita_hdr_pack(
        uint32_t *packet_buff,
        transport::vrt::if_packet_info_t &if_packet_info);

    const n230_device_args_t                  _dev_args;
    boost::shared_ptr<n230_resource_manager>  _resource_mgr;
    //TODO: Find a way to remove this dependency
    boost::weak_ptr<property_tree>          _tree;

    boost::mutex                            _stream_setup_mutex;
    uhd::msg_task::sptr                     _async_task;
    boost::shared_ptr<async_md_queue_t>     _async_md_queue;
    boost::weak_ptr<uhd::tx_streamer>       _tx_streamers[fpga::NUM_RADIOS];
    boost::weak_ptr<uhd::rx_streamer>       _rx_streamers[fpga::NUM_RADIOS];
    stream_args_t                           _tx_stream_cached_args[fpga::NUM_RADIOS];
    stream_args_t                           _rx_stream_cached_args[fpga::NUM_RADIOS];

    static const uint32_t HW_SEQ_NUM_MASK    = 0xFFF;
};

}}} //namespace

#endif //INCLUDED_N230_STREAM_MANAGER_HPP
