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
#include <linux/usrp_e.h> //ioctl structures and constants
#include "e100_impl.hpp"
#include "e100_regs.hpp"
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
#include <poll.h> //poll
#include <fcntl.h> //open, close
#include <sstream>
#include <fstream>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;

/***********************************************************************
 * io impl details (internal to this file)
 * - pirate crew of 1
 * - bounded buffer
 * - thread loop
 * - vrt packet handler states
 **********************************************************************/
struct e100_impl::io_impl{
    io_impl(zero_copy_if::sptr &xport):
        data_transport(xport),
        async_msg_fifo(100/*messages deep*/)
    {
        for (size_t i = 0; i < E100_NUM_RX_DSPS; i++){
            typedef bounded_buffer<managed_recv_buffer::sptr> buffs_queue_type;
            _buffs_queue.push_back(new buffs_queue_type(data_transport->get_num_recv_frames()));
        }
    }

    ~io_impl(void){
        recv_pirate_crew.interrupt_all();
        recv_pirate_crew.join_all();
        for (size_t i = 0; i < _buffs_queue.size(); i++){
            delete _buffs_queue[i];
        }
    }

    std::vector<bounded_buffer<managed_recv_buffer::sptr> *> _buffs_queue;

    //gets buffer, determines if its the requested index,
    //and either queues the buffer or returns the buffer
    managed_recv_buffer::sptr get_recv_buff(const size_t index, const double timeout){
        while (true){
            managed_recv_buffer::sptr buff;

            //attempt to pop a buffer from the queue
            if (_buffs_queue[index]->pop_with_haste(buff)) return buff;

            //otherwise, call into the transport
            buff = data_transport->get_recv_buff(timeout);
            if (buff.get() == NULL) return buff; //timeout

            //check the stream id to know which channel
            const boost::uint32_t *vrt_hdr = buff->cast<const boost::uint32_t *>();
            const size_t rx_index = uhd::wtohx(vrt_hdr[1]) - E100_DSP_SID_BASE;
            if (rx_index == index) return buff; //got expected message

            //otherwise queue and try again
            if (rx_index < E100_NUM_RX_DSPS) _buffs_queue[rx_index]->push_with_pop_on_full(buff);
            else UHD_MSG(error) << "Got a data packet with known SID " << uhd::wtohx(vrt_hdr[1]) << std::endl;
        }
    }

    //The data transport is listed first so that it is deconstructed last,
    //which is after the states and booty which may hold managed buffers.
    zero_copy_if::sptr data_transport;

    //state management for the vrt packet handler code
    sph::recv_packet_handler recv_handler;
    sph::send_packet_handler send_handler;
    bool continuous_streaming;

    //a pirate's life is the life for me!
    void recv_pirate_loop(
        boost::barrier &spawn_barrier,
        const boost::function<void(void)> &handle,
        e100_iface::sptr //keep a sptr to iface which shares gpio147
    ){
        spawn_barrier.wait();

        //open the GPIO and set it up for an IRQ
        std::ofstream edge_file("/sys/class/gpio/gpio147/edge");
        edge_file << "rising" << std::endl << std::flush;
        edge_file.close();
        int fd = ::open("/sys/class/gpio/gpio147/value", O_RDONLY);
        if (fd < 0) UHD_MSG(error) << "Unable to open GPIO for IRQ\n";

        while (not boost::this_thread::interruption_requested()){
            pollfd pfd;
            pfd.fd = fd;
            pfd.events = POLLPRI | POLLERR;
            ssize_t ret = ::poll(&pfd, 1, 100/*ms*/);
            if (ret > 0) handle();
        }

        //cleanup before thread exit
        ::close(fd);
    }
    bounded_buffer<async_metadata_t> async_msg_fifo;
    boost::thread_group recv_pirate_crew;
};

/***********************************************************************
 * Helper Functions
 **********************************************************************/
void e100_impl::io_init(void){

    //setup before the registers (transport called to calculate max spp)
    _io_impl = UHD_PIMPL_MAKE(io_impl, (_data_transport));

    //clear state machines
    _iface->poke32(E100_REG_CLEAR_RX, 0);
    _iface->poke32(E100_REG_CLEAR_TX, 0);

    //prepare the async msg buffer for incoming messages
    _iface->poke32(E100_REG_SR_ERR_CTRL, 1 << 0); //clear
    while ((_iface->peek32(E100_REG_RB_ERR_STATUS) & (1 << 2)) == 0){} //wait for idle
    _iface->poke32(E100_REG_SR_ERR_CTRL, 1 << 1); //start

    //spawn a pirate, yarrr!
    boost::barrier spawn_barrier(2);
    boost::function<void(void)> handle_irq_cb = boost::bind(&e100_impl::handle_irq, this);
    _io_impl->recv_pirate_crew.create_thread(boost::bind(
        &e100_impl::io_impl::recv_pirate_loop, _io_impl.get(),
        boost::ref(spawn_barrier), handle_irq_cb, _iface
    ));
    spawn_barrier.wait();
    //update mapping here since it didnt b4 when io init not called first
    update_xport_channel_mapping();
}

void e100_impl::handle_irq(void){
    //check the status of the async msg buffer
    const boost::uint32_t status = _iface->peek32(E100_REG_RB_ERR_STATUS);
    if ((status & 0x3) == 0) return; //not done or error
    //std::cout << boost::format("status: 0x%x") % status << std::endl;

    //load the data struct and call the ioctl
    usrp_e_ctl32 data;
    data.offset = E100_REG_ERR_BUFF;
    data.count = status >> 16;
    //FIXME ioctl reads words32 incorrectly _iface->ioctl(USRP_E_READ_CTL32, &data);
    for (size_t i = 0; i < data.count; i++){
        data.buf[i] = _iface->peek32(E100_REG_ERR_BUFF + i*sizeof(boost::uint32_t));
        //std::cout << boost::format("    buff[%u] = 0x%08x\n") % i % data.buf[i];
    }

    //unpack the vrt header and process below...
    vrt::if_packet_info_t if_packet_info;
    if_packet_info.num_packet_words32 = data.count;
    try{vrt::if_hdr_unpack_le(data.buf, if_packet_info);}
    catch(const std::exception &e){
        UHD_MSG(error) << "Error unpacking vrt header:\n" << e.what() << std::endl;
        goto prepare;
    }

    //handle a tx async report message
    if (if_packet_info.sid == E100_ASYNC_SID and if_packet_info.packet_type != vrt::if_packet_info_t::PACKET_TYPE_DATA){

        //fill in the async metadata
        async_metadata_t metadata;
        metadata.channel = 0;
        metadata.has_time_spec = if_packet_info.has_tsi and if_packet_info.has_tsf;
        metadata.time_spec = time_spec_t(
            time_t(if_packet_info.tsi), long(if_packet_info.tsf), _clock_ctrl->get_fpga_clock_rate()
        );
        metadata.event_code = async_metadata_t::event_code_t(sph::get_context_code(data.buf, if_packet_info));

        //push the message onto the queue
        _io_impl->async_msg_fifo.push_with_pop_on_full(metadata);

        //print some fastpath messages
        if (metadata.event_code &
            ( async_metadata_t::EVENT_CODE_UNDERFLOW
            | async_metadata_t::EVENT_CODE_UNDERFLOW_IN_PACKET)
        ) UHD_MSG(fastpath) << "U";
        else if (metadata.event_code &
            ( async_metadata_t::EVENT_CODE_SEQ_ERROR
            | async_metadata_t::EVENT_CODE_SEQ_ERROR_IN_BURST)
        ) UHD_MSG(fastpath) << "S";
    }

    //prepare for the next round
    prepare:
    _iface->poke32(E100_REG_SR_ERR_CTRL, 1 << 0); //clear
    while ((_iface->peek32(E100_REG_RB_ERR_STATUS) & (1 << 2)) == 0){} //wait for idle
    _iface->poke32(E100_REG_SR_ERR_CTRL, 1 << 1); //start
}

void e100_impl::update_xport_channel_mapping(void){
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
            &e100_impl::io_impl::get_recv_buff, _io_impl.get(), chan, _1
        ));
        _io_impl->recv_handler.set_overflow_handler(chan, boost::bind(
            &e100_impl::handle_overrun, this, chan
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
            &uhd::transport::zero_copy_if::get_send_buff, _io_impl->data_transport, _1
        ));
    }
    _io_impl->send_handler.set_converter(_send_otw_type);
    _io_impl->send_handler.set_max_samples_per_packet(get_max_send_samps_per_packet());
}

void e100_impl::issue_ddc_stream_cmd(const stream_cmd_t &stream_cmd, const size_t index){
    _io_impl->continuous_streaming = (stream_cmd.stream_mode == stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    _iface->poke32(E100_REG_RX_CTRL_STREAM_CMD(index), dsp_type1::calc_stream_cmd_word(stream_cmd));
    _iface->poke32(E100_REG_RX_CTRL_TIME_SECS(index),  boost::uint32_t(stream_cmd.time_spec.get_full_secs()));
    _iface->poke32(E100_REG_RX_CTRL_TIME_TICKS(index), stream_cmd.time_spec.get_tick_count(_clock_ctrl->get_fpga_clock_rate()));
}

void e100_impl::handle_overrun(const size_t index){
    if (_io_impl->continuous_streaming){
        this->issue_ddc_stream_cmd(stream_cmd_t::STREAM_MODE_START_CONTINUOUS, index);
    }
}

/***********************************************************************
 * Data Send
 **********************************************************************/
size_t e100_impl::get_max_send_samps_per_packet(void) const{
    static const size_t hdr_size = 0
        + vrt::max_if_hdr_words32*sizeof(boost::uint32_t)
        - sizeof(vrt::if_packet_info_t().cid) //no class id ever used
    ;
    size_t bpp = _send_frame_size - hdr_size;
    return bpp/_send_otw_type.get_sample_size();
}

size_t e100_impl::send(
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
size_t e100_impl::get_max_recv_samps_per_packet(void) const{
    static const size_t hdr_size = 0
        + vrt::max_if_hdr_words32*sizeof(boost::uint32_t)
        + sizeof(vrt::if_packet_info_t().tlr) //forced to have trailer
        - sizeof(vrt::if_packet_info_t().cid) //no class id ever used
    ;
    size_t bpp = _recv_frame_size - hdr_size;
    return bpp/_recv_otw_type.get_sample_size();
}

size_t e100_impl::recv(
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
bool e100_impl::recv_async_msg(
    async_metadata_t &async_metadata, double timeout
){
    boost::this_thread::disable_interruption di; //disable because the wait can throw
    return _io_impl->async_msg_fifo.pop_with_timed_wait(async_metadata, timeout);
}
