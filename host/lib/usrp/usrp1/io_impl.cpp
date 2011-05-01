//
// Copyright 2010-2011 Ettus Research LLC
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
#include <uhd/utils/safe_call.hpp>
#include <uhd/utils/thread_priority.hpp>
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

static const size_t alignment_padding = 512;

/***********************************************************************
 * Helper struct to associate an offset with a buffer
 **********************************************************************/
struct offset_send_buffer{
    offset_send_buffer(void){
        /* NOP */
    }

    offset_send_buffer(managed_send_buffer::sptr buff, size_t offset = 0):
        buff(buff), offset(offset)
    {
        /* NOP */
    }

    //member variables
    managed_send_buffer::sptr buff;
    size_t offset; /* in bytes */
};

/***********************************************************************
 * Reusable managed send buffer to handle aligned commits
 **********************************************************************/
class offset_managed_send_buffer : public managed_send_buffer{
public:
    typedef boost::function<void(offset_send_buffer&, offset_send_buffer&, size_t)> commit_cb_type;
    offset_managed_send_buffer(const commit_cb_type &commit_cb):
        _expired(true), _commit_cb(commit_cb)
    {
        /* NOP */
    }

    bool expired(void){return _expired;}

    void commit(size_t size){
        if (_expired) return;
        this->_commit_cb(_curr_buff, _next_buff, size);
        _expired = true;
    }

    sptr get_new(
        offset_send_buffer &curr_buff,
        offset_send_buffer &next_buff
    ){
        _expired = false;
        _curr_buff = curr_buff;
        _next_buff = next_buff;
        return sptr(this, &offset_managed_send_buffer::fake_deleter);
    }

private:
    static void fake_deleter(void *){
        //dont do anything and assume the bastard committed it
        //static_cast<offset_managed_send_buffer *>(obj)->commit(0);
    }

    void  *get_buff(void) const{return _curr_buff.buff->cast<char *>() + _curr_buff.offset;}
    size_t get_size(void) const{return _curr_buff.buff->size()         - _curr_buff.offset;}

    bool _expired;
    offset_send_buffer _curr_buff, _next_buff;
    commit_cb_type _commit_cb;
};

/***********************************************************************
 * IO Implementation Details
 **********************************************************************/
struct usrp1_impl::io_impl{
    io_impl(zero_copy_if::sptr data_transport):
        data_transport(data_transport),
        get_recv_buffs_fcn(boost::bind(&usrp1_impl::io_impl::get_recv_buffs, this, _1)),
        get_send_buffs_fcn(boost::bind(&usrp1_impl::io_impl::get_send_buffs, this, _1)),
        underflow_poll_samp_count(0),
        overflow_poll_samp_count(0),
        curr_buff(offset_send_buffer(data_transport->get_send_buff())),
        omsb(boost::bind(&usrp1_impl::io_impl::commit_send_buff, this, _1, _2, _3))
    {
        /* NOP */
    }

    ~io_impl(void){
        UHD_SAFE_CALL(flush_send_buff();)
    }

    zero_copy_if::sptr data_transport;

    //timeouts set on calls to recv/send (passed into get buffs methods)
    double recv_timeout, send_timeout;

    //bound callbacks for get buffs (bound once here, not in fast-path)
    vrt_packet_handler::get_recv_buffs_t get_recv_buffs_fcn;
    vrt_packet_handler::get_send_buffs_t get_send_buffs_fcn;

    //state management for the vrt packet handler code
    vrt_packet_handler::recv_state packet_handler_recv_state;
    vrt_packet_handler::send_state packet_handler_send_state;

    //state management for overflow and underflow
    size_t underflow_poll_samp_count;
    size_t overflow_poll_samp_count;

    //wrapper around the actual send buffer interface
    //all of this to ensure only aligned lengths are committed
    //NOTE: you must commit before getting a new buffer
    //since the vrt packet handler obeys this, we are ok
    offset_send_buffer curr_buff;
    offset_managed_send_buffer omsb;
    void commit_send_buff(offset_send_buffer&, offset_send_buffer&, size_t);
    void flush_send_buff(void);
    bool get_send_buffs(vrt_packet_handler::managed_send_buffs_t &);
    bool get_recv_buffs(vrt_packet_handler::managed_recv_buffs_t &buffs){
        UHD_ASSERT_THROW(buffs.size() == 1);
        buffs[0] = data_transport->get_recv_buff(recv_timeout);
        return buffs[0].get() != NULL;
    }
};

/*!
 * Perform an actual commit on the send buffer:
 * Copy the remainder of alignment to the next buffer.
 * Commit the current buffer at multiples of alignment.
 */
void usrp1_impl::io_impl::commit_send_buff(
    offset_send_buffer &curr,
    offset_send_buffer &next,
    size_t num_bytes
){
    //total number of bytes now in the current buffer
    size_t bytes_in_curr_buffer = curr.offset + num_bytes;

    //calculate how many to commit and remainder
    size_t num_bytes_remaining = bytes_in_curr_buffer % alignment_padding;
    size_t num_bytes_to_commit = bytes_in_curr_buffer - num_bytes_remaining;

    //copy the remainder into the next buffer
    std::memcpy(
        next.buff->cast<char *>() + next.offset,
        curr.buff->cast<char *>() + num_bytes_to_commit,
        num_bytes_remaining
    );

    //update the offset into the next buffer
    next.offset += num_bytes_remaining;

    //commit the current buffer
    curr.buff->commit(num_bytes_to_commit);

    //store the next buffer for the next call
    curr_buff = next;
}

/*!
 * Flush the current buffer by padding out to alignment and committing.
 */
void usrp1_impl::io_impl::flush_send_buff(void){
    //calculate the number of bytes to alignment
    size_t bytes_to_pad = (-1*curr_buff.offset)%alignment_padding;

    //send at least alignment_padding to guarantee zeros are sent
    if (bytes_to_pad == 0) bytes_to_pad = alignment_padding;

    //get the buffer, clear, and commit (really current buffer)
    vrt_packet_handler::managed_send_buffs_t buffs(1);
    if (this->get_send_buffs(buffs)){
        std::memset(buffs[0]->cast<void *>(), 0, bytes_to_pad);
        buffs[0]->commit(bytes_to_pad);
    }
}

/*!
 * Get a managed send buffer with the alignment padding:
 * Always grab the next send buffer so we can timeout here.
 */
bool usrp1_impl::io_impl::get_send_buffs(
    vrt_packet_handler::managed_send_buffs_t &buffs
){
    UHD_ASSERT_THROW(omsb.expired() and buffs.size() == 1);

    //try to get a new managed buffer with timeout
    offset_send_buffer next_buff(data_transport->get_send_buff(send_timeout));
    if (not next_buff.buff.get()) return false; /* propagate timeout here */

    //make a new managed buffer with the offset buffs
    buffs[0] = omsb.get_new(curr_buff, next_buff);

    return true;
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

    _soft_time_ctrl = soft_time_ctrl::make(
        boost::bind(&usrp1_impl::rx_stream_on_off, this, _1)
    );

    rx_stream_on_off(false);
    _io_impl->flush_send_buff();
}

void usrp1_impl::rx_stream_on_off(bool enb){
    return _iface->write_firmware_cmd(VRQ_FPGA_SET_RX_ENABLE, enb, 0, 0, 0);
    //drain any junk in the receive transport after stop streaming command
    while(not enb and _data_transport->get_recv_buff().get() != NULL){
        /* NOP */
    }
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

size_t usrp1_impl::get_max_send_samps_per_packet(void) const {
    return (_data_transport->get_send_frame_size() - alignment_padding)
        / _tx_otw_type.get_sample_size()
        / _tx_subdev_spec.size()
    ;
}

size_t usrp1_impl::send(
    const send_buffs_type &buffs, size_t num_samps,
    const tx_metadata_t &metadata, const io_type_t &io_type,
    send_mode_t send_mode, double timeout
){
    if (_soft_time_ctrl->send_pre(metadata, timeout)) return num_samps;

    _io_impl->send_timeout = timeout;
    size_t num_samps_sent = vrt_packet_handler::send(
        _io_impl->packet_handler_send_state,       //last state of the send handler
        buffs, num_samps,                          //buffer to fill
        metadata, send_mode,                       //samples metadata
        io_type, _tx_otw_type,                     //input and output types to convert
        _clock_ctrl->get_master_clock_freq(),      //master clock tick rate
        &usrp1_bs_vrt_packer,
        _io_impl->get_send_buffs_fcn,
        get_max_send_samps_per_packet(),
        0,                                         //vrt header offset
        _tx_subdev_spec.size()                     //num channels
    );

    //handle eob flag (commit the buffer, disable the DACs)
    //check num samps sent to avoid flush on incomplete/timeout
    if (metadata.end_of_burst and num_samps_sent == num_samps){
        _io_impl->flush_send_buff();
    }

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

size_t usrp1_impl::get_max_recv_samps_per_packet(void) const {
    return _data_transport->get_recv_frame_size()
        / _rx_otw_type.get_sample_size()
        / _rx_subdev_spec.size()
    ;
}

size_t usrp1_impl::recv(
    const recv_buffs_type &buffs, size_t num_samps,
    rx_metadata_t &metadata, const io_type_t &io_type,
    recv_mode_t recv_mode, double timeout
){
    _io_impl->recv_timeout = timeout;
    size_t num_samps_recvd = vrt_packet_handler::recv(
        _io_impl->packet_handler_recv_state,       //last state of the recv handler
        buffs, num_samps,                          //buffer to fill
        metadata, recv_mode,                       //samples metadata
        io_type, _rx_otw_type,                     //input and output types to convert
        _clock_ctrl->get_master_clock_freq(),      //master clock tick rate
        &usrp1_bs_vrt_unpacker,
        _io_impl->get_recv_buffs_fcn,
        &vrt_packet_handler::handle_overflow_nop,
        0,                                         //vrt header offset
        _rx_subdev_spec.size()                     //num channels
    );

    _soft_time_ctrl->recv_post(metadata, num_samps_recvd);

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
