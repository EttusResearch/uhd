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
#include "usrp2_impl.hpp"
#include "usrp2_regs.hpp"
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/thread_priority.hpp>
#include <uhd/transport/convert_types.hpp>
#include <uhd/transport/alignment_buffer.hpp>
#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <iostream>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;
namespace asio = boost::asio;

/***********************************************************************
 * constants
 **********************************************************************/
static const int underflow_flags = 0
    | async_metadata_t::EVENT_CODE_UNDERFLOW
    | async_metadata_t::EVENT_CODE_UNDERFLOW_IN_PACKET
;

static const size_t vrt_send_header_offset_words32 = 1;

/***********************************************************************
 * flow control monitor for a single tx channel
 *  - the pirate thread calls update
 *  - the get send buffer calls check
 **********************************************************************/
class flow_control_monitor{
public:
    typedef boost::uint32_t seq_type;
    typedef boost::shared_ptr<flow_control_monitor> sptr;

    /*!
     * Make a new flow control monitor.
     * \param max_seqs_out num seqs before throttling
     */
    flow_control_monitor(seq_type max_seqs_out){
        _last_seq_out = 0;
        _last_seq_ack = 0;
        _max_seqs_out = max_seqs_out;
    }

    /*!
     * Check the flow control condition.
     * \param seq the sequence to go out
     * \param timeout the timeout in seconds
     * \return false on timeout
     */
    UHD_INLINE bool check_fc_condition(seq_type seq, double timeout){
        boost::this_thread::disable_interruption di; //disable because the wait can throw
        boost::unique_lock<boost::mutex> lock(_fc_mutex);
        _last_seq_out = seq;
        return _fc_cond.timed_wait(
            lock,
            boost::posix_time::microseconds(long(timeout*1e6)),
            boost::bind(&flow_control_monitor::ready, this)
        );
    }

    /*!
     * Update the flow control condition.
     * \param seq the last sequence number to be ACK'd
     */
    UHD_INLINE void update_fc_condition(seq_type seq){
        boost::unique_lock<boost::mutex> lock(_fc_mutex);
        _last_seq_ack = seq;
        lock.unlock();
        _fc_cond.notify_one();
    }

private:
    bool ready(void){
        return seq_type(_last_seq_out -_last_seq_ack) < _max_seqs_out;
    }

    boost::mutex _fc_mutex;
    boost::condition _fc_cond;
    seq_type _last_seq_out, _last_seq_ack, _max_seqs_out;
};

/***********************************************************************
 * io impl details (internal to this file)
 * - pirate crew
 * - alignment buffer
 * - thread loop
 * - vrt packet handler states
 **********************************************************************/
struct usrp2_impl::io_impl{
    typedef alignment_buffer<managed_recv_buffer::sptr, time_spec_t> alignment_buffer_type;

    io_impl(size_t num_recv_frames, size_t send_frame_size, size_t width):
        packet_handler_recv_state(width),
        recv_pirate_booty(alignment_buffer_type::make(num_recv_frames-3, width)),
        async_msg_fifo(bounded_buffer<async_metadata_t>::make(100/*messages deep*/))
    {
        for (size_t i = 0; i < width; i++) fc_mons.push_back(
            flow_control_monitor::sptr(new flow_control_monitor(usrp2_impl::sram_bytes/send_frame_size))
        );
    }

    ~io_impl(void){
        recv_pirate_crew_raiding = false;
        recv_pirate_crew.interrupt_all();
        recv_pirate_crew.join_all();
    }

    bool get_recv_buffs(vrt_packet_handler::managed_recv_buffs_t &buffs, double timeout){
        boost::this_thread::disable_interruption di; //disable because the wait can throw
        return recv_pirate_booty->pop_elems_with_timed_wait(buffs, timeout);
    }

    bool get_send_buffs(
        const std::vector<zero_copy_if::sptr> &trans,
        vrt_packet_handler::managed_send_buffs_t &buffs,
        double timeout
    ){
        UHD_ASSERT_THROW(trans.size() == buffs.size());

        //calculate the flow control word
        const boost::uint32_t fc_word32 = packet_handler_send_state.next_packet_seq;

        //grab a managed buffer for each index
        for (size_t i = 0; i < buffs.size(); i++){
            if (not fc_mons[i]->check_fc_condition(fc_word32, timeout)) return false;
            buffs[i] = trans[i]->get_send_buff(timeout);
            if (not buffs[i].get()) return false;
            buffs[i]->cast<boost::uint32_t *>()[0] = uhd::htonx(fc_word32);
        }
        return true;
    }

    //flow control monitors
    std::vector<flow_control_monitor::sptr> fc_mons;

    //state management for the vrt packet handler code
    vrt_packet_handler::recv_state packet_handler_recv_state;
    vrt_packet_handler::send_state packet_handler_send_state;

    //methods and variables for the pirate crew
    void recv_pirate_loop(zero_copy_if::sptr, usrp2_mboard_impl::sptr, size_t);
    boost::thread_group recv_pirate_crew;
    bool recv_pirate_crew_raiding;
    alignment_buffer_type::sptr recv_pirate_booty;
    bounded_buffer<async_metadata_t>::sptr async_msg_fifo;
};

/***********************************************************************
 * Receive Pirate Loop
 * - while raiding, loot for recv buffers
 * - put booty into the alignment buffer
 **********************************************************************/
void usrp2_impl::io_impl::recv_pirate_loop(
    zero_copy_if::sptr zc_if,
    usrp2_mboard_impl::sptr mboard,
    size_t index
){
    set_thread_priority_safe();
    recv_pirate_crew_raiding = true;
    size_t next_packet_seq = 0;

    while(recv_pirate_crew_raiding){
        managed_recv_buffer::sptr buff = zc_if->get_recv_buff();
        if (not buff.get()) continue; //ignore timeout/error buffers

        try{
            //extract the vrt header packet info
            vrt::if_packet_info_t if_packet_info;
            if_packet_info.num_packet_words32 = buff->size()/sizeof(boost::uint32_t);
            const boost::uint32_t *vrt_hdr = buff->cast<const boost::uint32_t *>();
            vrt::if_hdr_unpack_be(vrt_hdr, if_packet_info);

            //handle a tx async report message
            if (if_packet_info.sid == 1 and if_packet_info.packet_type != vrt::if_packet_info_t::PACKET_TYPE_DATA){

                //fill in the async metadata
                async_metadata_t metadata;
                metadata.channel = index;
                metadata.has_time_spec = if_packet_info.has_tsi and if_packet_info.has_tsf;
                metadata.time_spec = time_spec_t(
                    time_t(if_packet_info.tsi), size_t(if_packet_info.tsf), mboard->get_master_clock_freq()
                );
                metadata.event_code = vrt_packet_handler::get_context_code<async_metadata_t::event_code_t>(vrt_hdr, if_packet_info);

                //catch the flow control packets and react
                if (metadata.event_code == 0){
                    boost::uint32_t fc_word32 = (vrt_hdr + if_packet_info.num_header_words32)[1];
                    this->fc_mons[index]->update_fc_condition(uhd::ntohx(fc_word32));
                    continue;
                }

                //print the famous U, and push the metadata into the message queue
                if (metadata.event_code & underflow_flags) std::cerr << "U" << std::flush;
                //else std::cout << "metadata.event_code " << metadata.event_code << std::endl;
                async_msg_fifo->push_with_pop_on_full(metadata);
                continue;
            }

            //handle the packet count / sequence number
            if (if_packet_info.packet_count != next_packet_seq){
                //std::cerr << "S" << (if_packet_info.packet_count - next_packet_seq)%16;
                std::cerr << "O" << std::flush; //report overflow (drops in the kernel)
            }
            next_packet_seq = (if_packet_info.packet_count+1)%16;

            //extract the timespec and round to the nearest packet
            UHD_ASSERT_THROW(if_packet_info.has_tsi and if_packet_info.has_tsf);
            time_spec_t time(
                time_t(if_packet_info.tsi), size_t(if_packet_info.tsf), mboard->get_master_clock_freq()
            );

            //push the packet into the buffer with the new time
            recv_pirate_booty->push_with_pop_on_full(buff, time, index);
        }catch(const std::exception &e){
            std::cerr << "Error (usrp2 recv pirate loop): " << e.what() << std::endl;
        }
    }
}

/***********************************************************************
 * Helper Functions
 **********************************************************************/
void usrp2_impl::io_init(void){

    //the assumption is that all data transports should be identical
    const size_t num_recv_frames = _data_transports.front()->get_num_recv_frames();
    const size_t send_frame_size = _data_transports.front()->get_send_frame_size();

    //create new io impl
    _io_impl = UHD_PIMPL_MAKE(io_impl, (num_recv_frames, send_frame_size, _data_transports.size()));

    //create a new pirate thread for each zc if (yarr!!)
    for (size_t i = 0; i < _data_transports.size(); i++){
        _io_impl->recv_pirate_crew.create_thread(boost::bind(
            &usrp2_impl::io_impl::recv_pirate_loop,
            _io_impl.get(), _data_transports.at(i),
            _mboards.at(i), i
        ));
    }
}

/***********************************************************************
 * Async Data
 **********************************************************************/
bool usrp2_impl::recv_async_msg(
    async_metadata_t &async_metadata, double timeout
){
    boost::this_thread::disable_interruption di; //disable because the wait can throw
    return _io_impl->async_msg_fifo->pop_with_timed_wait(async_metadata, timeout);
}

/***********************************************************************
 * Send Data
 **********************************************************************/
size_t usrp2_impl::get_max_send_samps_per_packet(void) const{
    static const size_t hdr_size = 0
        + vrt::max_if_hdr_words32*sizeof(boost::uint32_t)
        + vrt_send_header_offset_words32*sizeof(boost::uint32_t)
        - sizeof(vrt::if_packet_info_t().cid) //no class id ever used
    ;
    const size_t bpp = _data_transports.front()->get_send_frame_size() - hdr_size;
    return bpp/_tx_otw_type.get_sample_size();
}

size_t usrp2_impl::send(
    const std::vector<const void *> &buffs, size_t num_samps,
    const tx_metadata_t &metadata, const io_type_t &io_type,
    send_mode_t send_mode, double timeout
){
    return vrt_packet_handler::send(
        _io_impl->packet_handler_send_state,       //last state of the send handler
        buffs, num_samps,                          //buffer to fill
        metadata, send_mode,                       //samples metadata
        io_type, _tx_otw_type,                     //input and output types to convert
        _mboards.front()->get_master_clock_freq(), //master clock tick rate
        uhd::transport::vrt::if_hdr_pack_be,
        boost::bind(&usrp2_impl::io_impl::get_send_buffs, _io_impl.get(), _data_transports, _1, timeout),
        get_max_send_samps_per_packet(),
        vrt_send_header_offset_words32
    );
}

/***********************************************************************
 * Receive Data
 **********************************************************************/
size_t usrp2_impl::get_max_recv_samps_per_packet(void) const{
    static const size_t hdr_size = 0
        + vrt::max_if_hdr_words32*sizeof(boost::uint32_t)
        + sizeof(vrt::if_packet_info_t().tlr) //forced to have trailer
        - sizeof(vrt::if_packet_info_t().cid) //no class id ever used
    ;
    const size_t bpp = _data_transports.front()->get_recv_frame_size() - hdr_size;
    return bpp/_rx_otw_type.get_sample_size();
}

static void handle_overflow(std::vector<usrp2_mboard_impl::sptr> &mboards, size_t chan){
    std::cerr << "O" << std::flush;
    mboards.at(chan/mboards.size())->handle_overflow();
}

size_t usrp2_impl::recv(
    const std::vector<void *> &buffs, size_t num_samps,
    rx_metadata_t &metadata, const io_type_t &io_type,
    recv_mode_t recv_mode, double timeout
){
    return vrt_packet_handler::recv(
        _io_impl->packet_handler_recv_state,       //last state of the recv handler
        buffs, num_samps,                          //buffer to fill
        metadata, recv_mode,                       //samples metadata
        io_type, _rx_otw_type,                     //input and output types to convert
        _mboards.front()->get_master_clock_freq(), //master clock tick rate
        uhd::transport::vrt::if_hdr_unpack_be,
        boost::bind(&usrp2_impl::io_impl::get_recv_buffs, _io_impl.get(), _1, timeout),
        boost::bind(&handle_overflow, _mboards, _1)
    );
}
