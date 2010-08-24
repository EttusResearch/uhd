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

static const float poll_interval = 0.1;  //100ms

struct usrp1_send_state {
    uhd::transport::managed_send_buffer::sptr send_buff;
    size_t bytes_written;
    size_t underrun_poll_samp_count;

    size_t bytes_free()
    {
        if (send_buff != NULL)
            return send_buff->size() - bytes_written;
        else
            return 0;
    }
};

struct usrp1_recv_state {
    uhd::transport::managed_recv_buffer::sptr recv_buff;
    size_t bytes_read;
    size_t overrun_poll_samp_count;

    size_t bytes_avail()
    {
        if (recv_buff != NULL)
            return recv_buff->size() - bytes_read;
        else
            return 0;
    }
};

/***********************************************************************
 * IO Implementation Details
 **********************************************************************/
struct usrp1_impl::io_impl {
    io_impl();
    ~io_impl(void);

    //state handling for buffer management 
    usrp1_recv_state recv_state;
    usrp1_send_state send_state;

    //send transport management 
    bool get_send_buffer(zero_copy_if::sptr zc_if);
    size_t copy_convert_send_samps(const void *buff, size_t num_samps,
                              size_t sample_offset, const io_type_t io_type,
                              otw_type_t otw_type);
    bool conditional_buff_commit(bool force);
    bool check_underrun(usrp_ctrl::sptr ctrl_if,
                        size_t poll_interval, bool force);

    //recv transport management 
    bool get_recv_buffer(zero_copy_if::sptr zc_if);
    size_t copy_convert_recv_samps(void *buff, size_t num_samps,
                              size_t sample_offset, const io_type_t io_type,
                              otw_type_t otw_type);
    bool check_overrun(usrp_ctrl::sptr ctrl_if,
                        size_t poll_interval, bool force);
};

usrp1_impl::io_impl::io_impl()
{
    send_state.send_buff = uhd::transport::managed_send_buffer::sptr();
    recv_state.recv_buff = uhd::transport::managed_recv_buffer::sptr();
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

    _io_impl = UHD_PIMPL_MAKE(io_impl, ());
}

/***********************************************************************
 * Data Send
 **********************************************************************/
bool usrp1_impl::io_impl::get_send_buffer(zero_copy_if::sptr zc_if)
{
    if (send_state.send_buff == NULL) {

        send_state.send_buff = zc_if->get_send_buff();
        if (send_state.send_buff == NULL)
            return false;

        send_state.bytes_written = 0;
    }

    return true;
}

size_t usrp1_impl::io_impl::copy_convert_send_samps(const void *buff,
                                                    size_t num_samps,
                                                    size_t sample_offset,
                                                    const io_type_t io_type,
                                                    otw_type_t otw_type)
{
    UHD_ASSERT_THROW(send_state.bytes_free() % otw_type.get_sample_size() == 0);

    size_t samps_free = send_state.bytes_free() / otw_type.get_sample_size();
    size_t copy_samps = std::min(num_samps - sample_offset, samps_free); 

    const boost::uint8_t *io_mem =
        reinterpret_cast<const boost::uint8_t *>(buff);

    boost::uint8_t *otw_mem = send_state.send_buff->cast<boost::uint8_t *>();

    convert_io_type_to_otw_type(io_mem + sample_offset * io_type.size,
                                io_type,
                                otw_mem + send_state.bytes_written,
                                otw_type,
                                copy_samps);

    send_state.bytes_written += copy_samps * otw_type.get_sample_size();
    send_state.underrun_poll_samp_count += copy_samps;

    return copy_samps;
}

bool usrp1_impl::io_impl::conditional_buff_commit(bool force)
{
    if (send_state.bytes_written % 512)
        return false;

    if (force || send_state.bytes_free() == 0) {
        send_state.send_buff->commit(send_state.bytes_written);
        send_state.send_buff = uhd::transport::managed_send_buffer::sptr();
        return true;
    }
    
    return false;
}

bool usrp1_impl::io_impl::check_underrun(usrp_ctrl::sptr ctrl_if,
                                         size_t poll_interval,
                                         bool force)
{
    unsigned char underrun = 0;

    bool ready_to_poll = send_state.underrun_poll_samp_count > poll_interval;

    if (force || ready_to_poll) {
        int ret = ctrl_if->usrp_control_read(VRQ_GET_STATUS,
                                             0,
                                             GS_TX_UNDERRUN,
                                             &underrun, sizeof(char));
        if (ret < 0)
            std::cerr << "USRP: underrun check failed" << std::endl;
        if (underrun)
            std::cerr << "U" << std::endl;

        send_state.underrun_poll_samp_count = 0;
    }

    return (bool) underrun;
}

size_t usrp1_impl::send(const std::vector<const void *> &buffs,
                        size_t num_samps,
                        const tx_metadata_t &,
                        const io_type_t &io_type,
                        send_mode_t)
{
    UHD_ASSERT_THROW(buffs.size() == 1);

    size_t total_samps_sent = 0;

    while (total_samps_sent < num_samps) {
        if (!_io_impl->get_send_buffer(_data_transport))
            return 0;

        total_samps_sent += _io_impl->copy_convert_send_samps(buffs[0],
                                                              num_samps,
                                                              total_samps_sent,
                                                              io_type,
                                                              _tx_otw_type);
        if (total_samps_sent == num_samps)
            _io_impl->conditional_buff_commit(true);
        else
            _io_impl->conditional_buff_commit(false);

        _io_impl->check_underrun(_ctrl_transport,
                                 _tx_samps_per_poll_interval, false);
    }

    return total_samps_sent;
}

/***********************************************************************
 * Data Recv
 **********************************************************************/
bool usrp1_impl::io_impl::get_recv_buffer(zero_copy_if::sptr zc_if)
{
    if ((recv_state.recv_buff == NULL) || (recv_state.bytes_avail() == 0)) {

        recv_state.recv_buff = zc_if->get_recv_buff();
        if (recv_state.recv_buff == NULL)
            return false;

        recv_state.bytes_read = 0;
    }

    return true;
}

size_t usrp1_impl::io_impl::copy_convert_recv_samps(void *buff,
                                                    size_t num_samps,
                                                    size_t sample_offset,
                                                    const io_type_t io_type,
                                                    otw_type_t otw_type)
{
    UHD_ASSERT_THROW(recv_state.bytes_avail() % otw_type.get_sample_size() == 0);

    size_t samps_avail = recv_state.bytes_avail() / otw_type.get_sample_size();
    size_t copy_samps = std::min(num_samps - sample_offset, samps_avail); 

    const boost::uint8_t *otw_mem =
        recv_state.recv_buff->cast<const boost::uint8_t *>();

    boost::uint8_t *io_mem = reinterpret_cast<boost::uint8_t *>(buff);

    convert_otw_type_to_io_type(otw_mem + recv_state.bytes_read,
                                otw_type,
                                io_mem + sample_offset * io_type.size,
                                io_type,
                                copy_samps);

    recv_state.bytes_read += copy_samps * otw_type.get_sample_size();
    recv_state.overrun_poll_samp_count += copy_samps;

    return copy_samps;
}

bool usrp1_impl::io_impl::check_overrun(usrp_ctrl::sptr ctrl_if,
                                        size_t poll_interval,
                                        bool force)
{
    unsigned char overrun = 0;

    bool ready_to_poll = recv_state.overrun_poll_samp_count > poll_interval;

    if (force || ready_to_poll) {
        int ret = ctrl_if->usrp_control_read(VRQ_GET_STATUS,
                                             0,
                                             GS_RX_OVERRUN,
                                             &overrun, sizeof(char));
        if (ret < 0)
            std::cerr << "USRP: overrrun check failed" << std::endl;
        if (overrun)
            std::cerr << "O" << std::endl;

        recv_state.overrun_poll_samp_count = 0;
    }

    return (bool) overrun;
}

size_t usrp1_impl::recv(const std::vector<void *> &buffs,
                        size_t num_samps,
                        rx_metadata_t &,
                        const io_type_t &io_type,
                        recv_mode_t,
                        size_t)
{
    UHD_ASSERT_THROW(buffs.size() == 1);

    size_t total_samps_recv = 0;

    while (total_samps_recv < num_samps) {

        if (!_io_impl->get_recv_buffer(_data_transport))
            return 0;

        total_samps_recv += _io_impl->copy_convert_recv_samps(buffs[0],
                                                              num_samps,
                                                              total_samps_recv,
                                                              io_type,
                                                              _rx_otw_type);
        _io_impl->check_overrun(_ctrl_transport,
                                _rx_samps_per_poll_interval, false);
    }

    return total_samps_recv; 
}
