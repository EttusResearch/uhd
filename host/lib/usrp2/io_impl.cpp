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
#include <numeric>
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

    io_impl(const size_t mboards):
        rx_chan_occ(mboards), tx_chan_occ(mboards),
        async_msg_fifo(100/*messages deep*/)
    {
        /* NOP */
    }

    ~io_impl(void){
        recv_pirate_crew.interrupt_all();
        recv_pirate_crew.join_all();
    }

    managed_send_buffer::sptr get_send_buff(size_t chan, double timeout){
        flow_control_monitor &fc_mon = *fc_mons[chan];

        //wait on flow control w/ timeout
        if (not fc_mon.check_fc_condition(timeout)) return managed_send_buffer::sptr();

        //get a buffer from the transport w/ timeout
        managed_send_buffer::sptr buff = tx_xports[chan]->get_send_buff(timeout);

        //write the flow control word into the buffer
        if (buff.get()) buff->cast<boost::uint32_t *>()[0] = uhd::htonx(fc_mon.get_curr_seq_out());

        return buff;
    }

    //subdev spec mapping information
    std::vector<size_t> rx_chan_occ, tx_chan_occ;

    //tx dsp: xports and flow control monitors
    std::vector<zero_copy_if::sptr> tx_xports;
    std::vector<flow_control_monitor::sptr> fc_mons;

    //state management for the vrt packet handler code
    sph::recv_packet_handler recv_handler;
    sph::send_packet_handler send_handler;

    //methods and variables for the pirate crew
    void recv_pirate_loop(boost::barrier &, zero_copy_if::sptr, size_t);
    boost::thread_group recv_pirate_crew;
    bounded_buffer<async_metadata_t> async_msg_fifo;
    double tick_rate;
};

/***********************************************************************
 * Receive Pirate Loop
 * - while raiding, loot for message packet
 * - update flow control condition count
 * - put async message packets into queue
 **********************************************************************/
void usrp2_impl::io_impl::recv_pirate_loop(
    boost::barrier &spawn_barrier,
    zero_copy_if::sptr err_xport,
    size_t index
){
    spawn_barrier.wait();
    set_thread_priority_safe();

    //store a reference to the flow control monitor (offset by max dsps)
    flow_control_monitor &fc_mon = *(this->fc_mons[index]);

    while (not boost::this_thread::interruption_requested()){
        managed_recv_buffer::sptr buff = err_xport->get_recv_buff();
        if (not buff.get()) continue; //ignore timeout/error buffers

        try{
            //extract the vrt header packet info
            vrt::if_packet_info_t if_packet_info;
            if_packet_info.num_packet_words32 = buff->size()/sizeof(boost::uint32_t);
            const boost::uint32_t *vrt_hdr = buff->cast<const boost::uint32_t *>();
            vrt::if_hdr_unpack_be(vrt_hdr, if_packet_info);

            //handle a tx async report message
            if (if_packet_info.sid == USRP2_TX_ASYNC_SID and if_packet_info.packet_type != vrt::if_packet_info_t::PACKET_TYPE_DATA){

                //fill in the async metadata
                async_metadata_t metadata;
                metadata.channel = index;
                metadata.has_time_spec = if_packet_info.has_tsi and if_packet_info.has_tsf;
                metadata.time_spec = time_spec_t(
                    time_t(if_packet_info.tsi), size_t(if_packet_info.tsf), tick_rate
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

    //TODO //This is a hack/fix for the lingering packet problem.

    //create new io impl
    _io_impl = UHD_PIMPL_MAKE(io_impl, (_mboard_stuff.size()));

    //init first so we dont have an access race
    for (size_t i = 0; i < _mboard_stuff.size(); i++){
        //init the tx xport and flow control monitor
        _io_impl->tx_xports.push_back(_mboard_stuff[i].dsp_xports.at(0));
        _io_impl->fc_mons.push_back(flow_control_monitor::sptr(new flow_control_monitor(
            USRP2_SRAM_BYTES/_mboard_stuff[i].dsp_xports.at(0)->get_send_frame_size()
        )));
    }

    //create a new pirate thread for each zc if (yarr!!)
    boost::barrier spawn_barrier(_mboard_stuff.size()+1);
    for (size_t i = 0; i < _mboard_stuff.size(); i++){
        //spawn a new pirate to plunder the recv booty
        _io_impl->recv_pirate_crew.create_thread(boost::bind(
            &usrp2_impl::io_impl::recv_pirate_loop,
            _io_impl.get(), boost::ref(spawn_barrier),
            _mboard_stuff[i].err_xports.at(0), i
        ));
    }
    spawn_barrier.wait();

    //init some handler stuff
    _io_impl->recv_handler.set_vrt_unpacker(&vrt::if_hdr_unpack_be);
    _io_impl->recv_handler.set_converter(_rx_otw_type);
    _io_impl->send_handler.set_vrt_packer(&vrt::if_hdr_pack_be, vrt_send_header_offset_words32);
    _io_impl->send_handler.set_converter(_tx_otw_type);
    _io_impl->send_handler.set_max_samples_per_packet(get_max_send_samps_per_packet());
}

void usrp2_impl::update_tick_rate(const double rate){
    _io_impl->tick_rate = rate;
    boost::mutex::scoped_lock recv_lock = _io_impl->recv_handler.get_scoped_lock();
    _io_impl->recv_handler.set_tick_rate(rate);
    boost::mutex::scoped_lock send_lock = _io_impl->send_handler.get_scoped_lock();
    _io_impl->send_handler.set_tick_rate(rate);
}

void usrp2_impl::update_rx_samp_rate(const double rate){
    boost::mutex::scoped_lock recv_lock = _io_impl->recv_handler.get_scoped_lock();
    _io_impl->recv_handler.set_samp_rate(rate);
}

void usrp2_impl::update_tx_samp_rate(const double rate){
    boost::mutex::scoped_lock send_lock = _io_impl->send_handler.get_scoped_lock();
    _io_impl->send_handler.set_samp_rate(rate);
}

void usrp2_impl::update_rx_subdev_spec(const size_t which_mb, const subdev_spec_t &spec){
    boost::mutex::scoped_lock recv_lock = _io_impl->recv_handler.get_scoped_lock();

    //TODO setup mux for this spec

    //compute the new occupancy and resize
    _io_impl->rx_chan_occ[which_mb] = spec.size();
    const size_t nchan = std::accumulate(_io_impl->rx_chan_occ.begin(), _io_impl->rx_chan_occ.end(), 0);
    _io_impl->recv_handler.resize(nchan);

    //bind new callbacks for the handler
    size_t chan = 0;
    for (size_t mb = 0; mb < _mboard_stuff.size(); mb++){
        for (size_t dsp = 0; dsp < _io_impl->rx_chan_occ[mb]; dsp++){
            _io_impl->recv_handler.set_xport_chan_get_buff(chan++, boost::bind(
                &zero_copy_if::get_recv_buff, _mboard_stuff[mb].dsp_xports[dsp], _1
            ));
        }
    }
}

void usrp2_impl::update_tx_subdev_spec(const size_t which_mb, const subdev_spec_t &spec_){
    boost::mutex::scoped_lock send_lock = _io_impl->send_handler.get_scoped_lock();
    property_tree::path_type root = str(boost::format("/mboards/%u/dboards") % which_mb);

    //sanity checking
    subdev_spec_t spec(spec_);
    if (spec.size() == 0){
        //determine the first subdev spec that exists
        const std::string db_name = _tree->list(root).at(0);
        const std::string sd_name = _tree->list(root / db_name / "tx_frontends").at(0);
        spec.push_back(db_name + ":" + sd_name);
    }
    if (spec.size() != 1) throw uhd::value_error("tx subdev spec has to be size 1");

    //set the mux for this spec
    const std::string conn = _tree->access<std::string>(root / spec[0].db_name / "tx_frontends" / spec[0].sd_name / "connection").get();
    _mboard_stuff[which_mb].tx_fe->set_mux(conn);

    //compute the new occupancy and resize
    _io_impl->tx_chan_occ[which_mb] = spec.size();
    const size_t nchan = std::accumulate(_io_impl->tx_chan_occ.begin(), _io_impl->tx_chan_occ.end(), 0);
    _io_impl->send_handler.resize(nchan);

    //bind new callbacks for the handler
    size_t chan = 0;
    for (size_t mb = 0; mb < _mboard_stuff.size(); mb++){
        for (size_t dsp = 0; dsp < _io_impl->tx_chan_occ[mb]; dsp++){
            _io_impl->send_handler.set_xport_chan_get_buff(chan++, boost::bind(
                &usrp2_impl::io_impl::get_send_buff, _io_impl.get(), mb, _1
            ));
        }
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
    const size_t bpp = _mboard_stuff[0].dsp_xports[0]->get_send_frame_size() - hdr_size;
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
    const size_t bpp = _mboard_stuff[0].dsp_xports[0]->get_recv_frame_size() - hdr_size;
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
