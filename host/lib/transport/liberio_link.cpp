//
// Copyright 2019 Ettus Research, a National Instruments brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/config.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/static.hpp>
#include <uhdlib/transport/adapter.hpp>
#include <uhdlib/transport/liberio_link.hpp>
#include <sys/syslog.h>
#include <memory>

namespace uhd { namespace transport {

using liberio_ctx_sptr = std::shared_ptr<liberio_ctx>;

static const uint64_t USEC = 1000000;

static void liberio_log_cb(int severity, const char* msg, void*)
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

class liberio_context_holder
{
public:
    UHD_SINGLETON_FCN(liberio_context_holder, get);

    inline struct liberio_chan* alloc_tx_chan(const std::string& path)
    {
        return liberio_ctx_alloc_chan(_ctx, path.c_str(), TX, USRP_MEMORY_MMAP);
    }

    inline struct liberio_chan* alloc_rx_chan(const std::string& path)
    {
        return liberio_ctx_alloc_chan(_ctx, path.c_str(), RX, USRP_MEMORY_MMAP);
    }

private:
    liberio_context_holder(void)
    {
        _ctx = liberio_ctx_new();
        liberio_ctx_register_logger(_ctx, &liberio_log_cb, nullptr);
    }

    ~liberio_context_holder(void)
    {
        liberio_ctx_put(_ctx);
    }

    liberio_ctx* _ctx;
};

liberio_link::liberio_link(
    const std::string& tx_path, const std::string& rx_path, const link_params_t& params)
    : recv_link_base_t(params.num_recv_frames, params.recv_frame_size)
    , send_link_base_t(params.num_send_frames, params.send_frame_size)
{
    auto& ctx_holder = liberio_context_holder::get();

    /* Allocate TX channel (begins with refcount of 1)
     */
    _tx_chan = ctx_holder.alloc_tx_chan(tx_path);
    UHD_ASSERT_THROW(_tx_chan);

    /* Reset channel */
    liberio_chan_stop_streaming(_tx_chan);
    liberio_chan_request_buffers(_tx_chan, 0);
    UHD_ASSERT_THROW(!liberio_chan_set_fixed_size(_tx_chan, 0, params.send_frame_size));
    UHD_ASSERT_THROW(!liberio_chan_request_buffers(_tx_chan, params.num_send_frames));

    /* TODO: Check params in factory and adjust them there instead of throwing exception
     * here? */
    UHD_ASSERT_THROW(params.num_send_frames == liberio_chan_get_num_bufs(_tx_chan));
    for (size_t i = 0; i < params.num_send_frames; i++) {
        _send_buffs.push_back(liberio_frame_buff(_tx_chan));
    }

    /* Allocate RX channel (begins with refcount of 1)
     */
    _rx_chan = ctx_holder.alloc_rx_chan(rx_path);
    UHD_ASSERT_THROW(_rx_chan);

    /* stop the channel, free the buffers, set the size, allocate */
    liberio_chan_stop_streaming(_rx_chan);
    liberio_chan_request_buffers(_rx_chan, 0);
    UHD_ASSERT_THROW(!liberio_chan_set_fixed_size(_rx_chan, 0, params.recv_frame_size));
    UHD_ASSERT_THROW(!liberio_chan_request_buffers(_rx_chan, params.num_recv_frames));
    /* TODO: Check params in factory and adjust them there instead of throwing exception
     * here? */
    UHD_ASSERT_THROW(params.num_recv_frames == liberio_chan_get_num_bufs(_rx_chan));

    for (size_t i = 0; i < params.num_recv_frames; i++) {
        _recv_buffs.push_back(liberio_frame_buff(_rx_chan));
    }
    UHD_ASSERT_THROW(!liberio_chan_enqueue_all(_rx_chan));

    /* Start streaming on the devices */
    liberio_chan_start_streaming(_rx_chan);
    liberio_chan_start_streaming(_tx_chan);

    /* Finally, preload the buffer pools in parent class */
    for (auto& buff : _send_buffs) {
        send_link_base_t::preload_free_buff(&buff);
    }
    for (auto& buff : _recv_buffs) {
        recv_link_base_t::preload_free_buff(&buff);
    }

    auto info      = liberio_adapter_info();
    auto& adap_ctx = adapter_ctx::get();
    _adapter_id    = adap_ctx.register_adapter(info);

    UHD_LOGGER_TRACE("LIBERIO")
        << boost::format("Created liberio link (tx:%s, rx:%s)") % tx_path % rx_path;
    UHD_LOGGER_TRACE("LIBERIO")
        << boost::format("num_recv_frames=%d, recv_frame_size=%d, num_send_frames=%d, "
                         "send_frame_size=%d")
               % params.num_recv_frames % params.recv_frame_size % params.num_send_frames
               % params.send_frame_size;
}

liberio_link::~liberio_link()
{
    liberio_chan_put(_tx_chan);
    liberio_chan_put(_rx_chan);
}

liberio_link::sptr liberio_link::make(
    const std::string& tx_path, const std::string& rx_path, const link_params_t& params)
{
    UHD_ASSERT_THROW(params.recv_frame_size > 0);
    UHD_ASSERT_THROW(params.send_frame_size > 0);
    UHD_ASSERT_THROW(params.num_send_frames > 0);
    UHD_ASSERT_THROW(params.num_recv_frames > 0);

    return std::make_shared<liberio_link>(tx_path, rx_path, params);
}

}} // namespace uhd::transport
