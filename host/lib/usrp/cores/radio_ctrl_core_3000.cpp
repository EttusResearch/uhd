//
// Copyright 2012-2013 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "radio_ctrl_core_3000.hpp"
#include "async_packet_handler.hpp"
#include <uhd/exception.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/safe_call.hpp>
#include <uhd/transport/bounded_buffer.hpp>
#include <uhd/transport/vrt_if_packet.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <queue>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;

static const double ACK_TIMEOUT = 2.0; //supposed to be worst case practical timeout
static const double MASSIVE_TIMEOUT = 10.0; //for when we wait on a timed command
static const size_t SR_READBACK  = 32;

class radio_ctrl_core_3000_impl : public radio_ctrl_core_3000
{
public:

    radio_ctrl_core_3000_impl(
        const bool big_endian,
        uhd::transport::zero_copy_if::sptr ctrl_xport,
        uhd::transport::zero_copy_if::sptr resp_xport,
        const boost::uint32_t sid,
        const std::string &name
    ):
        _link_type(vrt::if_packet_info_t::LINK_TYPE_CHDR),
        _packet_type(vrt::if_packet_info_t::PACKET_TYPE_CONTEXT),
        _bige(big_endian),
        _ctrl_xport(ctrl_xport),
        _resp_xport(resp_xport),
        _sid(sid),
        _name(name),
        _seq_out(0),
        _timeout(ACK_TIMEOUT),
        _resp_queue(128/*max response msgs*/),
        _resp_queue_size(_resp_xport? _resp_xport->get_num_recv_frames() : 3)
    {
        UHD_LOG << "radio_ctrl_core_3000_impl() " << _name << std::endl;
        if (resp_xport)
        {
            while (resp_xport->get_recv_buff(0.0)){} //flush
        }
        this->set_time(uhd::time_spec_t(0.0));
        this->set_tick_rate(1.0); //something possible but bogus
    }

    ~radio_ctrl_core_3000_impl(void)
    {
        UHD_LOG << "~radio_ctrl_core_3000_impl() " << _name << std::endl;
        _timeout = ACK_TIMEOUT; //reset timeout to something small
        UHD_SAFE_CALL(
            this->peek32(0); //dummy peek with the purpose of ack'ing all packets
            _async_task.reset(); //now its ok to release the task
        )
    }

    /*******************************************************************
     * Peek and poke 32 bit implementation
     ******************************************************************/
    void poke32(const wb_addr_type addr, const boost::uint32_t data)
    {
        boost::mutex::scoped_lock lock(_mutex);
        UHD_LOGV(always) << _name << std::hex << " addr 0x" << addr << " data 0x" << data << std::dec << std::endl;

        this->send_pkt(addr/4, data);
        this->wait_for_ack(false);
    }

    boost::uint32_t peek32(const wb_addr_type addr)
    {
        boost::mutex::scoped_lock lock(_mutex);
        UHD_LOGV(always) << _name << std::hex << " addr 0x" << addr << std::dec << std::endl;

        this->send_pkt(SR_READBACK, addr/8);
        this->wait_for_ack(false);

        this->send_pkt(0);
        const boost::uint64_t res = this->wait_for_ack(true);
        const boost::uint32_t lo = boost::uint32_t(res & 0xffffffff);
        const boost::uint32_t hi = boost::uint32_t(res >> 32);
        return ((addr/4) & 0x1)? hi : lo;
    }

    boost::uint64_t peek64(const wb_addr_type addr)
    {
        boost::mutex::scoped_lock lock(_mutex);
        UHD_LOGV(always) << _name << std::hex << " addr 0x" << addr << std::dec << std::endl;

        this->send_pkt(SR_READBACK, addr/8);
        this->wait_for_ack(false);

        this->send_pkt(0);
        return this->wait_for_ack(true);
    }

    /*******************************************************************
     * Update methods for time
     ******************************************************************/
    void set_time(const uhd::time_spec_t &time)
    {
        boost::mutex::scoped_lock lock(_mutex);
        _time = time;
        _use_time = _time != uhd::time_spec_t(0.0);
        if (_use_time) _timeout = MASSIVE_TIMEOUT; //permanently sets larger timeout
    }

    void set_tick_rate(const double rate)
    {
        boost::mutex::scoped_lock lock(_mutex);
        _tick_rate = rate;
    }

private:

    /*******************************************************************
     * Primary control and interaction private methods
     ******************************************************************/
    UHD_INLINE void send_pkt(const boost::uint32_t addr, const boost::uint32_t data = 0)
    {
        managed_send_buffer::sptr buff = _ctrl_xport->get_send_buff(0.0);
        if (not buff){
            throw uhd::runtime_error("fifo ctrl timed out getting a send buffer");
        }
        boost::uint32_t *pkt = buff->cast<boost::uint32_t *>();

        //load packet info
        vrt::if_packet_info_t packet_info;
        packet_info.link_type = _link_type;
        packet_info.packet_type = _packet_type;
        packet_info.num_payload_words32 = 2;
        packet_info.num_payload_bytes = packet_info.num_payload_words32*sizeof(boost::uint32_t);
        packet_info.packet_count = _seq_out;
        packet_info.tsf = _time.to_ticks(_tick_rate);
        packet_info.sob = false;
        packet_info.eob = false;
        packet_info.sid = _sid;
        packet_info.has_sid = true;
        packet_info.has_cid = false;
        packet_info.has_tsi = false;
        packet_info.has_tsf = _use_time;
        packet_info.has_tlr = false;

        //load header
        if (_bige) vrt::if_hdr_pack_be(pkt, packet_info);
        else vrt::if_hdr_pack_le(pkt, packet_info);

        //load payload
        pkt[packet_info.num_header_words32+0] = (_bige)? uhd::htonx(addr) : uhd::htowx(addr);
        pkt[packet_info.num_header_words32+1] = (_bige)? uhd::htonx(data) : uhd::htowx(data);
        //UHD_MSG(status) << boost::format("0x%08x, 0x%08x\n") % addr % data;

        //send the buffer over the interface
        _outstanding_seqs.push(_seq_out);
        buff->commit(sizeof(boost::uint32_t)*(packet_info.num_packet_words32));

        _seq_out++; //inc seq for next call
    }

    UHD_INLINE boost::uint64_t wait_for_ack(const bool readback)
    {
        while (readback or (_outstanding_seqs.size() >= _resp_queue_size))
        {
            UHD_LOGV(always) << _name << " wait_for_ack: " << "readback = " << readback << " outstanding_seqs.size() " << _outstanding_seqs.size() << std::endl;

            //get seq to ack from outstanding packets list
            UHD_ASSERT_THROW(not _outstanding_seqs.empty());
            const size_t seq_to_ack = _outstanding_seqs.front();
            _outstanding_seqs.pop();

            //parse the packet
            vrt::if_packet_info_t packet_info;
            resp_buff_type resp_buff;
            boost::uint32_t const *pkt = NULL;
            managed_recv_buffer::sptr buff;

            //get buffer from response endpoint - or die in timeout
            if (_resp_xport)
            {
                buff = _resp_xport->get_recv_buff(_timeout);
                try
                {
                    UHD_ASSERT_THROW(bool(buff));
                    UHD_ASSERT_THROW(bool(buff->size()));
                }
                catch(const std::exception &ex)
                {
                    throw uhd::io_error(str(boost::format("Radio ctrl (%s) no response packet - %s") % _name % ex.what()));
                }
                pkt = buff->cast<const boost::uint32_t *>();
                packet_info.num_packet_words32 = buff->size()/sizeof(boost::uint32_t);
            }

            //get buffer from response endpoint - or die in timeout
            else
            {
                UHD_ASSERT_THROW(_resp_queue.pop_with_timed_wait(resp_buff, _timeout));
                pkt = resp_buff.data;
                packet_info.num_packet_words32 = sizeof(resp_buff)/sizeof(boost::uint32_t);
            }

            //parse the buffer
            try
            {
                packet_info.link_type = _link_type;
                if (_bige) vrt::if_hdr_unpack_be(pkt, packet_info);
                else vrt::if_hdr_unpack_le(pkt, packet_info);
            }
            catch(const std::exception &ex)
            {
                UHD_MSG(error) << "Radio ctrl bad VITA packet: " << ex.what() << std::endl;
                UHD_VAR(buff->size());
                UHD_MSG(status) << std::hex << pkt[0] << std::dec << std::endl;
                UHD_MSG(status) << std::hex << pkt[1] << std::dec << std::endl;
                UHD_MSG(status) << std::hex << pkt[2] << std::dec << std::endl;
                UHD_MSG(status) << std::hex << pkt[3] << std::dec << std::endl;
            }

            //check the buffer
            try
            {
                UHD_ASSERT_THROW(packet_info.has_sid);
                UHD_ASSERT_THROW(packet_info.sid == boost::uint32_t((_sid >> 16) | (_sid << 16)));
                UHD_ASSERT_THROW(packet_info.packet_count == (seq_to_ack & 0xfff));
                UHD_ASSERT_THROW(packet_info.num_payload_words32 == 2);
                UHD_ASSERT_THROW(packet_info.packet_type == _packet_type);
            }
            catch(const std::exception &ex)
            {
                throw uhd::io_error(str(boost::format("Radio ctrl (%s) packet parse error - %s") % _name % ex.what()));
            }

            //return the readback value
            if (readback and _outstanding_seqs.empty())
            {
                const boost::uint64_t hi = (_bige)? uhd::ntohx(pkt[packet_info.num_header_words32+0]) : uhd::wtohx(pkt[packet_info.num_header_words32+0]);
                const boost::uint64_t lo = (_bige)? uhd::ntohx(pkt[packet_info.num_header_words32+1]) : uhd::wtohx(pkt[packet_info.num_header_words32+1]);
                return ((hi << 32) | lo);
            }
        }
        return 0;
    }

    void push_response(const boost::uint32_t *buff)
    {
        resp_buff_type resp_buff;
        std::memcpy(resp_buff.data, buff, sizeof(resp_buff));
        _resp_queue.push_with_haste(resp_buff);
    }

    void hold_task(boost::shared_ptr<void> task)
    {
        _async_task = task;
    }

    const vrt::if_packet_info_t::link_type_t _link_type;
    const vrt::if_packet_info_t::packet_type_t _packet_type;
    const bool _bige;
    const uhd::transport::zero_copy_if::sptr _ctrl_xport;
    const uhd::transport::zero_copy_if::sptr _resp_xport;
    boost::shared_ptr<void> _async_task;
    const boost::uint32_t _sid;
    const std::string _name;
    boost::mutex _mutex;
    size_t _seq_out;
    uhd::time_spec_t _time;
    bool _use_time;
    double _tick_rate;
    double _timeout;
    std::queue<size_t> _outstanding_seqs;
    struct resp_buff_type
    {
        boost::uint32_t data[8];
    };
    bounded_buffer<resp_buff_type> _resp_queue;
    const size_t _resp_queue_size;
};


radio_ctrl_core_3000::sptr radio_ctrl_core_3000::make(
    const bool big_endian,
    zero_copy_if::sptr ctrl_xport,
    zero_copy_if::sptr resp_xport,
    const boost::uint32_t sid,
    const std::string &name
)
{
    return sptr(new radio_ctrl_core_3000_impl(big_endian, ctrl_xport, resp_xport, sid, name));
}
