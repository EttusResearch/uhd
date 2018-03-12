//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "n230_uart.hpp"

#include <uhd/transport/bounded_buffer.hpp>
#include <uhd/transport/vrt_if_packet.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/types/time_spec.hpp>
#include <uhd/exception.hpp>

using namespace uhd;
using namespace uhd::transport;

namespace uhd { namespace usrp { namespace n230 {

struct n230_uart_impl : n230_uart
{
    n230_uart_impl(zero_copy_if::sptr xport, const uint32_t sid):
        _xport(xport),
        _sid(sid),
        _count(0),
        _char_queue(4096)
    {
        //this default baud divider is over 9000
        this->set_baud_divider(9001);

        //create a task to handle incoming packets
        _recv_task = uhd::task::make(boost::bind(&n230_uart_impl::handle_recv, this));
    }

    void send_char(const char ch)
    {
        managed_send_buffer::sptr buff = _xport->get_send_buff();
        UHD_ASSERT_THROW(bool(buff));

        vrt::if_packet_info_t packet_info;
        packet_info.link_type = vrt::if_packet_info_t::LINK_TYPE_CHDR;
        packet_info.packet_type = vrt::if_packet_info_t::PACKET_TYPE_CONTEXT;
        packet_info.num_payload_words32 = 2;
        packet_info.num_payload_bytes = packet_info.num_payload_words32*sizeof(uint32_t);
        packet_info.packet_count = _count++;
        packet_info.sob = false;
        packet_info.eob = false;
        packet_info.sid = _sid;
        packet_info.has_sid = true;
        packet_info.has_cid = false;
        packet_info.has_tsi = false;
        packet_info.has_tsf = false;
        packet_info.has_tlr = false;

        uint32_t *packet_buff = buff->cast<uint32_t *>();
        vrt::if_hdr_pack_le(packet_buff, packet_info);
        packet_buff[packet_info.num_header_words32+0] = uhd::htonx(uint32_t(_baud_div));
        packet_buff[packet_info.num_header_words32+1] = uhd::htonx(uint32_t(ch));
        buff->commit(packet_info.num_packet_words32*sizeof(uint32_t));
    }

    void write_uart(const std::string &buff)
    {
        static bool r_sent = false;
        for (size_t i = 0; i < buff.size(); i++)
        {
            if (buff[i] == '\n' and not r_sent) this->send_char('\r');
            this->send_char(buff[i]);
            r_sent = (buff[i] == '\r');
        }
    }

    std::string read_uart(double timeout)
    {
        std::string line;
        char ch = '\0';
        while (_char_queue.pop_with_timed_wait(ch, timeout))
        {
            if (ch == '\r') continue;
            line += std::string(&ch, 1);
            if (ch == '\n') return line;
        }
        return line;
    }

    void handle_recv(void)
    {
        managed_recv_buffer::sptr buff = _xport->get_recv_buff();
        if (not buff)
            return;
        const uint32_t *packet_buff = buff->cast<const uint32_t *>();
        vrt::if_packet_info_t packet_info;
        packet_info.link_type = vrt::if_packet_info_t::LINK_TYPE_CHDR;
        packet_info.num_packet_words32 = buff->size()/sizeof(uint32_t);
        vrt::if_hdr_unpack_be(packet_buff, packet_info);
        const char ch = char(uhd::ntohx(packet_buff[packet_info.num_header_words32+1]));
        _char_queue.push_with_pop_on_full(ch);
    }

    void set_baud_divider(const double baud_div)
    {
        _baud_div = size_t(baud_div + 0.5);
    }

    const zero_copy_if::sptr _xport;
    const uint32_t _sid;
    size_t _count;
    size_t _baud_div;
    bounded_buffer<char> _char_queue;
    uhd::task::sptr _recv_task;
};


n230_uart::sptr n230_uart::make(zero_copy_if::sptr xport, const uint32_t sid)
{
    return n230_uart::sptr(new n230_uart_impl(xport, sid));
}

}}} //namespace
