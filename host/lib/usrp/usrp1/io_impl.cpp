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

/***********************************************************************
 * Pseudo send buffer implementation
 **********************************************************************/
class pseudo_managed_send_buffer : public managed_send_buffer{
public:

    pseudo_managed_send_buffer(
        const boost::asio::mutable_buffer &buff,
        const boost::function<ssize_t(size_t)> &commit
    ):
        _buff(buff),
        _commit(commit)
    {
        /* NOP */
    }

    ssize_t commit(size_t num_bytes){
        return _commit(num_bytes);
    }

private:
    const boost::asio::mutable_buffer &get(void) const{
        return _buff;
    }

    const boost::asio::mutable_buffer      _buff;
    const boost::function<ssize_t(size_t)> _commit;
};

/***********************************************************************
 * IO Implementation Details
 **********************************************************************/
struct usrp1_impl::io_impl{
    io_impl(zero_copy_if::sptr data_transport):
        data_transport(data_transport),
        underflow_poll_samp_count(0),
        overflow_poll_samp_count(0),
        send_buff(data_transport->get_send_buff()),
        num_bytes_committed(0)
    {
        /* NOP */
    }

    ~io_impl(void){
        flush_send_buff();
    }

    zero_copy_if::sptr data_transport;

    //state management for the vrt packet handler code
    vrt_packet_handler::recv_state packet_handler_recv_state;
    vrt_packet_handler::send_state packet_handler_send_state;

    //state management for overflow and underflow
    size_t underflow_poll_samp_count;
    size_t overflow_poll_samp_count;

    //wrapper around the actual send buffer interface
    //all of this to ensure only full buffers are committed
    managed_send_buffer::sptr send_buff;
    size_t num_bytes_committed;
    boost::uint8_t pseudo_buff[BYTES_PER_PACKET];
    ssize_t phony_commit_pseudo_buff(size_t num_bytes);
    ssize_t phony_commit_send_buff(size_t num_bytes);
    ssize_t commit_send_buff(void);
    void flush_send_buff(void);
    bool get_send_buffs(vrt_packet_handler::managed_send_buffs_t &);

    //helpers to get at the send buffer + offset
    inline void *get_send_mem_ptr(void){
        return send_buff->cast<boost::uint8_t *>() + num_bytes_committed;
    }
    inline size_t get_send_mem_size(void){
        return send_buff->size() - num_bytes_committed;
    }
};

/*!
 * Accept a commit of num bytes to the pseudo buffer.
 * Memcpy the entire contents of pseudo buffer into send buffers.
 *
 * Under most conditions:
 *   The first loop iteration will fill the remainder of the send buffer.
 *   The second loop iteration will empty the pseudo buffer remainder.
 */
ssize_t usrp1_impl::io_impl::phony_commit_pseudo_buff(size_t num_bytes){
    size_t bytes_to_copy = num_bytes, bytes_copied = 0;
    while(bytes_to_copy){
        size_t bytes_copied_here = std::min(bytes_to_copy, get_send_mem_size());
        std::memcpy(get_send_mem_ptr(), pseudo_buff + bytes_copied, bytes_copied_here);
        ssize_t ret = phony_commit_send_buff(bytes_copied_here);
        if (ret < 0) return ret;
        bytes_to_copy -= ret;
        bytes_copied += ret;
    }
    return bytes_copied;
}

/*!
 * Accept a commit of num bytes to the send buffer.
 * Conditionally commit the send buffer if full.
 */
ssize_t usrp1_impl::io_impl::phony_commit_send_buff(size_t num_bytes){
    num_bytes_committed += num_bytes;
    if (num_bytes_committed != send_buff->size()) return num_bytes;
    ssize_t ret = commit_send_buff();
    return (ret < 0)? ret : num_bytes;
}

/*!
 * Flush the send buffer:
 * Zero-pad the send buffer to the nearest 512 byte boundary and commit.
 */
void usrp1_impl::io_impl::flush_send_buff(void){
    size_t bytes_to_pad = (-1*num_bytes_committed)%512;
    std::memset(get_send_mem_ptr(), 0, bytes_to_pad);
    num_bytes_committed += bytes_to_pad;
    commit_send_buff();
}

/*!
 * Perform an actual commit on the send buffer:
 * Commit the contents of the send buffer and request a new buffer.
 */
ssize_t usrp1_impl::io_impl::commit_send_buff(void){
    ssize_t ret = send_buff->commit(num_bytes_committed);
    send_buff = data_transport->get_send_buff();
    num_bytes_committed = 0;
    return ret;
}

bool usrp1_impl::io_impl::get_send_buffs(
    vrt_packet_handler::managed_send_buffs_t &buffs
){
    UHD_ASSERT_THROW(buffs.size() == 1);

    //not enough bytes free -> use the pseudo buffer
    if (get_send_mem_size() < BYTES_PER_PACKET){
        buffs[0] = managed_send_buffer::sptr(new pseudo_managed_send_buffer(
            boost::asio::buffer(pseudo_buff),
            boost::bind(&usrp1_impl::io_impl::phony_commit_pseudo_buff, this, _1)
        ));
    }
    //otherwise use the send buffer offset by the bytes written
    else{
        buffs[0] = managed_send_buffer::sptr(new pseudo_managed_send_buffer(
            boost::asio::buffer(get_send_mem_ptr(), get_send_mem_size()),
            boost::bind(&usrp1_impl::io_impl::phony_commit_send_buff, this, _1)
        ));
    }

    return buffs[0].get() != NULL;
}

/***********************************************************************
 * Initialize internals within this file
 **********************************************************************/
void usrp1_impl::io_init(void){
    _rx_otw_type.width = 16;
    _rx_otw_type.shift = 0;
    _rx_otw_type.byteorder = otw_type_t::BO_LITTLE_ENDIAN;

    _tx_otw_type.width = 16;
    _tx_otw_type.shift = 0;
    _tx_otw_type.byteorder = otw_type_t::BO_LITTLE_ENDIAN;

    _io_impl = UHD_PIMPL_MAKE(io_impl, (_data_transport));
}

/***********************************************************************
 * Data send + helper functions
 **********************************************************************/
static void usrp1_bs_vrt_packer(
    boost::uint32_t *,
    vrt::if_packet_info_t &if_packet_info
){
    if_packet_info.num_header_words32 = 0;
    if_packet_info.num_packet_words32 = if_packet_info.num_payload_words32;
}

size_t usrp1_impl::send(
    const std::vector<const void *> &buffs, size_t num_samps,
    const tx_metadata_t &metadata, const io_type_t &io_type,
    send_mode_t send_mode
){
    size_t num_samps_sent = vrt_packet_handler::send(
        _io_impl->packet_handler_send_state,       //last state of the send handler
        buffs, num_samps,                          //buffer to fill
        metadata, send_mode,                       //samples metadata
        io_type, _tx_otw_type,                     //input and output types to convert
        _clock_ctrl->get_master_clock_freq(),      //master clock tick rate
        &usrp1_bs_vrt_packer,
        boost::bind(&usrp1_impl::io_impl::get_send_buffs, _io_impl.get(), _1),
        get_max_send_samps_per_packet(),
        0,                                         //vrt header offset
        _tx_subdev_spec.size()                     //num channels
    );

    //Don't honor sob because it is normal to be always bursting...
    //handle eob flag (commit the buffer)
    if (metadata.end_of_burst) _io_impl->flush_send_buff();

    //handle the polling for underflow conditions
    _io_impl->underflow_poll_samp_count += num_samps_sent;
    if (_io_impl->underflow_poll_samp_count >= _tx_samps_per_poll_interval){
        _io_impl->underflow_poll_samp_count = 0; //reset count
        boost::uint8_t underflow = 0;
        ssize_t ret = _ctrl_transport->usrp_control_read(
            VRQ_GET_STATUS, 0, GS_TX_UNDERRUN,
            &underflow, sizeof(underflow)
        );
        if (ret < 0)        std::cerr << "USRP: underflow check failed" << std::endl;
        else if (underflow) std::cerr << "U" << std::flush;
    }

    return num_samps_sent;
}

/***********************************************************************
 * Data recv + helper functions
 **********************************************************************/
static void usrp1_bs_vrt_unpacker(
    const boost::uint32_t *,
    vrt::if_packet_info_t &if_packet_info
){
    if_packet_info.packet_type = vrt::if_packet_info_t::PACKET_TYPE_DATA;
    if_packet_info.num_payload_words32 = if_packet_info.num_packet_words32;
    if_packet_info.num_header_words32 = 0;
    if_packet_info.packet_count = 0;
    if_packet_info.sob = false;
    if_packet_info.eob = false;
    if_packet_info.has_sid = false;
    if_packet_info.has_cid = false;
    if_packet_info.has_tsi = false;
    if_packet_info.has_tsf = false;
    if_packet_info.has_tlr = false;
}

static bool get_recv_buffs(
    zero_copy_if::sptr zc_if, size_t timeout_ms,
    vrt_packet_handler::managed_recv_buffs_t &buffs
){
    UHD_ASSERT_THROW(buffs.size() == 1);
    buffs[0] = zc_if->get_recv_buff(timeout_ms);
    return buffs[0].get() != NULL;
}

size_t usrp1_impl::recv(
    const std::vector<void *> &buffs, size_t num_samps,
    rx_metadata_t &metadata, const io_type_t &io_type,
    recv_mode_t recv_mode, size_t timeout_ms
){
    size_t num_samps_recvd = vrt_packet_handler::recv(
        _io_impl->packet_handler_recv_state,       //last state of the recv handler
        buffs, num_samps,                          //buffer to fill
        metadata, recv_mode,                       //samples metadata
        io_type, _rx_otw_type,                     //input and output types to convert
        _clock_ctrl->get_master_clock_freq(),      //master clock tick rate
        &usrp1_bs_vrt_unpacker,
        boost::bind(&get_recv_buffs, _data_transport, timeout_ms, _1),
        &vrt_packet_handler::handle_overflow_nop,
        0,                                         //vrt header offset
        _rx_subdev_spec.size()                     //num channels
    );

    //handle the polling for overflow conditions
    _io_impl->overflow_poll_samp_count += num_samps_recvd;
    if (_io_impl->overflow_poll_samp_count >= _rx_samps_per_poll_interval){
        _io_impl->overflow_poll_samp_count = 0; //reset count
        boost::uint8_t overflow = 0;
        ssize_t ret = _ctrl_transport->usrp_control_read(
            VRQ_GET_STATUS, 0, GS_RX_OVERRUN,
            &overflow, sizeof(overflow)
        );
        if (ret < 0)       std::cerr << "USRP: overflow check failed" << std::endl;
        else if (overflow) std::cerr << "O" << std::flush;
    }

    return num_samps_recvd;
}
