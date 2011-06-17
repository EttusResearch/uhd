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
#include "usrp_e100_impl.hpp"
#include "usrp_e100_regs.hpp"
#include <uhd/utils/msg.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/usrp/dsp_utils.hpp>
#include <uhd/usrp/dsp_props.hpp>
#include <uhd/utils/thread_priority.hpp>
#include <uhd/transport/bounded_buffer.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/barrier.hpp>
#include <sstream>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;

/***********************************************************************
 * Constants
 **********************************************************************/
static const int underflow_flags = async_metadata_t::EVENT_CODE_UNDERFLOW | async_metadata_t::EVENT_CODE_UNDERFLOW_IN_PACKET;

/***********************************************************************
 * Helpers
 **********************************************************************/
#if 1
#  define debug_print_buff(...)
#else
static void debug_print_buff(const std::string &what, managed_recv_buffer::sptr buff){
    std::ostringstream ss;
    ss << boost::format(
        "This is a %s packet, %u bytes.\n"
    ) % what % buff->size();
    for (size_t i = 0; i < 9; i++){
        ss << boost::format("    buff[%u] = 0x%08x\n") % i % buff->cast<const boost::uint32_t *>()[i];
    }
    ss << std::endl << std::endl;
    UHD_MSG(status) << ss.str();
}
#endif

/***********************************************************************
 * io impl details (internal to this file)
 * - pirate crew of 1
 * - bounded buffer
 * - thread loop
 * - vrt packet handler states
 **********************************************************************/
struct usrp_e100_impl::io_impl{
    io_impl(zero_copy_if::sptr &xport):
        data_xport(xport),
        async_msg_fifo(100/*messages deep*/)
    {
        for (size_t i = 0; i < E100_NUM_RX_DSPS; i++){
            typedef bounded_buffer<managed_recv_buffer::sptr> booty_type;
            recv_pirate_booty.push_back(new booty_type(data_xport->get_num_recv_frames()));
        }
    }

    ~io_impl(void){
        recv_pirate_crew.interrupt_all();
        recv_pirate_crew.join_all();
        for (size_t i = 0; i < recv_pirate_booty.size(); i++){
            delete recv_pirate_booty[i];
        }
    }

    managed_recv_buffer::sptr get_recv_buff(const size_t index, double timeout){
        boost::this_thread::disable_interruption di; //disable because the wait can throw
        managed_recv_buffer::sptr buff;
        recv_pirate_booty[index]->pop_with_timed_wait(buff, timeout);
        return buff; //ASSUME buff == NULL when pop times-out
    }

    //The data transport is listed first so that it is deconstructed last,
    //which is after the states and booty which may hold managed buffers.
    //This comment is invalid because its now a reference and not stored here.
    zero_copy_if::sptr &data_xport;

    //state management for the vrt packet handler code
    sph::recv_packet_handler recv_handler;
    sph::send_packet_handler send_handler;
    bool continuous_streaming;

    //a pirate's life is the life for me!
    void recv_pirate_loop(boost::barrier &, usrp_e100_clock_ctrl::sptr);
    std::vector<bounded_buffer<managed_recv_buffer::sptr> *> recv_pirate_booty;
    bounded_buffer<async_metadata_t> async_msg_fifo;
    boost::thread_group recv_pirate_crew;
};

/***********************************************************************
 * Receive Pirate Loop
 * - while raiding, loot for recv buffers
 * - put booty into the alignment buffer
 **********************************************************************/
void usrp_e100_impl::io_impl::recv_pirate_loop(
    boost::barrier &spawn_barrier, usrp_e100_clock_ctrl::sptr clock_ctrl
){
    spawn_barrier.wait();
    set_thread_priority_safe();

    while (not boost::this_thread::interruption_requested()){
        managed_recv_buffer::sptr buff = this->data_xport->get_recv_buff();
        if (buff.get() == NULL) continue; //ignore timeout buffers

        //handle an rx data packet or inline message
        const boost::uint32_t *vrt_hdr = buff->cast<const boost::uint32_t *>();
        const size_t rx_index = uhd::wtohx(vrt_hdr[1]) - E100_DSP_SID_BASE;
        if (rx_index < E100_NUM_RX_DSPS){
            debug_print_buff("data", buff);
            recv_pirate_booty[rx_index]->push_with_wait(buff);
            continue;
        }

        //otherwise, unpack the vrt header and process below...
        vrt::if_packet_info_t if_packet_info;
        if_packet_info.num_packet_words32 = buff->size()/sizeof(boost::uint32_t);
        try{vrt::if_hdr_unpack_le(vrt_hdr, if_packet_info);}
        catch(const std::exception &e){
            UHD_MSG(error) << "Error unpacking vrt header:\n" << e.what() << std::endl;
            continue;
        }

        //handle a tx async report message
        if (if_packet_info.sid == E100_ASYNC_SID and if_packet_info.packet_type != vrt::if_packet_info_t::PACKET_TYPE_DATA){
            debug_print_buff("async", buff);

            //fill in the async metadata
            async_metadata_t metadata;
            metadata.channel = 0;
            metadata.has_time_spec = if_packet_info.has_tsi and if_packet_info.has_tsf;
            metadata.time_spec = time_spec_t(
                time_t(if_packet_info.tsi), long(if_packet_info.tsf), clock_ctrl->get_fpga_clock_rate()
            );
            metadata.event_code = async_metadata_t::event_code_t(sph::get_context_code(vrt_hdr, if_packet_info));

            //print the famous U, and push the metadata into the message queue
            if (metadata.event_code & underflow_flags) UHD_MSG(fastpath) << "U";
            async_msg_fifo.push_with_pop_on_full(metadata);
            continue;
        }

        debug_print_buff("unknown", buff);
    }
}

/***********************************************************************
 * Helper Functions
 **********************************************************************/
void usrp_e100_impl::io_init(void){

    //setup before the registers (transport called to calculate max spp)
    _io_impl = UHD_PIMPL_MAKE(io_impl, (_data_xport));

    //clear state machines
    _iface->poke32(UE_REG_CLEAR_RX, 0);
    _iface->poke32(UE_REG_CLEAR_TX, 0);

    //spawn a pirate, yarrr!
    boost::barrier spawn_barrier(2);
    _io_impl->recv_pirate_crew.create_thread(boost::bind(
        &usrp_e100_impl::io_impl::recv_pirate_loop, _io_impl.get(),
        boost::ref(spawn_barrier), _clock_ctrl
    ));
    spawn_barrier.wait();
    //update mapping here since it didnt b4 when io init not called first
    update_xport_channel_mapping();
}

void usrp_e100_impl::update_xport_channel_mapping(void){
    if (_io_impl.get() == NULL) return; //not inited yet

    //set all of the relevant properties on the handler
    boost::mutex::scoped_lock recv_lock = _io_impl->recv_handler.get_scoped_lock();
    _io_impl->recv_handler.resize(_rx_subdev_spec.size());
    _io_impl->recv_handler.set_vrt_unpacker(&vrt::if_hdr_unpack_le);
    _io_impl->recv_handler.set_tick_rate(_clock_ctrl->get_fpga_clock_rate());
    //FIXME assumes homogeneous rates across all dsp
    _io_impl->recv_handler.set_samp_rate(_rx_dsp_proxies[_rx_dsp_proxies.keys().at(0)]->get_link()[DSP_PROP_HOST_RATE].as<double>());
    for (size_t chan = 0; chan < _io_impl->recv_handler.size(); chan++){
        _io_impl->recv_handler.set_xport_chan_get_buff(chan, boost::bind(
            &usrp_e100_impl::io_impl::get_recv_buff, _io_impl.get(), chan, _1
        ));
        _io_impl->recv_handler.set_overflow_handler(chan, boost::bind(
            &usrp_e100_impl::handle_overrun, this, chan
        ));
    }
    _io_impl->recv_handler.set_converter(_recv_otw_type);

    //set all of the relevant properties on the handler
    boost::mutex::scoped_lock send_lock = _io_impl->send_handler.get_scoped_lock();
    _io_impl->send_handler.resize(_tx_subdev_spec.size());
    _io_impl->send_handler.set_vrt_packer(&vrt::if_hdr_pack_le);
    _io_impl->send_handler.set_tick_rate(_clock_ctrl->get_fpga_clock_rate());
    //FIXME assumes homogeneous rates across all dsp
    _io_impl->send_handler.set_samp_rate(_tx_dsp_proxies[_tx_dsp_proxies.keys().at(0)]->get_link()[DSP_PROP_HOST_RATE].as<double>());
    for (size_t chan = 0; chan < _io_impl->send_handler.size(); chan++){
        _io_impl->send_handler.set_xport_chan_get_buff(chan, boost::bind(
            &uhd::transport::zero_copy_if::get_send_buff, _io_impl->data_xport, _1
        ));
    }
    _io_impl->send_handler.set_converter(_send_otw_type);
    _io_impl->send_handler.set_max_samples_per_packet(get_max_send_samps_per_packet());
}

void usrp_e100_impl::issue_ddc_stream_cmd(const stream_cmd_t &stream_cmd, const size_t index){
    _io_impl->continuous_streaming = (stream_cmd.stream_mode == stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    _iface->poke32(UE_REG_RX_CTRL_STREAM_CMD(index), dsp_type1::calc_stream_cmd_word(stream_cmd));
    _iface->poke32(UE_REG_RX_CTRL_TIME_SECS(index),  boost::uint32_t(stream_cmd.time_spec.get_full_secs()));
    _iface->poke32(UE_REG_RX_CTRL_TIME_TICKS(index), stream_cmd.time_spec.get_tick_count(_clock_ctrl->get_fpga_clock_rate()));
}

void usrp_e100_impl::handle_overrun(const size_t index){
    if (_io_impl->continuous_streaming){
        this->issue_ddc_stream_cmd(stream_cmd_t::STREAM_MODE_START_CONTINUOUS, index);
    }
}

/***********************************************************************
 * Data Send
 **********************************************************************/
size_t usrp_e100_impl::get_max_send_samps_per_packet(void) const{
    static const size_t hdr_size = 0
        + vrt::max_if_hdr_words32*sizeof(boost::uint32_t)
        - sizeof(vrt::if_packet_info_t().cid) //no class id ever used
    ;
    size_t bpp = _send_frame_size - hdr_size;
    return bpp/_send_otw_type.get_sample_size();
}

size_t usrp_e100_impl::send(
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
 * Data Recv
 **********************************************************************/
size_t usrp_e100_impl::get_max_recv_samps_per_packet(void) const{
    static const size_t hdr_size = 0
        + vrt::max_if_hdr_words32*sizeof(boost::uint32_t)
        + sizeof(vrt::if_packet_info_t().tlr) //forced to have trailer
        - sizeof(vrt::if_packet_info_t().cid) //no class id ever used
    ;
    size_t bpp = _recv_frame_size - hdr_size;
    return bpp/_recv_otw_type.get_sample_size();
}

size_t usrp_e100_impl::recv(
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

/***********************************************************************
 * Async Recv
 **********************************************************************/
bool usrp_e100_impl::recv_async_msg(
    async_metadata_t &async_metadata, double timeout
){
    boost::this_thread::disable_interruption di; //disable because the wait can throw
    return _io_impl->async_msg_fifo.pop_with_timed_wait(async_metadata, timeout);
}
