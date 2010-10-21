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

#include "usrp_e_impl.hpp"
#include "usrp_e_regs.hpp"
#include <uhd/usrp/dsp_utils.hpp>
#include <uhd/utils/thread_priority.hpp>
#include <uhd/transport/bounded_buffer.hpp>
#include "../../transport/vrt_packet_handler.hpp"
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <iostream>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;

zero_copy_if::sptr usrp_e_make_mmap_zero_copy(usrp_e_iface::sptr iface);

/***********************************************************************
 * Constants
 **********************************************************************/
static const size_t tx_async_report_sid = 1;
static const int underflow_flags = async_metadata_t::EVENT_CODE_UNDERFLOW | async_metadata_t::EVENT_CODE_UNDERFLOW_IN_PACKET;
static const bool recv_debug = false;

/***********************************************************************
 * io impl details (internal to this file)
 * - pirate crew of 1
 * - bounded buffer
 * - thread loop
 * - vrt packet handler states
 **********************************************************************/
struct usrp_e_impl::io_impl{
    //state management for the vrt packet handler code
    vrt_packet_handler::recv_state packet_handler_recv_state;
    vrt_packet_handler::send_state packet_handler_send_state;
    zero_copy_if::sptr data_xport;
    bool continuous_streaming;
    io_impl(usrp_e_iface::sptr iface):
        data_xport(usrp_e_make_mmap_zero_copy(iface)),
        recv_pirate_booty(recv_booty_type::make(data_xport->get_num_recv_frames())),
        async_msg_fifo(bounded_buffer<async_metadata_t>::make(100/*messages deep*/))
    {
        /* NOP */
    }

    ~io_impl(void){
        recv_pirate_crew_raiding = false;
        recv_pirate_crew.interrupt_all();
        recv_pirate_crew.join_all();
    }

    bool get_recv_buffs(vrt_packet_handler::managed_recv_buffs_t &buffs, double timeout){
        UHD_ASSERT_THROW(buffs.size() == 1);
        boost::this_thread::disable_interruption di; //disable because the wait can throw
        return recv_pirate_booty->pop_with_timed_wait(buffs.front(), timeout);
    }

    //a pirate's life is the life for me!
    void recv_pirate_loop();
    typedef bounded_buffer<managed_recv_buffer::sptr> recv_booty_type;
    recv_booty_type::sptr recv_pirate_booty;
    bounded_buffer<async_metadata_t>::sptr async_msg_fifo;
    boost::thread_group recv_pirate_crew;
    bool recv_pirate_crew_raiding;
};

/***********************************************************************
 * Receive Pirate Loop
 * - while raiding, loot for recv buffers
 * - put booty into the alignment buffer
 **********************************************************************/
void usrp_e_impl::io_impl::recv_pirate_loop(

){
    set_thread_priority_safe();
    recv_pirate_crew_raiding = true;

    while(recv_pirate_crew_raiding){
        managed_recv_buffer::sptr buff = this->data_xport->get_recv_buff();
        if (not buff.get()) continue; //ignore timeout/error buffers

        if (recv_debug){
            std::cout << "len " << buff->size() << std::endl;
            for (size_t i = 0; i < 9; i++){
                std::cout << boost::format("    0x%08x") % buff->cast<const boost::uint32_t *>()[i] << std::endl;
            }
            std::cout << std::endl << std::endl;
        }

        try{
            //extract the vrt header packet info
            vrt::if_packet_info_t if_packet_info;
            if_packet_info.num_packet_words32 = buff->size()/sizeof(boost::uint32_t);
            const boost::uint32_t *vrt_hdr = buff->cast<const boost::uint32_t *>();
            vrt::if_hdr_unpack_le(vrt_hdr, if_packet_info);

            //handle a tx async report message
            if (if_packet_info.sid == tx_async_report_sid and if_packet_info.packet_type != vrt::if_packet_info_t::PACKET_TYPE_DATA){

                //fill in the async metadata
                async_metadata_t metadata;
                metadata.channel = 0;
                metadata.has_time_spec = if_packet_info.has_tsi and if_packet_info.has_tsf;
                metadata.time_spec = time_spec_t(
                    time_t(if_packet_info.tsi), size_t(if_packet_info.tsf), MASTER_CLOCK_RATE
                );
                metadata.event_code = vrt_packet_handler::get_context_code<async_metadata_t::event_code_t>(vrt_hdr, if_packet_info);

                //print the famous U, and push the metadata into the message queue
                if (metadata.event_code & underflow_flags) std::cerr << "U" << std::flush;
                async_msg_fifo->push_with_pop_on_full(metadata);
                continue;
            }

            //same number of frames as the data transport -> always immediate
            recv_pirate_booty->push_with_wait(buff);

        }catch(const std::exception &e){
            std::cerr << "Error (usrp-e recv pirate loop): " << e.what() << std::endl;
        }
    }
}

/***********************************************************************
 * Helper Functions
 **********************************************************************/
void usrp_e_impl::io_init(void){
    //setup otw types
    _send_otw_type.width = 16;
    _send_otw_type.shift = 0;
    _send_otw_type.byteorder = otw_type_t::BO_LITTLE_ENDIAN;

    _recv_otw_type.width = 16;
    _recv_otw_type.shift = 0;
    _recv_otw_type.byteorder = otw_type_t::BO_LITTLE_ENDIAN;

    //setup rx data path
    _iface->poke32(UE_REG_CTRL_RX_NSAMPS_PER_PKT, get_max_recv_samps_per_packet());
    _iface->poke32(UE_REG_CTRL_RX_NCHANNELS, 1);
    _iface->poke32(UE_REG_CTRL_RX_CLEAR_OVERRUN, 1); //reset
    _iface->poke32(UE_REG_CTRL_RX_VRT_HEADER, 0
        | (0x1 << 28) //if data with stream id
        | (0x1 << 26) //has trailer
        | (0x3 << 22) //integer time other
        | (0x1 << 20) //fractional time sample count
    );
    _iface->poke32(UE_REG_CTRL_RX_VRT_STREAM_ID, 0);
    _iface->poke32(UE_REG_CTRL_RX_VRT_TRAILER, 0);

    //setup the tx policy
    _iface->poke32(UE_REG_CTRL_TX_REPORT_SID, tx_async_report_sid);
    _iface->poke32(UE_REG_CTRL_TX_POLICY, UE_FLAG_CTRL_TX_POLICY_NEXT_PACKET);

    _io_impl = UHD_PIMPL_MAKE(io_impl, (_iface));

    //spawn a pirate, yarrr!
    _io_impl->recv_pirate_crew.create_thread(boost::bind(
        &usrp_e_impl::io_impl::recv_pirate_loop, _io_impl.get()
    ));
}

void usrp_e_impl::issue_stream_cmd(const stream_cmd_t &stream_cmd){
    _io_impl->continuous_streaming = (stream_cmd.stream_mode == stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    _iface->poke32(UE_REG_CTRL_RX_STREAM_CMD, dsp_type1::calc_stream_cmd_word(
        stream_cmd, get_max_recv_samps_per_packet()
    ));
    _iface->poke32(UE_REG_CTRL_RX_TIME_SECS,  boost::uint32_t(stream_cmd.time_spec.get_full_secs()));
    _iface->poke32(UE_REG_CTRL_RX_TIME_TICKS, stream_cmd.time_spec.get_tick_count(MASTER_CLOCK_RATE));
}

void usrp_e_impl::handle_overrun(size_t){
    std::cerr << "O"; //the famous OOOOOOOOOOO
    _iface->poke32(UE_REG_CTRL_RX_CLEAR_OVERRUN, 0);
    if (_io_impl->continuous_streaming){
        this->issue_stream_cmd(stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    }
}

/***********************************************************************
 * Data Send
 **********************************************************************/
bool get_send_buffs(
    zero_copy_if::sptr trans, double timeout,
    vrt_packet_handler::managed_send_buffs_t &buffs
){
    UHD_ASSERT_THROW(buffs.size() == 1);
    buffs[0] = trans->get_send_buff(timeout);
    return buffs[0].get() != NULL;
}

#if 0
size_t usrp_e_impl::get_max_send_samps_per_packet(void) const{
    static const size_t hdr_size = 0
        + vrt::max_if_hdr_words32*sizeof(boost::uint32_t)
        - sizeof(vrt::if_packet_info_t().cid) //no class id ever used
    ;
    size_t bpp = _io_impl->data_xport->get_send_frame_size() - hdr_size;
    return bpp/_send_otw_type.get_sample_size();
}
#endif

size_t usrp_e_impl::send(
    const std::vector<const void *> &buffs, size_t num_samps,
    const tx_metadata_t &metadata, const io_type_t &io_type,
    send_mode_t send_mode, double timeout
){
    return vrt_packet_handler::send(
        _io_impl->packet_handler_send_state,       //last state of the send handler
        buffs, num_samps,                          //buffer to fill
        metadata, send_mode,                       //samples metadata
        io_type, _send_otw_type,                   //input and output types to convert
        MASTER_CLOCK_RATE,                         //master clock tick rate
        uhd::transport::vrt::if_hdr_pack_le,
        boost::bind(&get_send_buffs, _io_impl->data_xport, timeout, _1),
        get_max_send_samps_per_packet()
    );
}

/***********************************************************************
 * Data Recv
 **********************************************************************/
#if 0
size_t usrp_e_impl::get_max_recv_samps_per_packet(void) const{
    static const size_t hdr_size = 0
        + vrt::max_if_hdr_words32*sizeof(boost::uint32_t)
        + sizeof(vrt::if_packet_info_t().tlr) //forced to have trailer
        - sizeof(vrt::if_packet_info_t().cid) //no class id ever used
    ;
    size_t bpp = _io_impl->data_xport->get_recv_frame_size() - hdr_size;
    return bpp/_recv_otw_type.get_sample_size();
}
#endif

size_t usrp_e_impl::recv(
    const std::vector<void *> &buffs, size_t num_samps,
    rx_metadata_t &metadata, const io_type_t &io_type,
    recv_mode_t recv_mode, double timeout
){
    return vrt_packet_handler::recv(
        _io_impl->packet_handler_recv_state,       //last state of the recv handler
        buffs, num_samps,                          //buffer to fill
        metadata, recv_mode,                       //samples metadata
        io_type, _recv_otw_type,                   //input and output types to convert
        MASTER_CLOCK_RATE,                         //master clock tick rate
        uhd::transport::vrt::if_hdr_unpack_le,
        boost::bind(&usrp_e_impl::io_impl::get_recv_buffs, _io_impl.get(), _1, timeout),
        boost::bind(&usrp_e_impl::handle_overrun, this, _1)
    );
}

/***********************************************************************
 * Async Recv
 **********************************************************************/
bool usrp_e_impl::recv_async_msg(
    async_metadata_t &async_metadata, double timeout
){
    boost::this_thread::disable_interruption di; //disable because the wait can throw
    return _io_impl->async_msg_fifo->pop_with_timed_wait(async_metadata, timeout);
}
