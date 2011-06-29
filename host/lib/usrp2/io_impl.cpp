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

    io_impl(void):
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

    //setup rx otw type
    _rx_otw_type.width = 16;
    _rx_otw_type.shift = 0;
    _rx_otw_type.byteorder = uhd::otw_type_t::BO_BIG_ENDIAN;

    //setup tx otw type
    _tx_otw_type.width = 16;
    _tx_otw_type.shift = 0;
    _tx_otw_type.byteorder = uhd::otw_type_t::BO_BIG_ENDIAN;

    //create new io impl
    _io_impl = UHD_PIMPL_MAKE(io_impl, ());

    //init first so we dont have an access race
    BOOST_FOREACH(const std::string &mb, _mbc.keys()){
        //init the tx xport and flow control monitor
        _io_impl->tx_xports.push_back(_mbc[mb].dsp_xports.at(0));
        _io_impl->fc_mons.push_back(flow_control_monitor::sptr(new flow_control_monitor(
            USRP2_SRAM_BYTES/_mbc[mb].dsp_xports.at(0)->get_send_frame_size()
        )));
    }

    //create a new pirate thread for each zc if (yarr!!)
    boost::barrier spawn_barrier(_mbc.size()+1);
    size_t index = 0;
    BOOST_FOREACH(const std::string &mb, _mbc.keys()){
        //spawn a new pirate to plunder the recv booty
        _io_impl->recv_pirate_crew.create_thread(boost::bind(
            &usrp2_impl::io_impl::recv_pirate_loop,
            _io_impl.get(), boost::ref(spawn_barrier),
            _mbc[mb].err_xports.at(0), index++
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

void usrp2_impl::update_rx_subdev_spec(const std::string &which_mb, const subdev_spec_t &spec){
    boost::mutex::scoped_lock recv_lock = _io_impl->recv_handler.get_scoped_lock();
    property_tree::path_type root = "/mboards/" + which_mb + "/dboards";

    //sanity checking
    if (spec.size() == 0) throw uhd::value_error("rx subdev spec cant be empty");
    if (spec.size() > _mbc[which_mb].rx_dsps.size()) throw uhd::value_error("rx subdev spec too long");

    //setup mux for this spec
    for (size_t i = 0; i < spec.size(); i++){
        //ASSUME that we dont swap the rx fe mux...
        const std::string conn = _tree->access<std::string>(root / spec[i].db_name / "rx_frontends" / spec[i].sd_name / "connection").get();
        _mbc[which_mb].rx_dsps[i]->set_mux(conn);
    }

    //compute the new occupancy and resize
    _mbc[which_mb].rx_chan_occ = spec.size();
    size_t nchan = 0;
    BOOST_FOREACH(const std::string &mb, _mbc.keys()) nchan += _mbc[mb].rx_chan_occ;
    _io_impl->recv_handler.resize(nchan);

    //bind new callbacks for the handler
    size_t chan = 0;
    BOOST_FOREACH(const std::string &mb, _mbc.keys()){
        for (size_t dsp = 0; dsp < _mbc[mb].rx_chan_occ; dsp++){
            _mbc[mb].rx_dsps[dsp]->set_nsamps_per_packet(get_max_recv_samps_per_packet()); //seems to be a good place to set this
            _io_impl->recv_handler.set_xport_chan_get_buff(chan++, boost::bind(
                &zero_copy_if::get_recv_buff, _mbc[mb].dsp_xports[dsp], _1
            ));
        }
    }
}

void usrp2_impl::update_tx_subdev_spec(const std::string &which_mb, const subdev_spec_t &spec){
    boost::mutex::scoped_lock send_lock = _io_impl->send_handler.get_scoped_lock();
    property_tree::path_type root = "/mboards/" + which_mb + "/dboards";

    //sanity checking
    if (spec.size() != 1) throw uhd::value_error("tx subdev spec has to be size 1");

    //set the mux for this spec
    const std::string conn = _tree->access<std::string>(root / spec[0].db_name / "tx_frontends" / spec[0].sd_name / "connection").get();
    _mbc[which_mb].tx_fe->set_mux(conn);

    //compute the new occupancy and resize
    _mbc[which_mb].tx_chan_occ = spec.size();
    size_t nchan = 0;
    BOOST_FOREACH(const std::string &mb, _mbc.keys()) nchan += _mbc[mb].tx_chan_occ;
    _io_impl->send_handler.resize(nchan);

    //bind new callbacks for the handler
    size_t chan = 0, i = 0;
    BOOST_FOREACH(const std::string &mb, _mbc.keys()){
        for (size_t dsp = 0; dsp < _mbc[mb].tx_chan_occ; dsp++){
            _io_impl->send_handler.set_xport_chan_get_buff(chan++, boost::bind(
                &usrp2_impl::io_impl::get_send_buff, _io_impl.get(), i++, _1
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
    const size_t bpp = _mbc[_mbc.keys().front()].dsp_xports[0]->get_send_frame_size() - hdr_size;
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
    const size_t bpp = _mbc[_mbc.keys().front()].dsp_xports[0]->get_recv_frame_size() - hdr_size;
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
