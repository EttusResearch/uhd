//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "b200_uart.hpp"
#include "b200_impl.hpp"
#include <uhd/transport/bounded_buffer.hpp>
#include <uhd/transport/vrt_if_packet.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/types/time_spec.hpp>
#include <uhd/exception.hpp>

using namespace uhd;
using namespace uhd::transport;

struct b200_uart_impl : b200_uart
{
    b200_uart_impl(zero_copy_if::sptr xport, const uint32_t sid):
        _xport(xport),
        _sid(sid),
        _count(0),
        _baud_div(std::floor(B200_BUS_CLOCK_RATE/115200 + 0.5)),
        _line_queue(4096)
    {
        /*NOP*/
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
        packet_buff[packet_info.num_header_words32+0] = uhd::htowx(uint32_t(_baud_div));
        packet_buff[packet_info.num_header_words32+1] = uhd::htowx(uint32_t(ch));
        buff->commit(packet_info.num_packet_words32*sizeof(uint32_t));
    }

    void write_uart(const std::string &buff)
    {
        for(const char ch:  buff)
        {
            this->send_char(ch);
        }
    }

    std::string read_uart(double timeout)
    {
        std::string line;
        _line_queue.pop_with_timed_wait(line, timeout);
        return line;
    }

    void handle_uart_packet(managed_recv_buffer::sptr buff)
    {
        const uint32_t *packet_buff = buff->cast<const uint32_t *>();
        vrt::if_packet_info_t packet_info;
        packet_info.link_type = vrt::if_packet_info_t::LINK_TYPE_CHDR;
        packet_info.num_packet_words32 = buff->size()/sizeof(uint32_t);
        vrt::if_hdr_unpack_le(packet_buff, packet_info);
        const char ch = char(uhd::wtohx(packet_buff[packet_info.num_header_words32+1]));
        _line += ch;
        if (ch == '\n')
        {
            _line_queue.push_with_pop_on_full(_line);
            _line.clear();
        }
    }

    const zero_copy_if::sptr _xport;
    const uint32_t _sid;
    size_t _count;
    size_t _baud_div;
    bounded_buffer<std::string> _line_queue;
    std::string _line;
};


b200_uart::sptr b200_uart::make(zero_copy_if::sptr xport, const uint32_t sid)
{
    return b200_uart::sptr(new b200_uart_impl(xport, sid));
}
