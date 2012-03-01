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

    void poke32(wb_addr_type addr, boost::uint32_t data){
        boost::mutex::scoped_lock lock(_mutex);

        while (_xport->get_recv_buff(0.0)){} //flush

        {
            managed_send_buffer::sptr buff = _xport->get_send_buff(0.0);
            if (not buff){
                throw uhd::runtime_error("poke32 in fifo ctrl timed out getting a send buffer");
            }
            boost::uint32_t *pkt = buff->cast<boost::uint32_t *>();
            const boost::uint32_t ctrl_word = (addr >> 2) | POKE32_CMD | (++_seq << 16);
            //TODO vita packer goes here, below is the payload
            pkt[0] = htonl(ctrl_word);
            //FIXME cant be zero, need real VRT header here, see pkt dispatcher in fpga code
            if ((data & 0xffff) == 0) data = 1;
            pkt[1] = htonl(data);
            buff->commit(sizeof(boost::uint32_t)*2);
        }

        {
            managed_recv_buffer::sptr buff = _xport->get_recv_buff(ACK_TIMEOUT);
            if (not buff){
                throw uhd::runtime_error("poke32 in fifo ctrl timed out getting a recv buffer");
            }
            const boost::uint32_t *pkt = buff->cast<const boost::uint32_t *>();
            if (buff->size() < (sizeof(boost::uint32_t)*6) or (ntohl(pkt[4]) >> 16) != (_seq & 0xffff)){
                throw uhd::runtime_error("poke32 in fifo ctrl got invalid ack packet");
            }
        }
    }

    boost::uint32_t peek32(wb_addr_type addr){
        boost::mutex::scoped_lock lock(_mutex);

        while (_xport->get_recv_buff(0.0)){} //flush

        {
            managed_send_buffer::sptr buff = _xport->get_send_buff(0.0);
            if (not buff){
                throw uhd::runtime_error("peek32 in fifo ctrl timed out getting a send buffer");
            }
            boost::uint32_t *pkt = buff->cast<boost::uint32_t *>();
            const boost::uint32_t ctrl_word = (addr >> 2) | PEEK32_CMD | (++_seq << 16);
            //TODO vita packer goes here, below is the payload
            pkt[0] = htonl(ctrl_word);
            pkt[1] = htonl(0xffffffff); //FIXME cant be zero, need real VRT header here, see pkt dispatcher in fpga code
            buff->commit(sizeof(boost::uint32_t)*2);
        }

        {
            managed_recv_buffer::sptr buff = _xport->get_recv_buff(ACK_TIMEOUT);
            if (not buff){
                throw uhd::runtime_error("peek32 in fifo ctrl timed out getting a recv buffer");
            }
            const boost::uint32_t *pkt = buff->cast<const boost::uint32_t *>();
            if (buff->size() < (sizeof(boost::uint32_t)*6) or (ntohl(pkt[4]) >> 16) != (_seq & 0xffff)){
                throw uhd::runtime_error("peek32 in fifo ctrl got invalid ack packet");
            }
            return ntohl(pkt[5]);
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
