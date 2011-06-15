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

#define SRPH_DONT_CHECK_SEQUENCE
#include "../../transport/super_recv_packet_handler.hpp"
#include "../../transport/super_send_packet_handler.hpp"
#include "usrp_commands.h"
#include "usrp1_impl.hpp"
#include <uhd/utils/msg.hpp>
#include <uhd/utils/safe_call.hpp>
#include <uhd/utils/thread_priority.hpp>
#include <uhd/usrp/dsp_props.hpp>
#include <uhd/transport/bounded_buffer.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

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
        _commit_cb(commit_cb)
    {
        /* NOP */
    }

    void commit(size_t size){
        if (size != 0) this->_commit_cb(_curr_buff, _next_buff, size);
    }

    sptr get_new(
        offset_send_buffer &curr_buff,
        offset_send_buffer &next_buff
    ){
        _curr_buff = curr_buff;
        _next_buff = next_buff;
        return make_managed_buffer(this);
    }

private:
    void  *get_buff(void) const{return _curr_buff.buff->cast<char *>() + _curr_buff.offset;}
    size_t get_size(void) const{return _curr_buff.buff->size()         - _curr_buff.offset;}

    offset_send_buffer _curr_buff, _next_buff;
    commit_cb_type _commit_cb;
};

/***********************************************************************
 * BS VRT packer/unpacker functions (since samples don't have headers)
 **********************************************************************/
static void usrp1_bs_vrt_packer(
    boost::uint32_t *,
    vrt::if_packet_info_t &if_packet_info
){
    if_packet_info.num_header_words32 = 0;
    if_packet_info.num_packet_words32 = if_packet_info.num_payload_words32;
}

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

/***********************************************************************
 * IO Implementation Details
 **********************************************************************/
struct usrp1_impl::io_impl{
    io_impl(zero_copy_if::sptr data_transport):
        data_transport(data_transport),
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

    //state management for the vrt packet handler code
    sph::recv_packet_handler recv_handler;
    sph::send_packet_handler send_handler;

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
    managed_send_buffer::sptr get_send_buff(double timeout){
        //try to get a new managed buffer with timeout
        offset_send_buffer next_buff(data_transport->get_send_buff(timeout));
        if (not next_buff.buff.get()) return managed_send_buffer::sptr(); /* propagate timeout here */

        //make a new managed buffer with the offset buffs
        return omsb.get_new(curr_buff, next_buff);
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
    managed_send_buffer::sptr buff = this->get_send_buff(.1);
    if (buff.get() != NULL){
        std::memset(buff->cast<void *>(), 0, bytes_to_pad);
        buff->commit(bytes_to_pad);
    }
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

    this->enable_tx(true); //always enabled
    rx_stream_on_off(false);
    _io_impl->flush_send_buff();

    //update mapping here since it didnt b4 when io init not called first
    update_xport_channel_mapping();
}

void usrp1_impl::update_xport_channel_mapping(void){
    if (_io_impl.get() == NULL) return; //not inited yet

    //set all of the relevant properties on the handler
    boost::mutex::scoped_lock recv_lock = _io_impl->recv_handler.get_scoped_lock();
    _io_impl->recv_handler.set_vrt_unpacker(&usrp1_bs_vrt_unpacker);
    _io_impl->recv_handler.set_tick_rate(_clock_ctrl->get_master_clock_freq());
    _io_impl->recv_handler.set_samp_rate(_rx_dsp_proxies[_rx_dsp_proxies.keys().at(0)]->get_link()[DSP_PROP_HOST_RATE].as<double>());
    _io_impl->recv_handler.set_xport_chan_get_buff(0, boost::bind(
        &uhd::transport::zero_copy_if::get_recv_buff, _io_impl->data_transport, _1
    ));
    _io_impl->recv_handler.set_converter(_rx_otw_type, _rx_subdev_spec.size());

    //set all of the relevant properties on the handler
    boost::mutex::scoped_lock send_lock = _io_impl->send_handler.get_scoped_lock();
    _io_impl->send_handler.set_vrt_packer(&usrp1_bs_vrt_packer);
    _io_impl->send_handler.set_tick_rate(_clock_ctrl->get_master_clock_freq());
    _io_impl->send_handler.set_samp_rate(_tx_dsp_proxies[_tx_dsp_proxies.keys().at(0)]->get_link()[DSP_PROP_HOST_RATE].as<double>());
    _io_impl->send_handler.set_xport_chan_get_buff(0, boost::bind(
        &usrp1_impl::io_impl::get_send_buff, _io_impl.get(), _1
    ));
    _io_impl->send_handler.set_converter(_tx_otw_type, _tx_subdev_spec.size());
    _io_impl->send_handler.set_max_samples_per_packet(get_max_send_samps_per_packet());
}

void usrp1_impl::rx_stream_on_off(bool enb){
    this->enable_rx(enb);
    //drain any junk in the receive transport after stop streaming command
    while(not enb and _data_transport->get_recv_buff().get() != NULL){
        /* NOP */
    }
}

/***********************************************************************
 * Data send + helper functions
 **********************************************************************/
size_t usrp1_impl::get_max_send_samps_per_packet(void) const {
    return (_data_transport->get_send_frame_size() - alignment_padding)
        / _tx_otw_type.get_sample_size()
        / _tx_subdev_spec.size()
    ;
}

size_t usrp1_impl::send(
    const send_buffs_type &buffs, size_t nsamps_per_buff,
    const tx_metadata_t &metadata, const io_type_t &io_type,
    send_mode_t send_mode, double timeout
){
    if (_soft_time_ctrl->send_pre(metadata, timeout)) return 0;

    size_t num_samps_sent = _io_impl->send_handler.send(
        buffs, nsamps_per_buff,
        metadata, io_type,
        send_mode, timeout
    );

    //handle eob flag (commit the buffer, disable the DACs)
    //check num samps sent to avoid flush on incomplete/timeout
    if (metadata.end_of_burst and num_samps_sent == nsamps_per_buff){
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
        if (ret < 0)        UHD_MSG(error) << "USRP: underflow check failed" << std::endl;
        else if (underflow) UHD_MSG(fastpath) << "U";
    }

    return num_samps_sent;
}

/***********************************************************************
 * Data recv + helper functions
 **********************************************************************/
size_t usrp1_impl::get_max_recv_samps_per_packet(void) const {
    return _data_transport->get_recv_frame_size()
        / _rx_otw_type.get_sample_size()
        / _rx_subdev_spec.size()
    ;
}

size_t usrp1_impl::recv(
    const recv_buffs_type &buffs, size_t nsamps_per_buff,
    rx_metadata_t &metadata, const io_type_t &io_type,
    recv_mode_t recv_mode, double timeout
){
    size_t num_samps_recvd = _io_impl->recv_handler.recv(
        buffs, nsamps_per_buff,
        metadata, io_type,
        recv_mode, timeout
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
        if (ret < 0)       UHD_MSG(error) << "USRP: overflow check failed" << std::endl;
        else if (overflow) UHD_MSG(fastpath) << "O";
    }

    return num_samps_recvd;
}
