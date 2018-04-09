//
// Copyright 2011,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhdlib/usrp/common/recv_packet_demuxer.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/transport/vrt_if_packet.hpp>
#include <uhd/types/metadata.hpp>

#include <boost/thread/mutex.hpp>
#include <queue>
#include <deque>
#include <vector>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;

struct recv_pkt_demux_mrb : public managed_recv_buffer
{
public:
    recv_pkt_demux_mrb(void){/*NOP*/}

    void release(void)
    {
        delete this;
    }

    uint32_t buff[10];
};

static UHD_INLINE uint32_t extract_sid(managed_recv_buffer::sptr &buff){
    //ASSUME that the data is in little endian format
    return uhd::wtohx(buff->cast<const uint32_t *>()[1]);
}

recv_packet_demuxer::~recv_packet_demuxer(void){
    /* NOP */
}

class recv_packet_demuxer_impl : public uhd::usrp::recv_packet_demuxer{
public:
    recv_packet_demuxer_impl(
        transport::zero_copy_if::sptr transport,
        const size_t size,
        const uint32_t sid_base
    ):
        _transport(transport), _sid_base(sid_base), _queues(size)
    {
        /* NOP */
    }

    managed_recv_buffer::sptr get_recv_buff(const size_t index, const double timeout){
        boost::mutex::scoped_lock lock(_mutex);
        managed_recv_buffer::sptr buff;

        //there is already an entry in the queue, so pop that
        if (not _queues[index].wrapper.empty()){
            std::swap(buff, _queues[index].wrapper.front());
            _queues[index].wrapper.pop();
            return buff;
        }

        while (true){
            //otherwise call into the transport
            buff = _transport->get_recv_buff(timeout);
            if (buff.get() == NULL) return buff; //timeout

            //check the stream id to know which channel
            const size_t rx_index = extract_sid(buff) - _sid_base;
            if (rx_index == index) return buff; //got expected message

            //otherwise queue and try again
            if (rx_index < _queues.size()) _queues[rx_index].wrapper.push(buff);
            else
            {
                UHD_LOGGER_ERROR("STREAMER") << "Got a data packet with unknown SID " << extract_sid(buff) ;
                recv_pkt_demux_mrb *mrb = new recv_pkt_demux_mrb();
                vrt::if_packet_info_t info;
                info.packet_type = vrt::if_packet_info_t::PACKET_TYPE_DATA;
                info.num_payload_words32 = 1;
                info.num_payload_bytes = info.num_payload_words32*sizeof(uint32_t);
                info.has_sid = true;
                info.sid = _sid_base + index;
                vrt::if_hdr_pack_le(mrb->buff, info);
                mrb->buff[info.num_header_words32] = rx_metadata_t::ERROR_CODE_OVERFLOW;
                return mrb->make(mrb, mrb->buff, info.num_packet_words32*sizeof(uint32_t));
            }
        }
    }

private:
    transport::zero_copy_if::sptr _transport;
    const uint32_t _sid_base;
    boost::mutex _mutex;
    struct channel_guts_type{
        channel_guts_type(void): wrapper(container){}
        std::deque<managed_recv_buffer::sptr> container;
        std::queue<managed_recv_buffer::sptr> wrapper;
    };
    std::vector<channel_guts_type> _queues;
};

recv_packet_demuxer::sptr recv_packet_demuxer::make(transport::zero_copy_if::sptr transport, const size_t size, const uint32_t sid_base){
    return sptr(new recv_packet_demuxer_impl(transport, size, sid_base));
}
