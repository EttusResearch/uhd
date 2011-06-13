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

#include "../../transport/super_recv_packet_handler.hpp"
#include "../../transport/super_send_packet_handler.hpp"
#include "usrp2_impl.hpp"
#include "usrp2_regs.hpp"
#include <uhd/utils/log.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/exception.hpp>
#include <uhd/usrp/mboard_props.hpp>
#include <uhd/usrp/dsp_props.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/thread_priority.hpp>
#include <uhd/transport/bounded_buffer.hpp>
#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <boost/thread/mutex.hpp>
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
     * Gets the current sequence number to go out.
     * Increments the sequence for the next call
     * \return the sequence to be sent to the dsp
     */
    UHD_INLINE seq_type get_curr_seq_out(void){
        return _last_seq_out++;
    }

    /*!
     * Check the flow control condition.
     * \param timeout the timeout in seconds
     * \return false on timeout
     */
    UHD_INLINE bool check_fc_condition(double timeout){
        boost::mutex::scoped_lock lock(_fc_mutex);
        if (this->ready()) return true;
        boost::this_thread::disable_interruption di; //disable because the wait can throw
        return _fc_cond.timed_wait(lock, to_time_dur(timeout), _ready_fcn);
    }

    /*!
     * Update the flow control condition.
     * \param seq the last sequence number to be ACK'd
     */
    UHD_INLINE void update_fc_condition(seq_type seq){
        boost::mutex::scoped_lock lock(_fc_mutex);
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
 * io impl details (internal to this file)
 * - pirate crew
 * - alignment buffer
 * - thread loop
 * - vrt packet handler states
 **********************************************************************/
struct usrp2_impl::io_impl{

    io_impl(std::vector<zero_copy_if::sptr> &dsp_xports):
        dsp_xports(dsp_xports), //the assumption is that all data transports should be identical
        async_msg_fifo(100/*messages deep*/)
    {
        for (size_t i = 0; i < dsp_xports.size(); i++){
            fc_mons.push_back(flow_control_monitor::sptr(new flow_control_monitor(
                usrp2_impl::sram_bytes/dsp_xports.front()->get_send_frame_size()
            )));
        }
    }

    ~io_impl(void){
        recv_pirate_crew_raiding = false;
        recv_pirate_crew.interrupt_all();
        recv_pirate_crew.join_all();
    }

    managed_send_buffer::sptr get_send_buff(size_t chan, double timeout){
        const size_t index = send_map[chan];
        flow_control_monitor &fc_mon = *fc_mons[index];

        //wait on flow control w/ timeout
        if (not fc_mon.check_fc_condition(timeout)) return managed_send_buffer::sptr();

        //get a buffer from the transport w/ timeout
        managed_send_buffer::sptr buff = dsp_xports[index]->get_send_buff(timeout);

        //write the flow control word into the buffer
        if (buff.get()) buff->cast<boost::uint32_t *>()[0] = uhd::htonx(fc_mon.get_curr_seq_out());

        return buff;
    }

    std::vector<zero_copy_if::sptr> &dsp_xports;

    //mappings from channel index to dsp xport
    std::vector<size_t> send_map, recv_map;

    //previous state for each buffer
    std::vector<vrt::if_packet_info_t> prev_infos;

    //flow control monitors
    std::vector<flow_control_monitor::sptr> fc_mons;

    //state management for the vrt packet handler code
    sph::recv_packet_handler recv_handler;
    sph::send_packet_handler send_handler;

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
                metadata.event_code = async_metadata_t::event_code_t(sph::get_context_code(vrt_hdr, if_packet_info));

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

    //set all of the relevant properties on the handler
    boost::mutex::scoped_lock recv_lock = _io_impl->recv_handler.get_scoped_lock();
    _io_impl->recv_handler.resize(_io_impl->recv_map.size());
    _io_impl->recv_handler.set_vrt_unpacker(&vrt::if_hdr_unpack_be);
    _io_impl->recv_handler.set_tick_rate(_mboards.front()->get_master_clock_freq());
    //TODO temporarily use the first dsp rate until we support non-homo rates
    const std::string rx_dsp_name = _mboards.at(0)->get_link()[MBOARD_PROP_RX_DSP_NAMES].as<prop_names_t>().at(0);
    const double rx_host_rate = _mboards.at(0)->get_link()[named_prop_t(MBOARD_PROP_RX_DSP, rx_dsp_name)][DSP_PROP_HOST_RATE].as<double>();
    _io_impl->recv_handler.set_samp_rate(rx_host_rate);
    for (size_t chan = 0; chan < _io_impl->recv_handler.size(); chan++){
        _io_impl->recv_handler.set_xport_chan_get_buff(chan, boost::bind(
            &uhd::transport::zero_copy_if::get_recv_buff,
            _io_impl->dsp_xports[_io_impl->recv_map[chan]], _1
        ));
    }
    _io_impl->recv_handler.set_converter(_rx_otw_type);

    //set all of the relevant properties on the handler
    boost::mutex::scoped_lock send_lock = _io_impl->send_handler.get_scoped_lock();
    _io_impl->send_handler.resize(_io_impl->send_map.size());
    _io_impl->send_handler.set_vrt_packer(&vrt::if_hdr_pack_be, vrt_send_header_offset_words32);
    _io_impl->send_handler.set_tick_rate(_mboards.front()->get_master_clock_freq());
    //TODO temporarily use the first dsp rate until we support non-homo rates
    const std::string tx_dsp_name = _mboards.at(0)->get_link()[MBOARD_PROP_TX_DSP_NAMES].as<prop_names_t>().at(0);
    const double tx_host_rate = _mboards.at(0)->get_link()[named_prop_t(MBOARD_PROP_TX_DSP, tx_dsp_name)][DSP_PROP_HOST_RATE].as<double>();
    _io_impl->send_handler.set_samp_rate(tx_host_rate);
    for (size_t chan = 0; chan < _io_impl->send_handler.size(); chan++){
        _io_impl->send_handler.set_xport_chan_get_buff(chan, boost::bind(
            &usrp2_impl::io_impl::get_send_buff, _io_impl.get(), chan, _1
        ));
    }
    _io_impl->send_handler.set_converter(_tx_otw_type);
    _io_impl->send_handler.set_max_samples_per_packet(get_max_send_samps_per_packet());
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
    const send_buffs_type &buffs, size_t nsamps_per_buff,
    const tx_metadata_t &metadata, const io_type_t &io_type,
    send_mode_t send_mode, double timeout
){
    return _io_impl->send_handler.send(
        buffs, nsamps_per_buff,
        metadata, io_type,
        send_mode, timeout
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
    const size_t bpp = dsp_xports.front()->get_recv_frame_size() - hdr_size;
    return bpp/_rx_otw_type.get_sample_size();
}

size_t usrp2_impl::recv(
    const recv_buffs_type &buffs, size_t nsamps_per_buff,
    rx_metadata_t &metadata, const io_type_t &io_type,
    recv_mode_t recv_mode, double timeout
){
    return _io_impl->recv_handler.recv(
        buffs, nsamps_per_buff,
        metadata, io_type,
        recv_mode, timeout
    );
}
