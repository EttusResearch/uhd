//
// Copyright 2010 Ettus Research LLC
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

#include "../../transport/vrt_packet_handler.hpp"
#include "usrp_commands.h"
#include "usrp1_impl.hpp"
#include <uhd/utils/thread_priority.hpp>
#include <uhd/transport/convert_types.hpp>
#include <uhd/transport/bounded_buffer.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <iostream>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;
namespace asio = boost::asio;

/*
 * The FX2 firmware bursts data to the FPGA in 512 byte chunks so
 * maintain send state to make sure that happens.
 */
struct usrp1_send_state {
    uhd::transport::managed_send_buffer::sptr send_buff;
    size_t bytes_used;
    size_t bytes_free;
};

/***********************************************************************
 * IO Implementation Details
 **********************************************************************/
struct usrp1_impl::io_impl {
    io_impl(zero_copy_if::sptr zc_if);
    ~io_impl(void);

    bool get_recv_buff(managed_recv_buffer::sptr buff); 

    //state management for the vrt packet handler code
    vrt_packet_handler::recv_state packet_handler_recv_state;
    usrp1_send_state send_state;

    zero_copy_if::sptr data_transport;
    unsigned int count;
};

usrp1_impl::io_impl::io_impl(zero_copy_if::sptr zc_if)
 : packet_handler_recv_state(1), data_transport(zc_if), count(0)
{
    /* NOP */
}

usrp1_impl::io_impl::~io_impl(void)
{
   /* NOP */
}

void usrp1_impl::io_init(void)
{
    _rx_otw_type.width = 16;
    _rx_otw_type.shift = 0;
    _rx_otw_type.byteorder = otw_type_t::BO_LITTLE_ENDIAN;

    _tx_otw_type.width = 16;
    _tx_otw_type.shift = 0;
    _tx_otw_type.byteorder = otw_type_t::BO_LITTLE_ENDIAN;

    _io_impl = UHD_PIMPL_MAKE(io_impl, (_data_transport));
}

/***********************************************************************
 * Data Send
 **********************************************************************/
size_t usrp1_impl::send(const std::vector<const void *> &buffs,
                        size_t num_samps,
                        const tx_metadata_t &,
                        const io_type_t &io_type,
                        send_mode_t)
{
    UHD_ASSERT_THROW(buffs.size() == 1);

    size_t total_samps_sent = 0;

    while (total_samps_sent < num_samps) {

        if (_io_impl->send_state.send_buff == NULL) {
            _io_impl->send_state.send_buff = _data_transport->get_send_buff();
            if (_io_impl->send_state.send_buff == NULL) {
                return 0;
            }
            _io_impl->send_state.bytes_used = 0;
            _io_impl->send_state.bytes_free = _io_impl->send_state.send_buff->size();
        }

        size_t copy_samps =
               std::min(num_samps - total_samps_sent, _io_impl->send_state.bytes_free / _tx_otw_type.get_sample_size());

        const boost::uint8_t *io_mem =
                             reinterpret_cast<const boost::uint8_t *>(buffs[0]);

        boost::uint8_t *otw_mem = _io_impl->send_state.send_buff->cast<boost::uint8_t *>();

        // Type conversion and copy 
        convert_io_type_to_otw_type(
                     io_mem + total_samps_sent * io_type.size,
                     io_type,
                     otw_mem + _io_impl->send_state.bytes_used,
                     _tx_otw_type,
                     copy_samps);
 
        _io_impl->send_state.bytes_used += copy_samps * _tx_otw_type.get_sample_size();
        _io_impl->send_state.bytes_free -= copy_samps * _tx_otw_type.get_sample_size();

        if (_io_impl->send_state.bytes_free == 0) {
            _io_impl->send_state.send_buff->commit(_io_impl->send_state.bytes_used);
            _io_impl->send_state.send_buff = uhd::transport::managed_send_buffer::sptr();
        }

        total_samps_sent += copy_samps; 
 
        //check for underruns
        if (!(_io_impl->count++ % 1000)) {
            unsigned char underrun;
            int ret = _ctrl_transport->usrp_control_read(VRQ_GET_STATUS,
                                                         0,
                                                         GS_TX_UNDERRUN,
                                                         &underrun, sizeof(char));
            if (ret < 0)
                std::cerr << "error: underrun check failed" << std::endl;
            if (underrun)
                std::cerr << "U" << std::endl;
        }
    }

    return total_samps_sent;
}

/***********************************************************************
 * Data Recv
 **********************************************************************/
void _recv_helper(vrt_packet_handler::recv_state &state)
{
    size_t num_packet_words32 =
                       state.managed_buffs[0]->size() / sizeof(boost::uint32_t);

    const boost::uint32_t *data =
                        state.managed_buffs[0]->cast<const boost::uint32_t *>();

    state.copy_buffs[0] = reinterpret_cast<const boost::uint8_t *>(data);
    size_t num_payload_bytes = num_packet_words32 * sizeof(boost::uint32_t);
    state.size_of_copy_buffs = num_payload_bytes;
}

size_t usrp1_impl::recv(const std::vector<void *> &buffs,
                        size_t num_samps,
                        rx_metadata_t &,
                        const io_type_t &io_type,
                        recv_mode_t,
                        size_t)
{
    UHD_ASSERT_THROW(_io_impl->packet_handler_recv_state.width == 1);
    UHD_ASSERT_THROW(buffs.size() == 1);

    size_t sent_samps = 0;
    size_t nsamps_to_copy = 0;; 

    while (sent_samps < num_samps) {
        if (_io_impl->packet_handler_recv_state.size_of_copy_buffs == 0) {
            _io_impl->packet_handler_recv_state.fragment_offset_in_samps = 0;
            _io_impl->packet_handler_recv_state.managed_buffs[0] =
                                          _io_impl->data_transport->get_recv_buff();
 
            //timeout or something bad returns zero
            if (!_io_impl->packet_handler_recv_state.managed_buffs[0].get())
                return 0;
 
            _recv_helper(_io_impl->packet_handler_recv_state);
        }

        size_t bytes_per_item = _rx_otw_type.get_sample_size();
        size_t nsamps_available =
            _io_impl->packet_handler_recv_state.size_of_copy_buffs / bytes_per_item;
        nsamps_to_copy = std::min(num_samps, nsamps_available);
        size_t bytes_to_copy = nsamps_to_copy * bytes_per_item;

        convert_otw_type_to_io_type(
              _io_impl->packet_handler_recv_state.copy_buffs[0],
              _rx_otw_type,
              reinterpret_cast<boost::uint8_t *>(buffs[0]) + sent_samps * io_type.size,
              io_type,
              nsamps_to_copy);
 
        _io_impl->packet_handler_recv_state.copy_buffs[0] += bytes_to_copy; 
        _io_impl->packet_handler_recv_state.size_of_copy_buffs -= bytes_to_copy;

        sent_samps += nsamps_to_copy;

        //check for overruns
        if (!(_io_impl->count++ % 10000)) {
            unsigned char overrun;
            int ret = _ctrl_transport->usrp_control_read(
                                   VRQ_GET_STATUS,
                                   0,
                                   GS_RX_OVERRUN,
                                   &overrun, sizeof(char));
            if (ret < 0)
                std::cerr << "error: overrun check failed" << std::endl;
            if (overrun)
                std::cerr << "O" << std::endl;
        }
    }
    return sent_samps; 
}
