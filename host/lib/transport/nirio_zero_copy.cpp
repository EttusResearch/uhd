//
// Copyright 2013-2015 Ettus Research LLC
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

#include <uhd/transport/nirio_zero_copy.hpp>
#include <stdio.h>
#include <uhd/transport/nirio/nirio_fifo.h>
#include <uhd/transport/buffer_pool.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/atomic.hpp>
#include <boost/format.hpp>
#include <boost/make_shared.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp> //sleep
#include <vector>
#include <algorithm>    // std::max
//@TODO: Move the register defs required by the class to a common location
#include "../usrp/x300/x300_regs.hpp"

using namespace uhd;
using namespace uhd::transport;
using namespace uhd::niusrprio;

typedef uint64_t fifo_data_t;

class nirio_zero_copy_mrb : public managed_recv_buffer
{
public:
    nirio_zero_copy_mrb(nirio_fifo<fifo_data_t>& fifo, const size_t frame_size):
        _fifo(fifo), _frame_size(frame_size) { }

    void release(void)
    {
        _fifo.release(_frame_size / sizeof(fifo_data_t));
    }

    UHD_INLINE sptr get_new(const double timeout, size_t &index)
    {
        nirio_status status = 0;
        size_t elems_acquired = 0;
        size_t elems_remaining = 0;
        nirio_status_chain(_fifo.acquire(
            _typed_buffer, _frame_size / sizeof(fifo_data_t),
            static_cast<uint32_t>(timeout*1000),
            elems_acquired, elems_remaining), status);
        _length = elems_acquired * sizeof(fifo_data_t);
        _buffer = static_cast<void*>(_typed_buffer);

        if (nirio_status_not_fatal(status)) {
            index++;        //Advances the caller's buffer
            return make(this, _buffer, _length);
        } else if (status == NiRio_Status_CommunicationTimeout) {
            nirio_status_to_exception(status, "NI-RIO PCIe data transfer failed.");
            return sptr();
        } else {
            return sptr();  //NULL for timeout or error.
        }
    }

private:
    nirio_fifo<fifo_data_t>&    _fifo;
    fifo_data_t*                _typed_buffer;
    const size_t                _frame_size;
    size_t                      _num_frames;
};

class nirio_zero_copy_msb : public managed_send_buffer
{
public:
    nirio_zero_copy_msb(nirio_fifo<fifo_data_t>& fifo, const size_t frame_size):
        _fifo(fifo), _frame_size(frame_size) { }

    void release(void)
    {
        _fifo.release(_frame_size / sizeof(fifo_data_t));
    }

    UHD_INLINE sptr get_new(const double timeout, size_t &index)
    {
        nirio_status status = 0;
        size_t elems_acquired = 0;
        size_t elems_remaining = 0;
        nirio_status_chain(_fifo.acquire(
            _typed_buffer, _frame_size / sizeof(fifo_data_t),
            static_cast<uint32_t>(timeout*1000),
            elems_acquired, elems_remaining), status);
        _length = elems_acquired * sizeof(fifo_data_t);
        _buffer = static_cast<void*>(_typed_buffer);

        if (nirio_status_not_fatal(status)) {
            index++;        //Advances the caller's buffer
            return make(this, _buffer, _length);
        } else if (status == NiRio_Status_CommunicationTimeout) {
            nirio_status_to_exception(status, "NI-RIO PCIe data transfer failed.");
            return sptr();
        } else {
            return sptr();  //NULL for timeout or error.
        }
    }

private:
    nirio_fifo<fifo_data_t>&    _fifo;
    fifo_data_t*                _typed_buffer;
    const size_t                _frame_size;
    size_t                      _num_frames;
};

class nirio_zero_copy_impl : public nirio_zero_copy {
public:
    typedef boost::shared_ptr<nirio_zero_copy_impl> sptr;

    nirio_zero_copy_impl(
        uhd::niusrprio::niusrprio_session::sptr fpga_session,
        uint32_t instance,
        const zero_copy_xport_params& xport_params
    ):
        _fpga_session(fpga_session),
        _fifo_instance(instance),
        _xport_params(xport_params),
        _next_recv_buff_index(0), _next_send_buff_index(0)
    {
        UHD_LOG << boost::format("Creating PCIe transport for channel %d") % instance << std::endl;
        UHD_LOG << boost::format("nirio zero-copy RX transport configured with frame size = %u, #frames = %u, buffer size = %u\n")
                    % _xport_params.recv_frame_size % _xport_params.num_recv_frames %
                    (_xport_params.recv_frame_size * _xport_params.num_recv_frames);
        UHD_LOG << boost::format("nirio zero-copy TX transport configured with frame size = %u, #frames = %u, buffer size = %u\n")
                    % _xport_params.send_frame_size % _xport_params.num_send_frames % (_xport_params.send_frame_size * _xport_params.num_send_frames);

        _recv_buffer_pool = buffer_pool::make(_xport_params.num_recv_frames, _xport_params.recv_frame_size);
        _send_buffer_pool = buffer_pool::make(_xport_params.num_send_frames, _xport_params.send_frame_size);

        nirio_status status = 0;
        size_t actual_depth = 0, actual_size = 0;

        //Disable DMA streams in case last shutdown was unclean (cleanup, so don't status chain)
        _proxy()->poke(PCIE_TX_DMA_REG(DMA_CTRL_STATUS_REG, _fifo_instance), DMA_CTRL_DISABLED);
        _proxy()->poke(PCIE_RX_DMA_REG(DMA_CTRL_STATUS_REG, _fifo_instance), DMA_CTRL_DISABLED);

        _wait_until_stream_ready();

        //Configure frame width
        nirio_status_chain(
            _proxy()->poke(PCIE_TX_DMA_REG(DMA_FRAME_SIZE_REG, _fifo_instance),
                          static_cast<uint32_t>(_xport_params.send_frame_size/sizeof(fifo_data_t))),
            status);
        nirio_status_chain(
            _proxy()->poke(PCIE_RX_DMA_REG(DMA_FRAME_SIZE_REG, _fifo_instance),
                          static_cast<uint32_t>(_xport_params.recv_frame_size/sizeof(fifo_data_t))),
            status);
        //Config 32-bit word flipping and enable DMA streams
        nirio_status_chain(
            _proxy()->poke(PCIE_TX_DMA_REG(DMA_CTRL_STATUS_REG, _fifo_instance),
                          DMA_CTRL_SW_BUF_U32 | DMA_CTRL_ENABLED),
            status);
        nirio_status_chain(
            _proxy()->poke(PCIE_RX_DMA_REG(DMA_CTRL_STATUS_REG, _fifo_instance),
                          DMA_CTRL_SW_BUF_U32 | DMA_CTRL_ENABLED),
            status);

        //Create FIFOs
        nirio_status_chain(
            _fpga_session->create_rx_fifo(_fifo_instance, _recv_fifo),
            status);
        nirio_status_chain(
            _fpga_session->create_tx_fifo(_fifo_instance, _send_fifo),
            status);

        if ((_recv_fifo.get() != NULL) && (_send_fifo.get() != NULL)) {
            //Initialize FIFOs
            nirio_status_chain(
                _recv_fifo->initialize(
                    (_xport_params.recv_frame_size*_xport_params.num_recv_frames)/sizeof(fifo_data_t),
                    actual_depth, actual_size),
                status);
            nirio_status_chain(
                _send_fifo->initialize(
                    (_xport_params.send_frame_size*_xport_params.num_send_frames)/sizeof(fifo_data_t),
                    actual_depth, actual_size),
                status);

            _proxy()->get_rio_quirks().add_tx_fifo(_fifo_instance);

            nirio_status_chain(_recv_fifo->start(), status);
            nirio_status_chain(_send_fifo->start(), status);

            if (nirio_status_not_fatal(status)) {
                //allocate re-usable managed receive buffers
                for (size_t i = 0; i < get_num_recv_frames(); i++){
                    _mrb_pool.push_back(boost::shared_ptr<nirio_zero_copy_mrb>(new nirio_zero_copy_mrb(
                        *_recv_fifo, get_recv_frame_size())));
                }

                //allocate re-usable managed send buffers
                for (size_t i = 0; i < get_num_send_frames(); i++){
                    _msb_pool.push_back(boost::shared_ptr<nirio_zero_copy_msb>(new nirio_zero_copy_msb(
                        *_send_fifo, get_send_frame_size())));
                }
            }
        } else {
            nirio_status_chain(NiRio_Status_ResourceNotInitialized, status);
        }

        nirio_status_to_exception(status, "Could not create nirio_zero_copy transport.");
    }

    virtual ~nirio_zero_copy_impl()
    {
        _proxy()->get_rio_quirks().remove_tx_fifo(_fifo_instance);

        //Disable DMA streams (cleanup, so don't status chain)
        _proxy()->poke(PCIE_TX_DMA_REG(DMA_CTRL_STATUS_REG, _fifo_instance), DMA_CTRL_DISABLED);
        _proxy()->poke(PCIE_RX_DMA_REG(DMA_CTRL_STATUS_REG, _fifo_instance), DMA_CTRL_DISABLED);

        _flush_rx_buff();

        //Stop DMA channels. Stop is called in the fifo dtor but
        //it doesn't hurt to do it here.
        _send_fifo->stop();
        _recv_fifo->stop();
    }

    /*******************************************************************
     * Receive implementation:
     * Block on the managed buffer's get call and advance the index.
     ******************************************************************/
    managed_recv_buffer::sptr get_recv_buff(double timeout)
    {
        if (_next_recv_buff_index == _xport_params.num_recv_frames) _next_recv_buff_index = 0;
        return _mrb_pool[_next_recv_buff_index]->get_new(timeout, _next_recv_buff_index);
    }

    size_t get_num_recv_frames(void) const {return _xport_params.num_recv_frames;}
    size_t get_recv_frame_size(void) const {return _xport_params.recv_frame_size;}

    /*******************************************************************
     * Send implementation:
     * Block on the managed buffer's get call and advance the index.
     ******************************************************************/
    managed_send_buffer::sptr get_send_buff(double timeout)
    {
        if (_next_send_buff_index == _xport_params.num_send_frames) _next_send_buff_index = 0;
        return _msb_pool[_next_send_buff_index]->get_new(timeout, _next_send_buff_index);
    }

    size_t get_num_send_frames(void) const {return _xport_params.num_send_frames;}
    size_t get_send_frame_size(void) const {return _xport_params.send_frame_size;}

private:

    UHD_INLINE niriok_proxy::sptr _proxy() { return _fpga_session->get_kernel_proxy(); }

    UHD_INLINE void _flush_rx_buff()
    {
        // acquire is called with 0 elements requested first to
        // get the number of elements in the buffer and then
        // repeatedly with the number of remaining elements
        // until the buffer is empty
        for (size_t num_elems_requested = 0,
            num_elems_acquired = 0,
            num_elems_remaining = 1;
            num_elems_remaining;
            num_elems_requested = num_elems_remaining)
        {
            fifo_data_t* elems_buffer = NULL;
            nirio_status status = _recv_fifo->acquire(
                elems_buffer,
                num_elems_requested,
                0,                      // timeout
                num_elems_acquired,
                num_elems_remaining);
            // throw excetption if status is fatal
            nirio_status_to_exception(status,
                "NI-RIO PCIe data transfer failed during flush.");
            _recv_fifo->release(num_elems_acquired);
        }
    }

    UHD_INLINE void _wait_until_stream_ready()
    {
        static const uint32_t TIMEOUT_IN_MS = 100;

        uint32_t reg_data = 0xffffffff;
        bool tx_busy = true, rx_busy = true;
        boost::posix_time::ptime start_time;
        boost::posix_time::time_duration elapsed;
        nirio_status status = NiRio_Status_Success;

        nirio_status_chain(_proxy()->peek(
            PCIE_TX_DMA_REG(DMA_CTRL_STATUS_REG, _fifo_instance), reg_data), status);
        tx_busy = (reg_data & DMA_STATUS_BUSY) > 0;
        nirio_status_chain(_proxy()->peek(
            PCIE_RX_DMA_REG(DMA_CTRL_STATUS_REG, _fifo_instance), reg_data), status);
        rx_busy = (reg_data & DMA_STATUS_BUSY) > 0;

        if (nirio_status_not_fatal(status) && (tx_busy || rx_busy)) {
            start_time = boost::posix_time::microsec_clock::local_time();
            do {
                boost::this_thread::sleep(boost::posix_time::microsec(50)); //Avoid flooding the bus
                elapsed = boost::posix_time::microsec_clock::local_time() - start_time;
                nirio_status_chain(_proxy()->peek(
                    PCIE_TX_DMA_REG(DMA_CTRL_STATUS_REG, _fifo_instance), reg_data), status);
                tx_busy = (reg_data & DMA_STATUS_BUSY) > 0;
                nirio_status_chain(_proxy()->peek(
                    PCIE_RX_DMA_REG(DMA_CTRL_STATUS_REG, _fifo_instance), reg_data), status);
                rx_busy = (reg_data & DMA_STATUS_BUSY) > 0;
            } while (
                nirio_status_not_fatal(status) &&
                (tx_busy || rx_busy) &&
                elapsed.total_milliseconds() < TIMEOUT_IN_MS);

            if (tx_busy || rx_busy) {
                nirio_status_chain(NiRio_Status_FpgaBusy, status);
            }

            nirio_status_to_exception(status, "Could not create nirio_zero_copy transport.");
        }
    }

    //memory management -> buffers and fifos
    niusrprio::niusrprio_session::sptr _fpga_session;
    uint32_t _fifo_instance;
    nirio_fifo<fifo_data_t>::sptr _recv_fifo, _send_fifo;
    const zero_copy_xport_params _xport_params;
    buffer_pool::sptr _recv_buffer_pool, _send_buffer_pool;
    std::vector<boost::shared_ptr<nirio_zero_copy_msb> > _msb_pool;
    std::vector<boost::shared_ptr<nirio_zero_copy_mrb> > _mrb_pool;
    size_t _next_recv_buff_index, _next_send_buff_index;
};


nirio_zero_copy::sptr nirio_zero_copy::make(
    uhd::niusrprio::niusrprio_session::sptr fpga_session,
    const uint32_t instance,
    const zero_copy_xport_params& default_buff_args,
    const device_addr_t &hints
){
    //Initialize xport_params
    zero_copy_xport_params xport_params = default_buff_args;

    //The kernel buffer for this transport must be (num_frames * frame_size) big. Unlike ethernet,
    //where the kernel buffer size is independent of the circular buffer size for the transport,
    //it is possible for users to over constrain the system when they set the num_frames and the buff_size
    //So we give buff_size priority over num_frames and throw an error if they conflict.

    //RX
    xport_params.recv_frame_size = size_t(hints.cast<double>("recv_frame_size", default_buff_args.recv_frame_size));

    size_t usr_num_recv_frames = static_cast<size_t>(
        hints.cast<double>("num_recv_frames", default_buff_args.num_recv_frames));
    size_t usr_recv_buff_size = static_cast<size_t>(
        hints.cast<double>("recv_buff_size", default_buff_args.num_recv_frames));

    if (hints.has_key("num_recv_frames") and hints.has_key("recv_buff_size")) {
        if (usr_recv_buff_size < xport_params.recv_frame_size)
            throw uhd::value_error("recv_buff_size must be equal to or greater than (num_recv_frames * recv_frame_size)");

        if ((usr_recv_buff_size/xport_params.recv_frame_size) != usr_num_recv_frames)
            throw uhd::value_error("Conflicting values for recv_buff_size and num_recv_frames");
    }

    if (hints.has_key("recv_buff_size")) {
        xport_params.num_recv_frames = std::max<size_t>(1, usr_recv_buff_size/xport_params.recv_frame_size);    //Round down
    } else if (hints.has_key("num_recv_frames")) {
        xport_params.num_recv_frames = usr_num_recv_frames;
    }

    //TX
    xport_params.send_frame_size = size_t(hints.cast<double>("send_frame_size", default_buff_args.send_frame_size));

    size_t usr_num_send_frames = static_cast<size_t>(
        hints.cast<double>("num_send_frames", default_buff_args.num_send_frames));
    size_t usr_send_buff_size = static_cast<size_t>(
        hints.cast<double>("send_buff_size", default_buff_args.num_send_frames));

    if (hints.has_key("num_send_frames") and hints.has_key("send_buff_size")) {
        if (usr_send_buff_size < xport_params.send_frame_size)
            throw uhd::value_error("send_buff_size must be equal to or greater than (num_send_frames * send_frame_size)");

        if ((usr_send_buff_size/xport_params.send_frame_size) != usr_num_send_frames)
            throw uhd::value_error("Conflicting values for send_buff_size and num_send_frames");
    }

    if (hints.has_key("send_buff_size")) {
        xport_params.num_send_frames = std::max<size_t>(1, usr_send_buff_size/xport_params.send_frame_size);    //Round down
    } else if (hints.has_key("num_send_frames")) {
        xport_params.num_send_frames = usr_num_send_frames;
    }

    return nirio_zero_copy::sptr(new nirio_zero_copy_impl(fpga_session, instance, xport_params));
}

