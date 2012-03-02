//
// Copyright 2012 Ettus Research LLC
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

#include "usrp2_regs.hpp"
#include <uhd/exception.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/transport/vrt_if_packet.hpp>
#include "usrp2_fifo_ctrl.hpp"
#include <boost/thread/mutex.hpp>
#include <boost/asio.hpp> //htonl

using namespace uhd::transport;

static const size_t POKE32_CMD = (1 << 8);
static const size_t PEEK32_CMD = 0;
static const double ACK_TIMEOUT = 0.5;
static const double MASSIVE_TIMEOUT = 10.0; //for when we wait on a timed command
static const boost::uint32_t MAX_SEQS_OUT = 64;

class usrp2_fifo_ctrl_impl : public usrp2_fifo_ctrl{
public:

    usrp2_fifo_ctrl_impl(zero_copy_if::sptr xport):
        _xport(xport),
        _seq(0), _last_seq_ack(0)
    {
        while (_xport->get_recv_buff(0.0)){} //flush
        this->set_time(uhd::time_spec_t(0.0));
        this->set_tick_rate(1.0); //something possible but bogus
    }

    UHD_INLINE void send_pkt(wb_addr_type addr, boost::uint32_t data, int cmd){
        managed_send_buffer::sptr buff = _xport->get_send_buff(0.0);
        if (not buff){
            throw uhd::runtime_error("fifo ctrl timed out getting a send buffer");
        }
        boost::uint32_t *trans = buff->cast<boost::uint32_t *>();
        trans[0] = htonl(++_seq);
        boost::uint32_t *pkt = trans + 1;

        //load packet info
        vrt::if_packet_info_t packet_info;
        packet_info.packet_type = vrt::if_packet_info_t::PACKET_TYPE_CONTEXT;
        packet_info.num_payload_words32 = 2;
        packet_info.num_payload_bytes = packet_info.num_payload_words32*sizeof(boost::uint32_t);
        packet_info.packet_count = _seq;
        packet_info.tsf = _time.to_ticks(_tick_rate);
        packet_info.sob = false;
        packet_info.eob = false;
        packet_info.has_sid = false;
        packet_info.has_cid = false;
        packet_info.has_tsi = false;
        packet_info.has_tsf = _use_time;
        packet_info.has_tlr = false;

        //load header
        vrt::if_hdr_pack_be(pkt, packet_info);

        //load payload
        const boost::uint32_t ctrl_word = (addr & 0xff) | cmd | (_seq << 16);
        pkt[packet_info.num_header_words32+0] = htonl(ctrl_word);
        pkt[packet_info.num_header_words32+1] = htonl(data);

        //send the buffer over the interface
        buff->commit(sizeof(boost::uint32_t)*(packet_info.num_packet_words32+1));
    }

    void poke32(wb_addr_type addr, boost::uint32_t data){
        boost::mutex::scoped_lock lock(_mutex);

        this->send_pkt((addr - SETTING_REGS_BASE)/4, data, POKE32_CMD);

        this->wait_for_ack(boost::int16_t(_seq-MAX_SEQS_OUT));
    }

    boost::uint32_t peek32(wb_addr_type addr){
        boost::mutex::scoped_lock lock(_mutex);

        this->send_pkt((addr - READBACK_BASE)/4, 0, PEEK32_CMD);

        return this->wait_for_ack(boost::int16_t(_seq));
    }

    boost::uint32_t wait_for_ack(const boost::int16_t seq_to_ack){

        while (_last_seq_ack < seq_to_ack){
            managed_recv_buffer::sptr buff = _xport->get_recv_buff(_use_time? MASSIVE_TIMEOUT : ACK_TIMEOUT);
            if (not buff){
                throw uhd::runtime_error("fifo ctrl timed out looking for acks");
            }
            const boost::uint32_t *pkt = buff->cast<const boost::uint32_t *>();
            vrt::if_packet_info_t packet_info;
            packet_info.num_packet_words32 = buff->size()/sizeof(boost::uint32_t);
            vrt::if_hdr_unpack_be(pkt, packet_info);
            _last_seq_ack = ntohl(pkt[packet_info.num_header_words32+0]) >> 16;
            if (_last_seq_ack == seq_to_ack){
                return ntohl(pkt[packet_info.num_header_words32+1]);
            }
        }

        return 0;
    }

    void poke16(wb_addr_type, boost::uint16_t){
        throw uhd::not_implemented_error("poke16 not implemented in fifo ctrl module");
    }

    boost::uint16_t peek16(wb_addr_type){
        throw uhd::not_implemented_error("peek16 not implemented in fifo ctrl module");
    }

    void set_time(const uhd::time_spec_t &time){
        boost::mutex::scoped_lock lock(_mutex);
        _time = time;
        _use_time = _time != uhd::time_spec_t(0.0);
    }

    void set_tick_rate(const double rate){
        boost::mutex::scoped_lock lock(_mutex);
        _tick_rate = rate;
    }

private:
    zero_copy_if::sptr _xport;
    boost::mutex _mutex;
    boost::uint32_t _seq;
    boost::uint16_t _last_seq_ack;
    uhd::time_spec_t _time;
    bool _use_time;
    double _tick_rate;
};


usrp2_fifo_ctrl::sptr usrp2_fifo_ctrl::make(zero_copy_if::sptr xport){
    return sptr(new usrp2_fifo_ctrl_impl(xport));
}
