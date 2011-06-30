//
// Copyright 2011 Ettus Research LLC
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

#include "b100_ctrl.hpp"
#include <uhd/transport/usb_zero_copy.hpp>
#include <uhd/transport/zero_copy.hpp>
#include <uhd/transport/vrt_if_packet.hpp>
#include <uhd/utils/thread_priority.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/types/metadata.hpp>
#include <uhd/types/serial.hpp>
#include "ctrl_packet.hpp"
#include <boost/thread.hpp>
#include <uhd/exception.hpp>

using namespace uhd::transport;
using namespace uhd;

bool b100_ctrl_debug = false;

class b100_ctrl_impl : public b100_ctrl {
public:
    b100_ctrl_impl(uhd::transport::zero_copy_if::sptr ctrl_transport, const async_cb_type &async_cb):
        sync_ctrl_fifo(2),
        _async_cb(async_cb),
        _ctrl_transport(ctrl_transport),
        _seq(0)
    {
        boost::barrier spawn_barrier(2);
        viking_marauders.create_thread(boost::bind(&b100_ctrl_impl::viking_marauder_loop, this, boost::ref(spawn_barrier)));
        spawn_barrier.wait();
    }

    int write(boost::uint32_t addr, const ctrl_data_t &data);
    ctrl_data_t read(boost::uint32_t addr, size_t len);

    ~b100_ctrl_impl(void) {
        viking_marauders.interrupt_all();
        viking_marauders.join_all();
    }

    bool get_ctrl_data(ctrl_data_t &pkt_data, double timeout);

    void poke32(wb_addr_type addr, boost::uint32_t data){
        boost::mutex::scoped_lock lock(_ctrl_mutex);

        ctrl_data_t words(2);
        words[0] = data & 0x0000FFFF;
        words[1] = data >> 16;
        this->write(addr, words);
    }

    boost::uint32_t peek32(wb_addr_type addr){
        boost::mutex::scoped_lock lock(_ctrl_mutex);

        ctrl_data_t words = this->read(addr, 2);
        return boost::uint32_t((boost::uint32_t(words[1]) << 16) | words[0]);
    }

    void poke16(wb_addr_type addr, boost::uint16_t data){
        boost::mutex::scoped_lock lock(_ctrl_mutex);

        ctrl_data_t words(1);
        words[0] = data;
        this->write(addr, words);
    }

    boost::uint16_t peek16(wb_addr_type addr){
        boost::mutex::scoped_lock lock(_ctrl_mutex);

        ctrl_data_t words = this->read(addr, 1);
        return boost::uint16_t(words[0]);
    }

private:
    int send_pkt(boost::uint16_t *cmd);

    //änd hërë wë gö ä-Vïkïng för äsynchronous control packets
    void viking_marauder_loop(boost::barrier &);
    bounded_buffer<ctrl_data_t> sync_ctrl_fifo;
    async_cb_type _async_cb;
    boost::thread_group viking_marauders;

    uhd::transport::zero_copy_if::sptr _ctrl_transport;
    boost::uint8_t _seq;
    boost::mutex _ctrl_mutex;
};

/***********************************************************************
 * helper functions for packing/unpacking control packets
 **********************************************************************/
void pack_ctrl_pkt(boost::uint16_t *pkt_buff,
                          const ctrl_pkt_t &pkt){
    //first two bits are OP
    //next six bits are CALLBACKS
    //next 8 bits are SEQUENCE
    //next 16 bits are LENGTH (16-bit word)
    //next 32 bits are ADDRESS (16-bit word LSW)
    //then DATA (28 16-bit words)
    pkt_buff[0] = (boost::uint16_t(pkt.pkt_meta.op) << 14) | (boost::uint16_t(pkt.pkt_meta.callbacks) << 8) | pkt.pkt_meta.seq;
    pkt_buff[1] = pkt.pkt_meta.len;
    pkt_buff[2] = (pkt.pkt_meta.addr & 0x00000FFF);
    pkt_buff[3] = 0x0000; //address high bits always 0 on this device

    for(size_t i = 0; i < pkt.data.size(); i++) {
        pkt_buff[4+i] = pkt.data[i];
    }
}

void unpack_ctrl_pkt(const boost::uint16_t *pkt_buff,
                            ctrl_pkt_t &pkt){
    pkt.pkt_meta.seq = pkt_buff[0] & 0xFF;
    pkt.pkt_meta.op = CTRL_PKT_OP_READ; //really this is useless
    pkt.pkt_meta.len = pkt_buff[1];
    pkt.pkt_meta.callbacks = 0; //callbacks aren't implemented yet
    pkt.pkt_meta.addr = pkt_buff[2] | boost::uint32_t(pkt_buff[3] << 16);

    //let's check this so we don't go pushing 64K of crap onto the pkt
    if(pkt.pkt_meta.len > CTRL_PACKET_DATA_LENGTH) {
        throw uhd::runtime_error("Received control packet too long");
    }

    for(int i = 4; i < 4+pkt.pkt_meta.len; i++) pkt.data.push_back(pkt_buff[i]);
}

int b100_ctrl_impl::send_pkt(boost::uint16_t *cmd) {
    managed_send_buffer::sptr sbuf = _ctrl_transport->get_send_buff();
    if(!sbuf.get()) {
        throw uhd::runtime_error("Control channel send error");
    }

    //FIXME there's a better way to do this
    for(size_t i = 0; i < (CTRL_PACKET_LENGTH / sizeof(boost::uint16_t)); i++) {
        sbuf->cast<boost::uint16_t *>()[i] = cmd[i];
    }
    sbuf->commit(CTRL_PACKET_LENGTH); //fixed size transaction
    return 0;
}

int b100_ctrl_impl::write(boost::uint32_t addr, const ctrl_data_t &data) {
    UHD_ASSERT_THROW(data.size() <= (CTRL_PACKET_DATA_LENGTH / sizeof(boost::uint16_t)));
    ctrl_pkt_t pkt;
    pkt.data = data;
    pkt.pkt_meta.op = CTRL_PKT_OP_WRITE;
    pkt.pkt_meta.callbacks = 0;
    pkt.pkt_meta.seq = _seq++;
    pkt.pkt_meta.len = pkt.data.size();
    pkt.pkt_meta.addr = addr;
    boost::uint16_t pkt_buff[CTRL_PACKET_LENGTH / sizeof(boost::uint16_t)];

    pack_ctrl_pkt(pkt_buff, pkt);
    size_t result = send_pkt(pkt_buff);
    return result;
}

ctrl_data_t b100_ctrl_impl::read(boost::uint32_t addr, size_t len) {
    UHD_ASSERT_THROW(len <= (CTRL_PACKET_DATA_LENGTH / sizeof(boost::uint16_t)));

    ctrl_pkt_t pkt;
    pkt.pkt_meta.op = CTRL_PKT_OP_READ;
    pkt.pkt_meta.callbacks = 0;
    pkt.pkt_meta.seq = _seq++;
    pkt.pkt_meta.len = len;
    pkt.pkt_meta.addr = addr;
    boost::uint16_t pkt_buff[CTRL_PACKET_LENGTH / sizeof(boost::uint16_t)];

    pack_ctrl_pkt(pkt_buff, pkt);
    send_pkt(pkt_buff);

    //loop around waiting for the response to appear
    while(!get_ctrl_data(pkt.data, 0.05));

    return pkt.data;
}

/***********************************************************************
 * Viking marauders go pillaging for asynchronous control packets in the
 * control response endpoint. Sync packets go in sync_ctrl_fifo,
 * async TX error messages go in async_msg_fifo. sync_ctrl_fifo should
 * never have more than 1 message in it, since it's expected that we'll
 * wait for a control operation to finish before starting another one.
 **********************************************************************/
void b100_ctrl_impl::viking_marauder_loop(boost::barrier &spawn_barrier) {
    spawn_barrier.wait();
    set_thread_priority_safe();

    while (not boost::this_thread::interruption_requested()){
        managed_recv_buffer::sptr rbuf = _ctrl_transport->get_recv_buff();
        if(!rbuf.get()) continue; //that's ok, there are plenty of villages to pillage!
        const boost::uint16_t *pkt_buf = rbuf->cast<const boost::uint16_t *>();

        if(pkt_buf[0] >> 8 == CTRL_PACKET_HEADER_MAGIC) {
            //so it's got a control packet header, let's parse it.
            ctrl_pkt_t pkt;
            unpack_ctrl_pkt(pkt_buf, pkt);

            if(pkt.pkt_meta.seq != boost::uint8_t(_seq - 1)) {
                throw uhd::runtime_error("Sequence error on control channel");
            }
            if(pkt.pkt_meta.len > (CTRL_PACKET_LENGTH - CTRL_PACKET_HEADER_LENGTH)) {
                throw uhd::runtime_error("Control channel packet length too long");
            }

            //push it onto the queue
            sync_ctrl_fifo.push_with_wait(pkt.data);
        }
        else _async_cb(rbuf); //let the async callback handle it
    }
}

bool b100_ctrl_impl::get_ctrl_data(ctrl_data_t &pkt_data, double timeout){
    boost::this_thread::disable_interruption di; //disable because the wait can throw
    return sync_ctrl_fifo.pop_with_timed_wait(pkt_data, timeout);
}

/***********************************************************************
 * Public make function for b100_ctrl interface
 **********************************************************************/
b100_ctrl::sptr b100_ctrl::make(uhd::transport::zero_copy_if::sptr ctrl_transport, const async_cb_type &async_cb){
    return sptr(new b100_ctrl_impl(ctrl_transport, async_cb));
}
