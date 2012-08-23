//
// Copyright 2010-2012 Ettus Research LLC
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
#include "async_packet_handler.hpp"
#include "../../transport/super_recv_packet_handler.hpp"
#include "../../transport/super_send_packet_handler.hpp"
#include "usrp2_impl.hpp"
#include "usrp2_regs.hpp"
#include "fw_common.h"
#include <uhd/utils/log.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/utils/tasks.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/thread_priority.hpp>
#include <uhd/transport/bounded_buffer.hpp>
#include <boost/thread/thread.hpp>
#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/make_shared.hpp>
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
    flow_control_monitor(seq_type max_seqs_out):_max_seqs_out(max_seqs_out){
        this->clear();
        _ready_fcn = boost::bind(&flow_control_monitor::ready, this);
    }

    //! Clear the monitor, Ex: when a streamer is created
    void clear(void){
        _last_seq_out = 0;
        _last_seq_ack = 0;
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
    seq_type _last_seq_out, _last_seq_ack;
    const seq_type _max_seqs_out;
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
        async_msg_fifo(1000/*messages deep*/),
        tick_rate(1 /*non-zero default*/)
    {
        /* NOP */
    }

    ~io_impl(void){
        //Manually deconstuct the tasks, since this was not happening automatically.
        pirate_tasks.clear();
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

    //methods and variables for the pirate crew
    void recv_pirate_loop(zero_copy_if::sptr, size_t);
    std::list<task::sptr> pirate_tasks;
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
    zero_copy_if::sptr err_xport, size_t index
){
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
                load_metadata_from_buff(uhd::ntohx<boost::uint32_t>, metadata, if_packet_info, vrt_hdr, tick_rate, index);

                //catch the flow control packets and react
                if (metadata.event_code == 0){
                    boost::uint32_t fc_word32 = (vrt_hdr + if_packet_info.num_header_words32)[1];
                    fc_mon.update_fc_condition(uhd::ntohx(fc_word32));
                    continue;
                }
                //else UHD_MSG(often) << "metadata.event_code " << metadata.event_code << std::endl;
                async_msg_fifo.push_with_pop_on_full(metadata);

                standard_async_msg_prints(metadata);
            }
            else{
                //TODO unknown received packet, may want to print error...
            }
        }catch(const std::exception &e){
            UHD_MSG(error) << "Error in recv pirate loop: " << e.what() << std::endl;
        }
    }
}

/***********************************************************************
 * Helper Functions
 **********************************************************************/
void usrp2_impl::io_init(void){
    //create new io impl
    _io_impl = UHD_PIMPL_MAKE(io_impl, ());

    //init first so we dont have an access race
    BOOST_FOREACH(const std::string &mb, _mbc.keys()){
        //init the tx xport and flow control monitor
        _io_impl->tx_xports.push_back(_mbc[mb].tx_dsp_xport);
        _io_impl->fc_mons.push_back(flow_control_monitor::sptr(new flow_control_monitor(
            USRP2_SRAM_BYTES/_mbc[mb].tx_dsp_xport->get_send_frame_size()
        )));
    }

    //allocate streamer weak ptrs containers
    BOOST_FOREACH(const std::string &mb, _mbc.keys()){
        _mbc[mb].rx_streamers.resize(_mbc[mb].rx_dsps.size());
        _mbc[mb].tx_streamers.resize(1/*known to be 1 dsp*/);
    }

    //create a new pirate thread for each zc if (yarr!!)
    size_t index = 0;
    BOOST_FOREACH(const std::string &mb, _mbc.keys()){
        //spawn a new pirate to plunder the recv booty
        _io_impl->pirate_tasks.push_back(task::make(boost::bind(
            &usrp2_impl::io_impl::recv_pirate_loop, _io_impl.get(),
            _mbc[mb].tx_dsp_xport, index++
        )));
    }
}

void usrp2_impl::update_tick_rate(const double rate){
    _io_impl->tick_rate = rate; //shadow for async msg

    //update the tick rate on all existing streamers -> thread safe
    BOOST_FOREACH(const std::string &mb, _mbc.keys()){
        for (size_t i = 0; i < _mbc[mb].rx_streamers.size(); i++){
            boost::shared_ptr<sph::recv_packet_streamer> my_streamer =
                boost::dynamic_pointer_cast<sph::recv_packet_streamer>(_mbc[mb].rx_streamers[i].lock());
            if (my_streamer.get() == NULL) continue;
            my_streamer->set_tick_rate(rate);
        }
        for (size_t i = 0; i < _mbc[mb].tx_streamers.size(); i++){
            boost::shared_ptr<sph::send_packet_streamer> my_streamer =
                boost::dynamic_pointer_cast<sph::send_packet_streamer>(_mbc[mb].tx_streamers[i].lock());
            if (my_streamer.get() == NULL) continue;
            my_streamer->set_tick_rate(rate);
        }
    }
}

void usrp2_impl::update_rx_samp_rate(const std::string &mb, const size_t dsp, const double rate){
    boost::shared_ptr<sph::recv_packet_streamer> my_streamer =
        boost::dynamic_pointer_cast<sph::recv_packet_streamer>(_mbc[mb].rx_streamers[dsp].lock());
    if (my_streamer.get() == NULL) return;

    my_streamer->set_samp_rate(rate);
    const double adj = _mbc[mb].rx_dsps[dsp]->get_scaling_adjustment();
    my_streamer->set_scale_factor(adj);
}

void usrp2_impl::update_tx_samp_rate(const std::string &mb, const size_t dsp, const double rate){
    boost::shared_ptr<sph::send_packet_streamer> my_streamer =
        boost::dynamic_pointer_cast<sph::send_packet_streamer>(_mbc[mb].tx_streamers[dsp].lock());
    if (my_streamer.get() == NULL) return;

    my_streamer->set_samp_rate(rate);
    const double adj = _mbc[mb].tx_dsp->get_scaling_adjustment();
    my_streamer->set_scale_factor(adj);
}

void usrp2_impl::update_rates(void){
    BOOST_FOREACH(const std::string &mb, _mbc.keys()){
        fs_path root = "/mboards/" + mb;
        _tree->access<double>(root / "tick_rate").update();

        //and now that the tick rate is set, init the host rates to something
        BOOST_FOREACH(const std::string &name, _tree->list(root / "rx_dsps")){
            _tree->access<double>(root / "rx_dsps" / name / "rate" / "value").update();
        }
        BOOST_FOREACH(const std::string &name, _tree->list(root / "tx_dsps")){
            _tree->access<double>(root / "tx_dsps" / name / "rate" / "value").update();
        }
    }
}

void usrp2_impl::update_rx_subdev_spec(const std::string &which_mb, const subdev_spec_t &spec){
    fs_path root = "/mboards/" + which_mb + "/dboards";

    //sanity checking
    validate_subdev_spec(_tree, spec, "rx", which_mb);

    //setup mux for this spec
    bool fe_swapped = false;
    for (size_t i = 0; i < spec.size(); i++){
        const std::string conn = _tree->access<std::string>(root / spec[i].db_name / "rx_frontends" / spec[i].sd_name / "connection").get();
        if (i == 0 and (conn == "QI" or conn == "Q")) fe_swapped = true;
        _mbc[which_mb].rx_dsps[i]->set_mux(conn, fe_swapped);
    }
    _mbc[which_mb].rx_fe->set_mux(fe_swapped);

    //compute the new occupancy and resize
    _mbc[which_mb].rx_chan_occ = spec.size();
    size_t nchan = 0;
    BOOST_FOREACH(const std::string &mb, _mbc.keys()) nchan += _mbc[mb].rx_chan_occ;
}

void usrp2_impl::update_tx_subdev_spec(const std::string &which_mb, const subdev_spec_t &spec){
    fs_path root = "/mboards/" + which_mb + "/dboards";

    //sanity checking
    validate_subdev_spec(_tree, spec, "tx", which_mb);

    //set the mux for this spec
    const std::string conn = _tree->access<std::string>(root / spec[0].db_name / "tx_frontends" / spec[0].sd_name / "connection").get();
    _mbc[which_mb].tx_fe->set_mux(conn);

    //compute the new occupancy and resize
    _mbc[which_mb].tx_chan_occ = spec.size();
    size_t nchan = 0;
    BOOST_FOREACH(const std::string &mb, _mbc.keys()) nchan += _mbc[mb].tx_chan_occ;
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
 * Stream destination programmer
 **********************************************************************/
void usrp2_impl::program_stream_dest(
    zero_copy_if::sptr &xport, const uhd::stream_args_t &args
){
    //perform an initial flush of transport
    while (xport->get_recv_buff(0.0)){}

    //program the stream command
    usrp2_stream_ctrl_t stream_ctrl = usrp2_stream_ctrl_t();
    stream_ctrl.sequence = uhd::htonx(boost::uint32_t(0 /* don't care seq num */));
    stream_ctrl.vrt_hdr = uhd::htonx(boost::uint32_t(USRP2_INVALID_VRT_HEADER));

    //user has provided an alternative address and port for destination
    if (args.args.has_key("addr") and args.args.has_key("port")){
        UHD_MSG(status) << boost::format(
            "Programming streaming destination for custom address.\n"
            "IPv4 Address: %s, UDP Port: %s\n"
        ) % args.args["addr"] % args.args["port"] << std::endl;

        asio::io_service io_service;
        asio::ip::udp::resolver resolver(io_service);
        asio::ip::udp::resolver::query query(asio::ip::udp::v4(), args.args["addr"], args.args["port"]);
        asio::ip::udp::endpoint endpoint = *resolver.resolve(query);
        stream_ctrl.ip_addr = uhd::htonx(boost::uint32_t(endpoint.address().to_v4().to_ulong()));
        stream_ctrl.udp_port = uhd::htonx(boost::uint32_t(endpoint.port()));

        for (size_t i = 0; i < 3; i++){
            UHD_MSG(status) << "ARP attempt " << i << std::endl;
            managed_send_buffer::sptr send_buff = xport->get_send_buff();
            std::memcpy(send_buff->cast<void *>(), &stream_ctrl, sizeof(stream_ctrl));
            send_buff->commit(sizeof(stream_ctrl));
            send_buff.reset();
            boost::this_thread::sleep(boost::posix_time::milliseconds(300));
            managed_recv_buffer::sptr recv_buff = xport->get_recv_buff(0.0);
            if (recv_buff and recv_buff->size() >= sizeof(boost::uint32_t)){
                const boost::uint32_t result = uhd::ntohx(recv_buff->cast<const boost::uint32_t *>()[0]);
                if (result == 0){
                    UHD_MSG(status) << "Success! " << std::endl;
                    return;
                }
            }
        }
        throw uhd::runtime_error("Device failed to ARP when programming alternative streaming destination.");
    }

    else{
        //send the partial stream control without destination
        managed_send_buffer::sptr send_buff = xport->get_send_buff();
        std::memcpy(send_buff->cast<void *>(), &stream_ctrl, sizeof(stream_ctrl));
        send_buff->commit(sizeof(stream_ctrl)/2);
    }
}

/***********************************************************************
 * Receive streamer
 **********************************************************************/
rx_streamer::sptr usrp2_impl::get_rx_stream(const uhd::stream_args_t &args_){
    stream_args_t args = args_;

    //setup defaults for unspecified values
    args.otw_format = args.otw_format.empty()? "sc16" : args.otw_format;
    args.channels = args.channels.empty()? std::vector<size_t>(1, 0) : args.channels;

    //calculate packet size
    static const size_t hdr_size = 0
        + vrt::max_if_hdr_words32*sizeof(boost::uint32_t)
        + sizeof(vrt::if_packet_info_t().tlr) //forced to have trailer
        - sizeof(vrt::if_packet_info_t().cid) //no class id ever used
        - sizeof(vrt::if_packet_info_t().tsi) //no int time ever used
    ;
    const size_t bpp = _mbc[_mbc.keys().front()].rx_dsp_xports[0]->get_recv_frame_size() - hdr_size;
    const size_t bpi = convert::get_bytes_per_item(args.otw_format);
    const size_t spp = unsigned(args.args.cast<double>("spp", bpp/bpi));

    //make the new streamer given the samples per packet
    boost::shared_ptr<sph::recv_packet_streamer> my_streamer = boost::make_shared<sph::recv_packet_streamer>(spp);

    //init some streamer stuff
    my_streamer->resize(args.channels.size());
    my_streamer->set_vrt_unpacker(&vrt::if_hdr_unpack_be);

    //set the converter
    uhd::convert::id_type id;
    id.input_format = args.otw_format + "_item32_be";
    id.num_inputs = 1;
    id.output_format = args.cpu_format;
    id.num_outputs = 1;
    my_streamer->set_converter(id);

    //bind callbacks for the handler
    for (size_t chan_i = 0; chan_i < args.channels.size(); chan_i++){
        const size_t chan = args.channels[chan_i];
        size_t num_chan_so_far = 0;
        BOOST_FOREACH(const std::string &mb, _mbc.keys()){
            num_chan_so_far += _mbc[mb].rx_chan_occ;
            if (chan < num_chan_so_far){
                const size_t dsp = chan + _mbc[mb].rx_chan_occ - num_chan_so_far;
                _mbc[mb].rx_dsps[dsp]->set_nsamps_per_packet(spp); //seems to be a good place to set this
                _mbc[mb].rx_dsps[dsp]->setup(args);
                this->program_stream_dest(_mbc[mb].rx_dsp_xports[dsp], args);
                my_streamer->set_xport_chan_get_buff(chan_i, boost::bind(
                    &zero_copy_if::get_recv_buff, _mbc[mb].rx_dsp_xports[dsp], _1
                ), true /*flush*/);
                _mbc[mb].rx_streamers[dsp] = my_streamer; //store weak pointer
                break;
            }
        }
    }

    //set the packet threshold to be an entire socket buffer's worth
    const size_t packets_per_sock_buff = size_t(50e6/_mbc[_mbc.keys().front()].rx_dsp_xports[0]->get_recv_frame_size());
    my_streamer->set_alignment_failure_threshold(packets_per_sock_buff);

    //sets all tick and samp rates on this streamer
    this->update_rates();

    return my_streamer;
}

/***********************************************************************
 * Transmit streamer
 **********************************************************************/
tx_streamer::sptr usrp2_impl::get_tx_stream(const uhd::stream_args_t &args_){
    stream_args_t args = args_;

    //setup defaults for unspecified values
    args.otw_format = args.otw_format.empty()? "sc16" : args.otw_format;
    args.channels = args.channels.empty()? std::vector<size_t>(1, 0) : args.channels;

    //calculate packet size
    static const size_t hdr_size = 0
        + vrt_send_header_offset_words32*sizeof(boost::uint32_t)
        + vrt::max_if_hdr_words32*sizeof(boost::uint32_t)
        + sizeof(vrt::if_packet_info_t().tlr) //forced to have trailer
        - sizeof(vrt::if_packet_info_t().cid) //no class id ever used
        - sizeof(vrt::if_packet_info_t().sid) //no stream id ever used
        - sizeof(vrt::if_packet_info_t().tsi) //no int time ever used
    ;
    const size_t bpp = _mbc[_mbc.keys().front()].tx_dsp_xport->get_send_frame_size() - hdr_size;
    const size_t spp = bpp/convert::get_bytes_per_item(args.otw_format);

    //make the new streamer given the samples per packet
    boost::shared_ptr<sph::send_packet_streamer> my_streamer = boost::make_shared<sph::send_packet_streamer>(spp);

    //init some streamer stuff
    my_streamer->resize(args.channels.size());
    my_streamer->set_vrt_packer(&vrt::if_hdr_pack_be, vrt_send_header_offset_words32);

    //set the converter
    uhd::convert::id_type id;
    id.input_format = args.cpu_format;
    id.num_inputs = 1;
    id.output_format = args.otw_format + "_item32_be";
    id.num_outputs = 1;
    my_streamer->set_converter(id);

    //bind callbacks for the handler
    for (size_t chan_i = 0; chan_i < args.channels.size(); chan_i++){
        const size_t chan = args.channels[chan_i];
        size_t num_chan_so_far = 0;
        size_t abs = 0;
        BOOST_FOREACH(const std::string &mb, _mbc.keys()){
            num_chan_so_far += _mbc[mb].tx_chan_occ;
            if (chan < num_chan_so_far){
                const size_t dsp = chan + _mbc[mb].tx_chan_occ - num_chan_so_far;
                if (not args.args.has_key("noclear")){
                    _io_impl->fc_mons[abs]->clear();
                }
                _mbc[mb].tx_dsp->setup(args);
                my_streamer->set_xport_chan_get_buff(chan_i, boost::bind(
                    &usrp2_impl::io_impl::get_send_buff, _io_impl.get(), abs, _1
                ));
                _mbc[mb].tx_streamers[dsp] = my_streamer; //store weak pointer
                break;
            }
            abs += 1; //assume 1 tx dsp
        }
    }

    //sets all tick and samp rates on this streamer
    this->update_rates();

    return my_streamer;
}
