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
#include "usrp2_impl.hpp"
#include "usrp2_regs.hpp"
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/thread_priority.hpp>
#include <uhd/transport/bounded_buffer.hpp>
#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <iostream>
#include <list>

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

    io_impl(size_t send_frame_size, const std::vector<zero_copy_if::sptr> &xports):
        xports(xports),
        packet_handler_recv_state(xports.size()),
        packet_handler_send_state(xports.size()),
        async_msg_fifo(100/*messages deep*/)
    {
        for (size_t i = 0; i < xports.size(); i++){
            fc_mons.push_back(flow_control_monitor::sptr(
                new flow_control_monitor(usrp2_impl::sram_bytes/send_frame_size)
            ));
            //init empty packet infos
            vrt::if_packet_info_t packet_info;
            packet_info.packet_count = 0xf;
            packet_info.has_tsi = true;
            packet_info.tsi = 0;
            packet_info.has_tsf = true;
            packet_info.tsf = 0;
            prev_infos.push_back(packet_info);
        }
    }

    ~io_impl(void){
        recv_pirate_crew_raiding = false;
        recv_pirate_crew.interrupt_all();
        recv_pirate_crew.join_all();
    }

    bool get_send_buffs(
        vrt_packet_handler::managed_send_buffs_t &buffs,
        double timeout
    ){
        UHD_ASSERT_THROW(xports.size() == buffs.size());

        //calculate the flow control word
        const boost::uint32_t fc_word32 = packet_handler_send_state.next_packet_seq;

        //grab a managed buffer for each index
        for (size_t i = 0; i < buffs.size(); i++){
            if (not fc_mons[i]->check_fc_condition(fc_word32, timeout)) return false;
            buffs[i] = xports[i]->get_send_buff(timeout);
            if (not buffs[i].get()) return false;
            buffs[i]->cast<boost::uint32_t *>()[0] = uhd::htonx(fc_word32);
        }
        return true;
    }

    bool get_recv_buffs(
        vrt_packet_handler::managed_recv_buffs_t &buffs,
        double timeout
    );

    const std::vector<zero_copy_if::sptr> &xports;

    //previous state for each buffer
    std::vector<vrt::if_packet_info_t> prev_infos;

    //flow control monitors
    std::vector<flow_control_monitor::sptr> fc_mons;

    //state management for the vrt packet handler code
    vrt_packet_handler::recv_state packet_handler_recv_state;
    vrt_packet_handler::send_state packet_handler_send_state;

    //methods and variables for the pirate crew
    void recv_pirate_loop(zero_copy_if::sptr, usrp2_mboard_impl::sptr, size_t);
    boost::thread_group recv_pirate_crew;
    bool recv_pirate_crew_raiding;
    bounded_buffer<async_metadata_t> async_msg_fifo;
    boost::mutex spawn_mutex;
};

/***********************************************************************
 * Receive Pirate Loop
 * - while raiding, loot for message packet
 * - update flow control condition count
 * - put async message packets into queue
 **********************************************************************/
void usrp2_impl::io_impl::recv_pirate_loop(
    zero_copy_if::sptr zc_if_err0,
    usrp2_mboard_impl::sptr mboard,
    size_t index
){
    set_thread_priority_safe();
    recv_pirate_crew_raiding = true;

    spawn_mutex.unlock();

    while(recv_pirate_crew_raiding){
        managed_recv_buffer::sptr buff = zc_if_err0->get_recv_buff();
        if (not buff.get()) continue; //ignore timeout/error buffers

        try{
            //extract the vrt header packet info
            vrt::if_packet_info_t if_packet_info;
            if_packet_info.num_packet_words32 = buff->size()/sizeof(boost::uint32_t);
            const boost::uint32_t *vrt_hdr = buff->cast<const boost::uint32_t *>();
            vrt::if_hdr_unpack_be(vrt_hdr, if_packet_info);

            //handle a tx async report message
            if (if_packet_info.sid == usrp2_impl::ASYNC_SID and if_packet_info.packet_type != vrt::if_packet_info_t::PACKET_TYPE_DATA){

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
                async_msg_fifo.push_with_pop_on_full(metadata);
            }
            else{
                //TODO unknown received packet, may want to print error...
            }
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
    const size_t send_frame_size = _data_transports.front()->get_send_frame_size();

    //create new io impl
    _io_impl = UHD_PIMPL_MAKE(io_impl, (send_frame_size, _data_transports));

    //create a new pirate thread for each zc if (yarr!!)
    for (size_t i = 0; i < _data_transports.size(); i++){
        //lock the unlocked mutex (non-blocking)
        _io_impl->spawn_mutex.lock();
        //spawn a new pirate to plunder the recv booty
        _io_impl->recv_pirate_crew.create_thread(boost::bind(
            &usrp2_impl::io_impl::recv_pirate_loop,
            _io_impl.get(), _err0_transports.at(i),
            _mboards.at(i), i
        ));
        //block here until the spawned thread unlocks
        _io_impl->spawn_mutex.lock();
        //exit loop iteration in an unlocked condition
        _io_impl->spawn_mutex.unlock();
    }
}

/***********************************************************************
 * Async Data
 **********************************************************************/
bool usrp2_impl::recv_async_msg(
    async_metadata_t &async_metadata, double timeout
){
    boost::this_thread::disable_interruption di; //disable because the wait can throw
    return _io_impl->async_msg_fifo.pop_with_timed_wait(async_metadata, timeout);
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
        boost::bind(&usrp2_impl::io_impl::get_send_buffs, _io_impl.get(), _1, timeout),
        get_max_send_samps_per_packet(),
        vrt_send_header_offset_words32
    );
}

/***********************************************************************
 * Alignment logic on receive
 **********************************************************************/
static UHD_INLINE boost::posix_time::time_duration to_time_dur(double timeout){
    return boost::posix_time::microseconds(long(timeout*1e6));
}

static UHD_INLINE double from_time_dur(const boost::posix_time::time_duration &time_dur){
    return 1e-6*time_dur.total_microseconds();
}

static UHD_INLINE time_spec_t extract_time_spec(
    const vrt::if_packet_info_t &packet_info
){
    return time_spec_t( //assumes has_tsi and has_tsf are true
        time_t(packet_info.tsi), size_t(packet_info.tsf),
        100e6 //tick rate does not have to be correct for comparison purposes
    );
}

static UHD_INLINE void extract_packet_info(
    managed_recv_buffer::sptr &buff,
    vrt::if_packet_info_t &prev_info,
    time_spec_t &time, bool &clear, bool &msg
){
    //extract packet info
    vrt::if_packet_info_t next_info;
    next_info.num_packet_words32 = buff->size()/sizeof(boost::uint32_t);
    vrt::if_hdr_unpack_be(buff->cast<const boost::uint32_t *>(), next_info);

    //handle the packet count / sequence number
    if ((prev_info.packet_count+1)%16 != next_info.packet_count){
        std::cerr << "O" << std::flush; //report overflow (drops in the kernel)
    }

    time = extract_time_spec(next_info);
    clear = extract_time_spec(prev_info) > time;
    msg = next_info.packet_type != vrt::if_packet_info_t::PACKET_TYPE_DATA;
    prev_info = next_info;
}

static UHD_INLINE bool handle_msg_packet(
    vrt_packet_handler::managed_recv_buffs_t &buffs, size_t index
){
    for (size_t i = 0; i < buffs.size(); i++){
        if (i == index) continue;
        buffs[i].reset(); //set NULL
    }
    return true;
}

UHD_INLINE bool usrp2_impl::io_impl::get_recv_buffs(
    vrt_packet_handler::managed_recv_buffs_t &buffs,
    double timeout
){
    if (buffs.size() == 1){
        buffs[0] = xports[0]->get_recv_buff(timeout);
        if (buffs[0].get() == NULL) return false;
        bool clear, msg; time_spec_t time; //unused variables
        //call extract_packet_info to handle printing the overflows
        extract_packet_info(buffs[0], this->prev_infos[0], time, clear, msg);
        return true;
    }
    //-------------------- begin alignment logic ---------------------//
    boost::system_time exit_time = boost::get_system_time() + to_time_dur(timeout);
    managed_recv_buffer::sptr buff_tmp;
    std::list<size_t> _all_indexes, indexes_to_do;
    for (size_t i = 0; i < buffs.size(); i++) _all_indexes.push_back(i);
    bool clear, msg;
    time_spec_t expected_time;

    //respond to a clear by starting from scratch
    got_clear:
    indexes_to_do = _all_indexes;
    clear = false;

    //do an initial pop to load an initial sequence id
    size_t index = indexes_to_do.front();
    buff_tmp = xports[index]->get_recv_buff(from_time_dur(exit_time - boost::get_system_time()));
    if (buff_tmp.get() == NULL) return false;
    extract_packet_info(buff_tmp, this->prev_infos[index], expected_time, clear, msg);
    if (clear) goto got_clear;
    buffs[index] = buff_tmp;
    if (msg) return handle_msg_packet(buffs, index);
    indexes_to_do.pop_front();

    //get an aligned set of elements from the buffers:
    while(indexes_to_do.size() != 0){

        //pop an element off for this index
        index = indexes_to_do.front();
        buff_tmp = xports[index]->get_recv_buff(from_time_dur(exit_time - boost::get_system_time()));
        if (buff_tmp.get() == NULL) return false;
        time_spec_t this_time;
        extract_packet_info(buff_tmp, this->prev_infos[index], this_time, clear, msg);
        if (clear) goto got_clear;
        buffs[index] = buff_tmp;
        if (msg) return handle_msg_packet(buffs, index);

        //if the sequence id matches:
        //  remove this index from the list and continue
        if (this_time == expected_time){
            indexes_to_do.pop_front();
            continue;
        }

        //if the sequence id is older:
        //  continue with the same index to try again
        else if (this_time < expected_time){
            continue;
        }

        //if the sequence id is newer:
        //  use the new expected time for comparison
        //  add all other indexes back into the list
        else{
            expected_time = this_time;
            indexes_to_do = _all_indexes;
            indexes_to_do.remove(index);
            continue;
        }
    }
    return true;
    //-------------------- end alignment logic -----------------------//
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
        boost::bind(&handle_overflow, boost::ref(_mboards), _1)
    );
}
