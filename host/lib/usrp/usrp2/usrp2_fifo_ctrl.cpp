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

class usrp2_fifo_ctrl_impl : public usrp2_fifo_ctrl{
public:

    usrp2_fifo_ctrl_impl(zero_copy_if::sptr xport):
        _xport(xport),
        _seq(0)
    {
        //NOP
    }

    UHD_INLINE void send_pkt(wb_addr_type addr, boost::uint32_t data, int cmd){
        managed_send_buffer::sptr buff = _xport->get_send_buff(0.0);
        if (not buff){
            throw uhd::runtime_error("peek32/poke32 in fifo ctrl timed out getting a send buffer");
        }
        boost::uint32_t *trans = buff->cast<boost::uint32_t *>();
        trans[0] = htonl(_seq++);
        boost::uint32_t *pkt = trans + 1;

        //load packet info
        vrt::if_packet_info_t packet_info;
        packet_info.packet_type = vrt::if_packet_info_t::PACKET_TYPE_CONTEXT;
        packet_info.num_payload_words32 = 2;
        packet_info.num_payload_bytes = packet_info.num_payload_words32*sizeof(boost::uint32_t);
        packet_info.packet_count = _seq;
        packet_info.sob = false;
        packet_info.eob = false;
        packet_info.has_sid = false;
        packet_info.has_cid = false;
        packet_info.has_tsi = false;
        packet_info.has_tsf = false;
        packet_info.has_tlr = false;

        //load header with offset 1
        vrt::if_hdr_pack_be(pkt, packet_info);

        //load payload with offset 1
        const boost::uint32_t ctrl_word = (addr & 0xff) | cmd | (_seq << 16);
        pkt[packet_info.num_header_words32+0] = htonl(ctrl_word);
        pkt[packet_info.num_header_words32+1] = htonl(data);

        //send the buffer over the interface
        buff->commit(sizeof(boost::uint32_t)*(packet_info.num_packet_words32+1));
    }

    void poke32(wb_addr_type addr, boost::uint32_t data){
        boost::mutex::scoped_lock lock(_mutex);

        while (_xport->get_recv_buff(0.0)){} //flush

        this->send_pkt(addr, data, POKE32_CMD);

        {
            managed_recv_buffer::sptr buff = _xport->get_recv_buff(ACK_TIMEOUT);
            if (not buff){
                throw uhd::runtime_error("poke32 in fifo ctrl timed out getting a recv buffer");
            }
            const boost::uint32_t *pkt = buff->cast<const boost::uint32_t *>();
            vrt::if_packet_info_t packet_info;
            packet_info.num_packet_words32 = buff->size()/sizeof(boost::uint32_t);
            vrt::if_hdr_unpack_be(pkt, packet_info);
        }
    }

    boost::uint32_t peek32(wb_addr_type addr){
        boost::mutex::scoped_lock lock(_mutex);

        while (_xport->get_recv_buff(0.0)){} //flush

        this->send_pkt(addr >> 2, 0, PEEK32_CMD);

        {
            managed_recv_buffer::sptr buff = _xport->get_recv_buff(ACK_TIMEOUT);
            if (not buff){
                throw uhd::runtime_error("peek32 in fifo ctrl timed out getting a recv buffer");
            }
            const boost::uint32_t *pkt = buff->cast<const boost::uint32_t *>();
            vrt::if_packet_info_t packet_info;
            packet_info.num_packet_words32 = buff->size()/sizeof(boost::uint32_t);
            vrt::if_hdr_unpack_be(pkt, packet_info);
            return ntohl(pkt[packet_info.num_header_words32+1]);
        }
    }

    void poke16(wb_addr_type, boost::uint16_t){
        throw uhd::not_implemented_error("poke16 not implemented in fifo ctrl module");
    }

    boost::uint16_t peek16(wb_addr_type){
        throw uhd::not_implemented_error("peek16 not implemented in fifo ctrl module");
    }

private:
    zero_copy_if::sptr _xport;
    boost::mutex _mutex;
    boost::uint32_t _seq;
};


usrp2_fifo_ctrl::sptr usrp2_fifo_ctrl::make(zero_copy_if::sptr xport){
    return sptr(new usrp2_fifo_ctrl_impl(xport));
}
