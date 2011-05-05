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
#include <uhd/utils/log.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/exception.hpp>
#include <uhd/usrp/mboard_props.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/thread_priority.hpp>
#include <uhd/transport/bounded_buffer.hpp>
#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/barrier.hpp>
#include <iostream>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;
namespace asio = boost::asio;
namespace pt = boost::posix_time;

/***********************************************************************
 * helpers
 **********************************************************************/
static UHD_INLINE pt::time_duration to_time_dur(double timeout){
    return pt::microseconds(long(timeout*1e6));
}

static UHD_INLINE double from_time_dur(const pt::time_duration &time_dur){
    return 1e-6*time_dur.total_microseconds();
}

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
        _ready_fcn = boost::bind(&flow_control_monitor::ready, this);
    }

    /*!
     * Check the flow control condition.
     * \param seq the sequence to go out
     * \param timeout the timeout in seconds
     * \return false on timeout
     */
    UHD_INLINE bool check_fc_condition(seq_type seq, double timeout){
        boost::unique_lock<boost::mutex> lock(_fc_mutex);
        _last_seq_out = seq;
        if (this->ready()) return true;
        boost::this_thread::disable_interruption di; //disable because the wait can throw
        return _fc_cond.timed_wait(lock, to_time_dur(timeout), _ready_fcn);
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
    boost::function<bool(void)> _ready_fcn;
};

/***********************************************************************
 * Alignment indexes class: keeps track of indexes
 **********************************************************************/
class alignment_indexes{
public:
    alignment_indexes(void){_indexes = 0;}
    void reset(size_t len){_indexes = (1 << len) - 1;}
    size_t front(void){ //TODO replace with look-up table
        size_t index = 0;
        while ((_indexes & (1 << index)) == 0) index++;
        return index;
    }
    void remove(size_t index){_indexes &= ~(1 << index);}
    bool empty(void){return _indexes == 0;}
private: size_t _indexes;
};

/***********************************************************************
 * io impl details (internal to this file)
 * - pirate crew
 * - alignment buffer
 * - thread loop
 * - vrt packet handler states
 **********************************************************************/
struct usrp2_impl::io_impl{

    io_impl(std::vector<zero_copy_if::sptr> &dsp_xports):
        dsp_xports(dsp_xports), //the assumption is that all data transports should be identical
        get_recv_buffs_fcn(boost::bind(&usrp2_impl::io_impl::get_recv_buffs, this, _1)),
        get_send_buffs_fcn(boost::bind(&usrp2_impl::io_impl::get_send_buffs, this, _1)),
        async_msg_fifo(100/*messages deep*/)
    {
        for (size_t i = 0; i < dsp_xports.size(); i++){
            fc_mons.push_back(flow_control_monitor::sptr(new flow_control_monitor(
                usrp2_impl::sram_bytes/dsp_xports.front()->get_send_frame_size()
            )));;
        }

        //init empty packet infos
        vrt::if_packet_info_t packet_info = vrt::if_packet_info_t();
        packet_info.packet_count = 0xf;
        packet_info.has_tsi = true;
        packet_info.tsi = 0;
        packet_info.has_tsf = true;
        packet_info.tsf = 0;
        prev_infos.resize(dsp_xports.size(), packet_info);
    }

    ~io_impl(void){
        recv_pirate_crew_raiding = false;
        recv_pirate_crew.interrupt_all();
        recv_pirate_crew.join_all();
    }

    bool get_send_buffs(vrt_packet_handler::managed_send_buffs_t &buffs){
        UHD_ASSERT_THROW(send_map.size() == buffs.size());

        //calculate the flow control word
        const boost::uint32_t fc_word32 = packet_handler_send_state.next_packet_seq;

        //grab a managed buffer for each index
        for (size_t i = 0; i < buffs.size(); i++){
            if (not fc_mons[send_map[i]]->check_fc_condition(fc_word32, send_timeout)) return false;
            buffs[i] = dsp_xports[send_map[i]]->get_send_buff(send_timeout);
            if (not buffs[i].get()) return false;
            buffs[i]->cast<boost::uint32_t *>()[0] = uhd::htonx(fc_word32);
        }
        return true;
    }

    alignment_indexes indexes_to_do; //used in alignment logic
    time_spec_t expected_time; //used in alignment logic
    bool get_recv_buffs(vrt_packet_handler::managed_recv_buffs_t &buffs);

    std::vector<zero_copy_if::sptr> &dsp_xports;

    //mappings from channel index to dsp xport
    std::vector<size_t> send_map, recv_map;

    //timeouts set on calls to recv/send (passed into get buffs methods)
    double recv_timeout, send_timeout;

    //bound callbacks for get buffs (bound once here, not in fast-path)
    vrt_packet_handler::get_recv_buffs_t get_recv_buffs_fcn;
    vrt_packet_handler::get_send_buffs_t get_send_buffs_fcn;

    //previous state for each buffer
    std::vector<vrt::if_packet_info_t> prev_infos;

    //flow control monitors
    std::vector<flow_control_monitor::sptr> fc_mons;

    //state management for the vrt packet handler code
    vrt_packet_handler::recv_state packet_handler_recv_state;
    vrt_packet_handler::send_state packet_handler_send_state;

    //methods and variables for the pirate crew
    void recv_pirate_loop(boost::barrier &, usrp2_mboard_impl::sptr, zero_copy_if::sptr, size_t);
    boost::thread_group recv_pirate_crew;
    bool recv_pirate_crew_raiding;
    bounded_buffer<async_metadata_t> async_msg_fifo;
};

/***********************************************************************
 * Receive Pirate Loop
 * - while raiding, loot for message packet
 * - update flow control condition count
 * - put async message packets into queue
 **********************************************************************/
void usrp2_impl::io_impl::recv_pirate_loop(
    boost::barrier &spawn_barrier,
    usrp2_mboard_impl::sptr mboard,
    zero_copy_if::sptr err_xport,
    size_t index
){
    recv_pirate_crew_raiding = true;
    spawn_barrier.wait();
    set_thread_priority_safe();

    //store a reference to the flow control monitor (offset by max dsps)
    flow_control_monitor &fc_mon = *(this->fc_mons[index*usrp2_mboard_impl::MAX_NUM_DSPS]);

    while(recv_pirate_crew_raiding){
        managed_recv_buffer::sptr buff = err_xport->get_recv_buff();
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
                    fc_mon.update_fc_condition(uhd::ntohx(fc_word32));
                    continue;
                }

                //print the famous U, and push the metadata into the message queue
                if (metadata.event_code & underflow_flags) UHD_MSG(fastpath) << "U";
                //else UHD_MSG(often) << "metadata.event_code " << metadata.event_code << std::endl;
                async_msg_fifo.push_with_pop_on_full(metadata);
            }
            else{
                //TODO unknown received packet, may want to print error...
            }
        }catch(const std::exception &e){
            UHD_MSG(error) << "Error (usrp2 recv pirate loop): " << e.what() << std::endl;
        }
    }
}

/***********************************************************************
 * Helper Functions
 **********************************************************************/
void usrp2_impl::io_init(void){

    //create new io impl
    _io_impl = UHD_PIMPL_MAKE(io_impl, (dsp_xports));

    //create a new pirate thread for each zc if (yarr!!)
    boost::barrier spawn_barrier(_mboards.size()+1);
    for (size_t i = 0; i < _mboards.size(); i++){
        //spawn a new pirate to plunder the recv booty
        _io_impl->recv_pirate_crew.create_thread(boost::bind(
            &usrp2_impl::io_impl::recv_pirate_loop,
            _io_impl.get(), boost::ref(spawn_barrier),
            _mboards.at(i), err_xports.at(i), i
        ));
    }
    spawn_barrier.wait();

    //update mapping here since it didnt b4 when io init not called first
    update_xport_channel_mapping();
}

void usrp2_impl::update_xport_channel_mapping(void){
    if (_io_impl.get() == NULL) return; //not inited yet

    _io_impl->recv_map.clear();
    _io_impl->send_map.clear();

    for (size_t i = 0; i < _mboards.size(); i++){

        subdev_spec_t rx_subdev_spec = _mboards[i]->get_link()[MBOARD_PROP_RX_SUBDEV_SPEC].as<subdev_spec_t>();
        for (size_t j = 0; j < rx_subdev_spec.size(); j++){
            _io_impl->recv_map.push_back(i*usrp2_mboard_impl::MAX_NUM_DSPS+j);
            UHD_LOG << "recv_map.back() " << _io_impl->recv_map.back() << std::endl;
        }

        subdev_spec_t tx_subdev_spec = _mboards[i]->get_link()[MBOARD_PROP_TX_SUBDEV_SPEC].as<subdev_spec_t>();
        for (size_t j = 0; j < tx_subdev_spec.size(); j++){
            _io_impl->send_map.push_back(i*usrp2_mboard_impl::MAX_NUM_DSPS+j);
            UHD_LOG << "send_map.back() " << _io_impl->send_map.back() << std::endl;
        }

    }

    _io_impl->packet_handler_recv_state = vrt_packet_handler::recv_state(_io_impl->recv_map.size());
    _io_impl->packet_handler_send_state = vrt_packet_handler::send_state(_io_impl->send_map.size());
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
    const size_t bpp = dsp_xports.front()->get_send_frame_size() - hdr_size;
    return bpp/_tx_otw_type.get_sample_size();
}

size_t usrp2_impl::send(
    const send_buffs_type &buffs, size_t num_samps,
    const tx_metadata_t &metadata, const io_type_t &io_type,
    send_mode_t send_mode, double timeout
){
    _io_impl->send_timeout = timeout;
    return vrt_packet_handler::send(
        _io_impl->packet_handler_send_state,       //last state of the send handler
        buffs, num_samps,                          //buffer to fill
        metadata, send_mode,                       //samples metadata
        io_type, _tx_otw_type,                     //input and output types to convert
        _mboards.front()->get_master_clock_freq(), //master clock tick rate
        uhd::transport::vrt::if_hdr_pack_be,
        _io_impl->get_send_buffs_fcn,
        get_max_send_samps_per_packet(),
        vrt_send_header_offset_words32
    );
}

/***********************************************************************
 * Alignment logic on receive
 **********************************************************************/
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
        UHD_MSG(fastpath) << "O"; //report overflow (drops in the kernel)
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
    vrt_packet_handler::managed_recv_buffs_t &buffs
){
    if (buffs.size() == 1){
        buffs[0] = dsp_xports[recv_map[0]]->get_recv_buff(recv_timeout);
        if (buffs[0].get() == NULL) return false;
        bool clear, msg; time_spec_t time; //unused variables
        //call extract_packet_info to handle printing the overflows
        extract_packet_info(buffs[0], this->prev_infos[recv_map[0]], time, clear, msg);
        return true;
    }
    //-------------------- begin alignment logic ---------------------//
    UHD_ASSERT_THROW(recv_map.size() == buffs.size());
    boost::system_time exit_time = boost::get_system_time() + to_time_dur(recv_timeout);
    managed_recv_buffer::sptr buff_tmp;
    bool clear, msg;
    size_t index;

    //If we did not enter this routine with an empty indexes set,
    //jump to after the clear so we can preserve the previous state.
    //This saves buffers from being lost when using non-blocking recv.
    if (not indexes_to_do.empty()) goto skip_pop_initial;

    //respond to a clear by starting from scratch
    got_clear:
    indexes_to_do.reset(buffs.size());
    clear = false;

    //do an initial pop to load an initial sequence id
    index = indexes_to_do.front();
    buff_tmp = dsp_xports[recv_map[index]]->get_recv_buff(from_time_dur(exit_time - boost::get_system_time()));
    if (buff_tmp.get() == NULL) return false;
    extract_packet_info(buff_tmp, this->prev_infos[recv_map[index]], expected_time, clear, msg);
    if (clear) goto got_clear;
    buffs[index] = buff_tmp;
    if (msg) return handle_msg_packet(buffs, index);
    indexes_to_do.remove(index);
    skip_pop_initial:

    //get an aligned set of elements from the buffers:
    while(not indexes_to_do.empty()){

        //pop an element off for this index
        index = indexes_to_do.front();
        buff_tmp = dsp_xports[recv_map[index]]->get_recv_buff(from_time_dur(exit_time - boost::get_system_time()));
        if (buff_tmp.get() == NULL) return false;
        time_spec_t this_time;
        extract_packet_info(buff_tmp, this->prev_infos[recv_map[index]], this_time, clear, msg);
        if (clear) goto got_clear;
        buffs[index] = buff_tmp;
        if (msg) return handle_msg_packet(buffs, index);

        //if the sequence id matches:
        //  remove this index from the list and continue
        if (this_time == expected_time){
            indexes_to_do.remove(index);
        }

        //if the sequence id is newer:
        //  use the new expected time for comparison
        //  add all other indexes back into the list
        else if (this_time > expected_time){
            expected_time = this_time;
            indexes_to_do.reset(buffs.size());
            indexes_to_do.remove(index);
        }

        //if the sequence id is older:
        //  continue with the same index to try again
        //else if (this_time < expected_time)...

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
    const size_t bpp = dsp_xports.front()->get_recv_frame_size() - hdr_size;
    return bpp/_rx_otw_type.get_sample_size();
}

void usrp2_impl::handle_overflow(size_t chan){
    UHD_MSG(fastpath) << "O";
    ldiv_t indexes = ldiv(chan, usrp2_mboard_impl::NUM_RX_DSPS);
    _mboards.at(indexes.quot)->handle_overflow(indexes.rem);
}

size_t usrp2_impl::recv(
    const recv_buffs_type &buffs, size_t num_samps,
    rx_metadata_t &metadata, const io_type_t &io_type,
    recv_mode_t recv_mode, double timeout
){
    _io_impl->recv_timeout = timeout;
    return vrt_packet_handler::recv(
        _io_impl->packet_handler_recv_state,       //last state of the recv handler
        buffs, num_samps,                          //buffer to fill
        metadata, recv_mode,                       //samples metadata
        io_type, _rx_otw_type,                     //input and output types to convert
        _mboards.front()->get_master_clock_freq(), //master clock tick rate
        uhd::transport::vrt::if_hdr_unpack_be,
        _io_impl->get_recv_buffs_fcn,
        boost::bind(&usrp2_impl::handle_overflow, this, _1)
    );
}
