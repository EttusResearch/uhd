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

#include "validate_subdev_spec.hpp"
#define SRPH_DONT_CHECK_SEQUENCE
#include "../../transport/super_recv_packet_handler.hpp"
#include "../../transport/super_send_packet_handler.hpp"
#include "usrp1_calc_mux.hpp"
#include "fpga_regs_standard.h"
#include "usrp_commands.h"
#include "usrp1_impl.hpp"
#include <uhd/utils/msg.hpp>
#include <uhd/utils/safe_call.hpp>
#include <uhd/transport/bounded_buffer.hpp>
#include <boost/math/special_functions/sign.hpp>
#include <boost/math/special_functions/round.hpp>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;

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
        curr_buff(offset_send_buffer(data_transport->get_send_buff())),
        omsb(boost::bind(&usrp1_impl::io_impl::commit_send_buff, this, _1, _2, _3))
    {
        /* NOP */
    }

    ~io_impl(void){
        vandal_tribe.interrupt_all();
        vandal_tribe.join_all();
        UHD_SAFE_CALL(flush_send_buff();)
    }

    zero_copy_if::sptr data_transport;

    //state management for the vrt packet handler code
    sph::recv_packet_handler recv_handler;
    sph::send_packet_handler send_handler;

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

    boost::thread_group vandal_tribe;
    boost::system_time last_send_time;
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

    //create a new vandal thread to poll xerflow conditions
    boost::barrier spawn_barrier(2);
    _io_impl->vandal_tribe.create_thread(boost::bind(
        &usrp1_impl::vandal_conquest_loop,
        this, boost::ref(spawn_barrier)
    ));
    spawn_barrier.wait();

    //init some handler stuff
    _io_impl->recv_handler.set_tick_rate(_master_clock_rate);
    _io_impl->recv_handler.set_vrt_unpacker(&usrp1_bs_vrt_unpacker);
    _io_impl->recv_handler.set_xport_chan_get_buff(0, boost::bind(
        &uhd::transport::zero_copy_if::get_recv_buff, _io_impl->data_transport, _1
    ));
    _io_impl->send_handler.set_tick_rate(_master_clock_rate);
    _io_impl->send_handler.set_vrt_packer(&usrp1_bs_vrt_packer);
    _io_impl->send_handler.set_xport_chan_get_buff(0, boost::bind(
        &usrp1_impl::io_impl::get_send_buff, _io_impl.get(), _1
    ));

    //init as disabled, then call the real function (uses restore)
    this->enable_rx(false);
    this->enable_tx(false);
    rx_stream_on_off(false);
    tx_stream_on_off(false);
    _io_impl->flush_send_buff();
}

void usrp1_impl::rx_stream_on_off(bool enb){
    this->restore_rx(enb);
    //drain any junk in the receive transport after stop streaming command
    while(not enb and _data_transport->get_recv_buff().get() != NULL){
        /* NOP */
    }
}

void usrp1_impl::tx_stream_on_off(bool enb){
    _io_impl->last_send_time = boost::get_system_time();
    if (_tx_enabled and not enb) _io_impl->flush_send_buff();
    this->restore_tx(enb);
}

/*!
 * Casually poll the overflow and underflow registers.
 * On an underflow, push an async message into the queue and print.
 * On an overflow, interleave an inline message into recv and print.
 * This procedure creates "soft" inline and async user messages.
 */
void usrp1_impl::vandal_conquest_loop(boost::barrier &spawn_barrier){
    spawn_barrier.wait();

    //initialize the async metadata
    async_metadata_t async_metadata;
    async_metadata.channel = 0;
    async_metadata.has_time_spec = true;
    async_metadata.event_code = async_metadata_t::EVENT_CODE_UNDERFLOW;

    //initialize the inline metadata
    rx_metadata_t inline_metadata;
    inline_metadata.has_time_spec = true;
    inline_metadata.error_code = rx_metadata_t::ERROR_CODE_OVERFLOW;

    //start the polling loop...
    try{ while (not boost::this_thread::interruption_requested()){
        boost::uint8_t underflow = 0, overflow = 0;

        //shutoff transmit if it has been too long since send() was called
        if (_tx_enabled and (boost::get_system_time() - _io_impl->last_send_time) > boost::posix_time::milliseconds(100)){
            this->tx_stream_on_off(false);
        }

        //always poll regardless of enabled so we can clear the conditions
        _fx2_ctrl->usrp_control_read(
            VRQ_GET_STATUS, 0, GS_TX_UNDERRUN, &underflow, sizeof(underflow)
        );
        _fx2_ctrl->usrp_control_read(
            VRQ_GET_STATUS, 0, GS_RX_OVERRUN, &overflow, sizeof(overflow)
        );

        //handle message generation for xerflow conditions
        if (_tx_enabled and underflow){
            async_metadata.time_spec = _soft_time_ctrl->get_time();
            _soft_time_ctrl->get_async_queue().push_with_pop_on_full(async_metadata);
            UHD_MSG(fastpath) << "U";
        }
        if (_rx_enabled and overflow){
            inline_metadata.time_spec = _soft_time_ctrl->get_time();
            _soft_time_ctrl->get_inline_queue().push_with_pop_on_full(inline_metadata);
            UHD_MSG(fastpath) << "O";
        }

        boost::this_thread::sleep(boost::posix_time::milliseconds(50));
    }}
    catch(const boost::thread_interrupted &){} //normal exit condition
    catch(const std::exception &e){
        UHD_MSG(error) << "The vandal caught an unexpected exception " << e.what() << std::endl;
    }
}

/***********************************************************************
 * Properties callback methods below
 **********************************************************************/
void usrp1_impl::update_rx_subdev_spec(const uhd::usrp::subdev_spec_t &spec){
    boost::mutex::scoped_lock lock = _io_impl->recv_handler.get_scoped_lock();

    //sanity checking
    validate_subdev_spec(_tree, spec, "rx");

    _rx_subdev_spec = spec; //shadow
    _io_impl->recv_handler.resize(spec.size());
    _io_impl->recv_handler.set_converter(_rx_otw_type, spec.size());

    //set the mux and set the number of rx channels
    std::vector<mapping_pair_t> mapping;
    BOOST_FOREACH(const subdev_spec_pair_t &pair, spec){
        const std::string conn = _tree->access<std::string>(str(boost::format(
            "/mboards/0/dboards/%s/rx_frontends/%s/connection"
        ) % pair.db_name % pair.sd_name)).get();
        mapping.push_back(std::make_pair(pair.db_name, conn));
    }
    bool s = this->disable_rx();
    _iface->poke32(FR_RX_MUX, calc_rx_mux(mapping));
    this->restore_rx(s);
}

void usrp1_impl::update_tx_subdev_spec(const uhd::usrp::subdev_spec_t &spec){
    boost::mutex::scoped_lock lock = _io_impl->send_handler.get_scoped_lock();

    //sanity checking
    validate_subdev_spec(_tree, spec, "tx");

    _tx_subdev_spec = spec; //shadow
    _io_impl->send_handler.resize(spec.size());
    _io_impl->send_handler.set_converter(_tx_otw_type, spec.size());

    //set the mux and set the number of tx channels
    std::vector<mapping_pair_t> mapping;
    BOOST_FOREACH(const subdev_spec_pair_t &pair, spec){
        const std::string conn = _tree->access<std::string>(str(boost::format(
            "/mboards/0/dboards/%s/tx_frontends/%s/connection"
        ) % pair.db_name % pair.sd_name)).get();
        mapping.push_back(std::make_pair(pair.db_name, conn));
    }
    bool s = this->disable_tx();
    _iface->poke32(FR_TX_MUX, calc_tx_mux(mapping));
    this->restore_tx(s);

    //if the spec changes size, so does the max samples per packet...
    _io_impl->send_handler.set_max_samples_per_packet(get_max_send_samps_per_packet());
}

double usrp1_impl::update_rx_samp_rate(const double samp_rate){
    boost::mutex::scoped_lock lock = _io_impl->recv_handler.get_scoped_lock();

    const size_t rate = uhd::clip<size_t>(
        boost::math::iround(_master_clock_rate / samp_rate), size_t(std::ceil(_master_clock_rate / 8e6)), 256
    );

    bool s = this->disable_rx();
    _iface->poke32(FR_DECIM_RATE, rate/2 - 1);
    this->restore_rx(s);

    _io_impl->recv_handler.set_samp_rate(_master_clock_rate / rate);
    return _master_clock_rate / rate;
}

double usrp1_impl::update_tx_samp_rate(const double samp_rate){
    boost::mutex::scoped_lock lock = _io_impl->send_handler.get_scoped_lock();

    const size_t rate = uhd::clip<size_t>(
        boost::math::iround(_master_clock_rate / samp_rate), size_t(std::ceil(_master_clock_rate / 8e6)), 256
    );

    bool s = this->disable_tx();
    _iface->poke32(FR_INTERP_RATE, rate/2 - 1);
    this->restore_tx(s);

    _io_impl->send_handler.set_samp_rate(_master_clock_rate / rate);
    return _master_clock_rate / rate;
}

double usrp1_impl::update_rx_dsp_freq(const size_t dspno, const double freq_){

    //correct for outside of rate (wrap around)
    double freq = std::fmod(freq_, _master_clock_rate);
    if (std::abs(freq) > _master_clock_rate/2.0)
        freq -= boost::math::sign(freq)*_master_clock_rate;

    //calculate the freq register word (signed)
    UHD_ASSERT_THROW(std::abs(freq) <= _master_clock_rate/2.0);
    static const double scale_factor = std::pow(2.0, 32);
    const boost::int32_t freq_word = boost::int32_t(boost::math::round((freq / _master_clock_rate) * scale_factor));

    static const boost::uint32_t dsp_index_to_reg_val[4] = {
        FR_RX_FREQ_0, FR_RX_FREQ_1, FR_RX_FREQ_2, FR_RX_FREQ_3
    };
    _iface->poke32(dsp_index_to_reg_val[dspno], ~freq_word + 1);

    return (double(freq_word) / scale_factor) * _master_clock_rate;
}

double usrp1_impl::update_tx_dsp_freq(const size_t dspno, const double freq){
    //map the freq shift key to a subdev spec to a particular codec chip
    _dbc[_tx_subdev_spec.at(dspno).db_name].codec->set_duc_freq(freq, _master_clock_rate);
    return freq; //assume infinite precision
}

/***********************************************************************
 * Async Data
 **********************************************************************/
bool usrp1_impl::recv_async_msg(
    async_metadata_t &async_metadata, double timeout
){
    boost::this_thread::disable_interruption di; //disable because the wait can throw
    return _soft_time_ctrl->get_async_queue().pop_with_timed_wait(async_metadata, timeout);
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

    this->tx_stream_on_off(true); //always enable (it will do the right thing)
    size_t num_samps_sent = _io_impl->send_handler.send(
        buffs, nsamps_per_buff,
        metadata, io_type,
        send_mode, timeout
    );

    //handle eob flag (commit the buffer, /*disable the DACs*/)
    //check num samps sent to avoid flush on incomplete/timeout
    if (metadata.end_of_burst and num_samps_sent == nsamps_per_buff){
        async_metadata_t metadata;
        metadata.channel = 0;
        metadata.has_time_spec = true;
        metadata.time_spec = _soft_time_ctrl->get_time();
        metadata.event_code = async_metadata_t::EVENT_CODE_BURST_ACK;
        _soft_time_ctrl->get_async_queue().push_with_pop_on_full(metadata);
        this->tx_stream_on_off(false);
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
    //interleave a "soft" inline message into the receive stream:
    if (_soft_time_ctrl->get_inline_queue().pop_with_haste(metadata)) return 0;

    size_t num_samps_recvd = _io_impl->recv_handler.recv(
        buffs, nsamps_per_buff,
        metadata, io_type,
        recv_mode, timeout
    );

    return _soft_time_ctrl->recv_post(metadata, num_samps_recvd);
}
