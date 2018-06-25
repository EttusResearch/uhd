//
// Copyright 2017 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "liberio_zero_copy.hpp"
#include <uhd/config.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/log.hpp>
#include <liberio/liberio.h>
#include <boost/make_shared.hpp>
#include <sys/syslog.h>
#include <mutex>

namespace uhd { namespace transport {

static const uint64_t USEC = 1000000;

static void liberio_log_cb(int severity, const char *msg, void *)
{
    switch (severity) {
    case LOG_WARNING:
        UHD_LOG_WARNING("LIBERIO", msg);
        return;
    case LOG_NOTICE:
    case LOG_INFO:
        UHD_LOG_INFO("LIBERIO", msg);
        return;
    default:
        UHD_LOG_INFO("LIBERIO", msg);
    };
}

class liberio_context_holder {
public:
    liberio_context_holder(void)
    {
        _ctx = liberio_ctx_new();
        liberio_ctx_register_logger(_ctx, &liberio_log_cb, nullptr);
    }

    ~liberio_context_holder(void) { liberio_ctx_put(_ctx); }

    liberio_ctx* get(void)
    {
        liberio_ctx_get(_ctx);
        return _ctx;
    }
private:
    liberio_ctx *_ctx;
};

UHD_SINGLETON_FCN(liberio_context_holder, get_liberio_context_holder);

class liberio_zero_copy_msb : public virtual managed_send_buffer {
public:
    liberio_zero_copy_msb(liberio_chan *chan) : _chan(chan), _buf(nullptr) {}
    ~liberio_zero_copy_msb(void)
    {
        liberio_chan_put(_chan);
    }

    void release(void)
    {
        if (_buf) {
            liberio_buf_set_payload(_buf, 0, _length);
            liberio_chan_buf_enqueue(_chan, _buf);
        }
    }

    sptr get_new(double timeout, size_t &index)
    {
        _buf = liberio_chan_buf_dequeue(_chan, timeout * USEC);
        if (!_buf)
            return sptr();

        index++;

        return make(this, liberio_buf_get_mem(_buf, 0),
                    liberio_buf_get_len(_buf, 0));
    }

private:
    liberio_chan *_chan;
    liberio_buf *_buf;
};

class liberio_zero_copy_mrb : public virtual managed_recv_buffer {
public:
    liberio_zero_copy_mrb(liberio_chan *chan) : _chan(chan), _buf(nullptr) {}
    ~liberio_zero_copy_mrb(void)
    {
        liberio_chan_put(_chan);
    }

    void release(void)
    {
        if (_buf)
            liberio_chan_buf_enqueue(_chan, _buf);
    }

    sptr get_new(double timeout, size_t &index)
    {
        _buf = liberio_chan_buf_dequeue(_chan, timeout * USEC);
        if (!_buf)
            return sptr();

        index++;

        return make(this, liberio_buf_get_mem(_buf, 0),
                    liberio_buf_get_payload(_buf, 0));
    }

private:
    liberio_chan *_chan;
    liberio_buf *_buf;
};

class liberio_zero_copy_impl : public liberio_zero_copy {
public:

    liberio_zero_copy_impl(const std::string &tx_path,
                           const std::string &rx_path,
                           const zero_copy_xport_params& xport_params)
                          : _tx_buf_size(xport_params.send_frame_size),
                            _rx_buf_size(xport_params.recv_frame_size),
                            _next_recv_buff_index(0),
                            _next_send_buff_index(0)
    {
        UHD_ASSERT_THROW(xport_params.recv_frame_size > 0);
        UHD_ASSERT_THROW(xport_params.send_frame_size > 0);
        UHD_ASSERT_THROW(xport_params.num_send_frames > 0);
        UHD_ASSERT_THROW(xport_params.num_recv_frames > 0);

        liberio_ctx *ctx = get_liberio_context_holder().get();

        /* we hold a reference, that we'd drop immediately after,
         * so no requirement to get another one here ...
         */
        _tx_chan = liberio_ctx_alloc_chan(ctx, tx_path.c_str(), TX,
                           USRP_MEMORY_MMAP);

        UHD_ASSERT_THROW(_tx_chan);
        liberio_chan_stop_streaming(_tx_chan);
        liberio_chan_request_buffers(_tx_chan, 0);
        UHD_ASSERT_THROW(
            !liberio_chan_set_fixed_size(_tx_chan, 0,
                xport_params.send_frame_size
            )
        );
        UHD_ASSERT_THROW(
            !liberio_chan_request_buffers(
                _tx_chan, xport_params.num_send_frames
            )
        );
        _num_send_bufs = liberio_chan_get_num_bufs(_tx_chan);

        for (size_t i = 0; i < xport_params.num_send_frames; i++) {
            liberio_chan_get(_tx_chan);
            _msb_pool.push_back(
                boost::make_shared<liberio_zero_copy_msb>(_tx_chan));
        }

        /* we hold a reference, that we'd drop immediately after,
         * so no requirement to get another one here ...
         */
        _rx_chan = liberio_ctx_alloc_chan(ctx, rx_path.c_str(),
                          RX, USRP_MEMORY_MMAP);
        UHD_ASSERT_THROW(_rx_chan);

        /* done with the local reference, the channel keeps its own */
        liberio_ctx_put(ctx);

        /* stop the channel, free the buffers, set the size, allocate */
        liberio_chan_stop_streaming(_rx_chan);
        liberio_chan_request_buffers(_rx_chan, 0);
        UHD_ASSERT_THROW(
            !liberio_chan_set_fixed_size(_rx_chan, 0,
                xport_params.recv_frame_size
            )
        );
        UHD_ASSERT_THROW(!liberio_chan_request_buffers(
                _rx_chan, xport_params.num_recv_frames));
        _num_recv_bufs = liberio_chan_get_num_bufs(_rx_chan);

        for (size_t i = 0; i < xport_params.num_recv_frames; i++) {
            liberio_chan_get(_rx_chan);
            _mrb_pool.push_back(
                boost::make_shared<liberio_zero_copy_mrb>(_rx_chan));
        }

        UHD_ASSERT_THROW(!liberio_chan_enqueue_all(_rx_chan));

        liberio_chan_start_streaming(_rx_chan);
        liberio_chan_start_streaming(_tx_chan);
    }

    ~liberio_zero_copy_impl(void)
    {
        liberio_chan_put(_tx_chan);
        liberio_chan_put(_rx_chan);
    }

    managed_recv_buffer::sptr get_recv_buff(double timeout = 0.1)
    {
        std::lock_guard<std::mutex> lock(_rx_mutex);
        if (_next_recv_buff_index == _num_recv_bufs)
            _next_recv_buff_index = 0;
        return _mrb_pool[_next_recv_buff_index]->get_new(
            timeout, _next_recv_buff_index);
    }

    size_t get_num_recv_frames(void) const
    {
        return liberio_chan_get_num_bufs(_rx_chan);
    }

    size_t get_recv_frame_size(void) const
    {
        return _rx_buf_size;
    }

    managed_send_buffer::sptr get_send_buff(double timeout = 0.1)
    { 
        std::lock_guard<std::mutex> lock(_tx_mutex);
        if (_next_send_buff_index == _num_send_bufs)
            _next_send_buff_index = 0;
        return _msb_pool[_next_send_buff_index]->get_new(
            timeout, _next_send_buff_index);
    }

    size_t get_num_send_frames(void) const
    {
        return liberio_chan_get_num_bufs(_tx_chan);
    }

    size_t get_send_frame_size(void) const
    {
        return _tx_buf_size;
    }

private:
    liberio_chan *_tx_chan;
    const size_t _tx_buf_size;
    size_t _num_send_bufs;
    liberio_chan *_rx_chan;
    const size_t _rx_buf_size;
    size_t _num_recv_bufs;

    std::vector<boost::shared_ptr<liberio_zero_copy_mrb> > _mrb_pool;
    size_t _next_recv_buff_index;
    std::vector<boost::shared_ptr<liberio_zero_copy_msb> > _msb_pool;
    size_t _next_send_buff_index;
    std::mutex _rx_mutex;
    std::mutex _tx_mutex;
};

liberio_zero_copy::sptr liberio_zero_copy::make(
    const std::string &tx_path,
    const std::string &rx_path,
    const zero_copy_xport_params &default_buff_args)
{
    return liberio_zero_copy::sptr(
        new liberio_zero_copy_impl(tx_path, rx_path, default_buff_args)
    );
}

}}
