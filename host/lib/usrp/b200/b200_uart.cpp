//
// Copyright 2013 Ettus Research LLC
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

#include "b200_uart.hpp"
#include <uhd/transport/bounded_buffer.hpp>
#include <uhd/transport/vrt_if_packet.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/types/time_spec.hpp>
#include <uhd/exception.hpp>

using namespace uhd;
using namespace uhd::transport;

struct b200_uart_impl : b200_uart
{
    b200_uart_impl(zero_copy_if::sptr xport, const boost::uint32_t sid):
        _xport(xport),
        _sid(sid),
        _count(0),
        _char_queue(4096)
    {
        //this default baud divider is over 9000
        this->set_baud_divider(9001);
    }

    void send_char(const char ch)
    {
        managed_send_buffer::sptr buff = _xport->get_send_buff();
        UHD_ASSERT_THROW(bool(buff));

        vrt::if_packet_info_t packet_info;
        packet_info.link_type = vrt::if_packet_info_t::LINK_TYPE_CHDR;
        packet_info.packet_type = vrt::if_packet_info_t::PACKET_TYPE_CONTEXT;
        packet_info.num_payload_words32 = 2;
        packet_info.num_payload_bytes = packet_info.num_payload_words32*sizeof(boost::uint32_t);
        packet_info.packet_count = _count++;
        packet_info.sob = false;
        packet_info.eob = false;
        packet_info.sid = _sid;
        packet_info.has_sid = true;
        packet_info.has_cid = false;
        packet_info.has_tsi = false;
        packet_info.has_tsf = false;
        packet_info.has_tlr = false;

        boost::uint32_t *packet_buff = buff->cast<boost::uint32_t *>();
        vrt::if_hdr_pack_le(packet_buff, packet_info);
        packet_buff[packet_info.num_header_words32+0] = uhd::htowx(boost::uint32_t(_baud_div));
        packet_buff[packet_info.num_header_words32+1] = uhd::htowx(boost::uint32_t(ch));
        buff->commit(packet_info.num_packet_words32*sizeof(boost::uint32_t));
    }

    void write_uart(const std::string &buff)
    {
        for (size_t i = 0; i < buff.size(); i++)
        {
            if (buff[i] == '\n') this->send_char('\r');
            this->send_char(buff[i]);
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

    void handle_uart_packet(managed_recv_buffer::sptr buff)
    {
        const boost::uint32_t *packet_buff = buff->cast<const boost::uint32_t *>();
        vrt::if_packet_info_t packet_info;
        packet_info.link_type = vrt::if_packet_info_t::LINK_TYPE_CHDR;
        packet_info.num_packet_words32 = buff->size()/sizeof(boost::uint32_t);
        vrt::if_hdr_unpack_le(packet_buff, packet_info);
        const char ch = char(uhd::wtohx(packet_buff[packet_info.num_header_words32+1]));
        _char_queue.push_with_pop_on_full(ch);
    }

    void set_baud_divider(const double baud_div)
    {
        _baud_div = size_t(baud_div + 0.5);
    }

    const zero_copy_if::sptr _xport;
    const boost::uint32_t _sid;
    size_t _count;
    size_t _baud_div;
    bounded_buffer<char> _char_queue;
};


b200_uart::sptr b200_uart::make(zero_copy_if::sptr xport, const boost::uint32_t sid)
{
    return b200_uart::sptr(new b200_uart_impl(xport, sid));
}
