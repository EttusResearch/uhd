//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/log.hpp>
#include <uhd/utils/safe_call.hpp>
#include <uhdlib/transport/adapter.hpp>
#include <uhdlib/transport/links.hpp>
#include <uhdlib/transport/nirio_link.hpp>
#include <boost/format.hpp>
#include <chrono>

// X300 regs
#include "../usrp/x300/x300_regs.hpp"

using namespace uhd::transport;
using namespace std::chrono_literals;
using namespace uhd::niusrprio;

/******************************************************************************
 * Local helpers
 *****************************************************************************/
namespace {

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#    include <windows.h>
size_t get_page_size()
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwPageSize;
}
#else
#    include <unistd.h>
size_t get_page_size()
{
    return size_t(sysconf(_SC_PAGESIZE));
}
#endif
const size_t page_size = get_page_size();

} // namespace

#define PROXY _fpga_session->get_kernel_proxy()

/******************************************************************************
 * Structors
 *****************************************************************************/
nirio_link::nirio_link(uhd::niusrprio::niusrprio_session::sptr fpga_session,
    uint32_t instance,
    const link_params_t& params)
    : recv_link_base_t(params.num_recv_frames, params.recv_frame_size)
    , send_link_base_t(params.num_send_frames, params.send_frame_size)
    , _fpga_session(fpga_session)
    , _fifo_instance(instance)
    , _link_params(params)
{
    UHD_LOG_TRACE("NIRIO", "Creating PCIe transport for channel " << instance);
    UHD_LOGGER_TRACE("NIRIO")
        << boost::format("nirio zero-copy RX transport configured with frame size = "
                         "%u, #frames = %u, buffer size = %u\n")
               % _link_params.recv_frame_size % _link_params.num_recv_frames
               % (_link_params.recv_frame_size * _link_params.num_recv_frames);
    UHD_LOGGER_TRACE("NIRIO")
        << boost::format("nirio zero-copy TX transport configured with frame size = "
                         "%u, #frames = %u, buffer size = %u\n")
               % _link_params.send_frame_size % _link_params.num_send_frames
               % (_link_params.send_frame_size * _link_params.num_send_frames);

    nirio_status status = 0;
    size_t actual_depth = 0, actual_size = 0;

    // Disable DMA streams in case last shutdown was unclean (cleanup, so don't status
    // chain)
    PROXY->poke(PCIE_TX_DMA_REG(DMA_CTRL_STATUS_REG, _fifo_instance), DMA_CTRL_DISABLED);
    PROXY->poke(PCIE_RX_DMA_REG(DMA_CTRL_STATUS_REG, _fifo_instance), DMA_CTRL_DISABLED);

    _wait_until_stream_ready();

    // Configure frame width
    nirio_status_chain(
        PROXY->poke(PCIE_TX_DMA_REG(DMA_FRAME_SIZE_REG, _fifo_instance),
            static_cast<uint32_t>(_link_params.send_frame_size / sizeof(fifo_data_t))),
        status);
    nirio_status_chain(
        PROXY->poke(PCIE_RX_DMA_REG(DMA_FRAME_SIZE_REG, _fifo_instance),
            static_cast<uint32_t>(_link_params.recv_frame_size / sizeof(fifo_data_t))),
        status);

    // Config 64-bit word flipping and enable DMA streams
    nirio_status_chain(PROXY->poke(PCIE_TX_DMA_REG(DMA_CTRL_STATUS_REG, _fifo_instance),
                           DMA_CTRL_SW_BUF_U64 | DMA_CTRL_ENABLED),
        status);
    nirio_status_chain(PROXY->poke(PCIE_RX_DMA_REG(DMA_CTRL_STATUS_REG, _fifo_instance),
                           DMA_CTRL_SW_BUF_U64 | DMA_CTRL_ENABLED),
        status);

    // Create FIFOs
    nirio_status_chain(_fpga_session->create_rx_fifo(_fifo_instance, _recv_fifo), status);
    nirio_status_chain(_fpga_session->create_tx_fifo(_fifo_instance, _send_fifo), status);

    if ((_recv_fifo.get() != NULL) && (_send_fifo.get() != NULL)) {
        // Initialize FIFOs
        nirio_status_chain(_recv_fifo->initialize((_link_params.recv_frame_size
                                                      * _link_params.num_recv_frames)
                                                      / sizeof(fifo_data_t),
                               _link_params.recv_frame_size / sizeof(fifo_data_t),
                               actual_depth,
                               actual_size),
            status);
        nirio_status_chain(_send_fifo->initialize((_link_params.send_frame_size
                                                      * _link_params.num_send_frames)
                                                      / sizeof(fifo_data_t),
                               _link_params.send_frame_size / sizeof(fifo_data_t),
                               actual_depth,
                               actual_size),
            status);

        PROXY->get_rio_quirks().add_tx_fifo(_fifo_instance);

        nirio_status_chain(_recv_fifo->start(), status);
        nirio_status_chain(_send_fifo->start(), status);

        if (!nirio_status_not_fatal(status)) {
            UHD_LOG_ERROR("NIRIO", "Fatal error while creating RX/TX FIFOs!");
        }
    } else {
        nirio_status_chain(NiRio_Status_ResourceNotInitialized, status);
    }

    nirio_status_to_exception(status, "Could not create nirio_link!");

    // Preallocate empty frame_buffs
    // We don't need to reserve any memory, because the DMA engine will do that
    // for us. We just need to create a bunch of frame_buff objects.
    _recv_buffs.reserve(_link_params.num_recv_frames);
    _send_buffs.reserve(_link_params.num_send_frames);
    for (size_t i = 0; i < _link_params.num_recv_frames; i++) {
        _recv_buffs.emplace_back();
        recv_link_base_t::preload_free_buff(&_recv_buffs.back());
    }
    for (size_t i = 0; i < _link_params.num_send_frames; i++) {
        _send_buffs.emplace_back();
        send_link_base_t::preload_free_buff(&_send_buffs.back());
    }

    // Create adapter info
    auto info   = nirio_adapter_info(_fpga_session->get_resource());
    auto& ctx   = adapter_ctx::get();
    _adapter_id = ctx.register_adapter(info);
}

nirio_link::~nirio_link()
{
    PROXY->get_rio_quirks().remove_tx_fifo(_fifo_instance);

    // Disable DMA streams (cleanup, so don't status chain)
    PROXY->poke(PCIE_TX_DMA_REG(DMA_CTRL_STATUS_REG, _fifo_instance), DMA_CTRL_DISABLED);
    PROXY->poke(PCIE_RX_DMA_REG(DMA_CTRL_STATUS_REG, _fifo_instance), DMA_CTRL_DISABLED);

    UHD_SAFE_CALL(_flush_rx_buff();)

    // Stop DMA channels. Stop is called in the fifo dtor but
    // it doesn't hurt to do it here.
    _send_fifo->stop();
    _recv_fifo->stop();
}

nirio_link::sptr nirio_link::make(uhd::niusrprio::niusrprio_session::sptr fpga_session,
    const uint32_t instance,
    const uhd::transport::link_params_t& default_params,
    const uhd::device_addr_t& hints,
    size_t& recv_buff_size,
    size_t& send_buff_size)
{
    UHD_ASSERT_THROW(default_params.num_recv_frames != 0);
    UHD_ASSERT_THROW(default_params.num_send_frames != 0);
    UHD_ASSERT_THROW(default_params.recv_frame_size != 0);
    UHD_ASSERT_THROW(default_params.send_frame_size != 0);
    UHD_ASSERT_THROW(default_params.recv_buff_size != 0);
    UHD_ASSERT_THROW(default_params.send_buff_size != 0);

    // Initialize xport_params
    link_params_t link_params = default_params;

    // The kernel buffer for this transport must be (num_frames * frame_size) big. Unlike
    // ethernet, where the kernel buffer size is independent of the circular buffer size
    // for the transport, it is possible for users to over constrain the system when they
    // set the num_frames and the buff_size. So we give buff_size priority over num_frames
    // and throw an error if they conflict.

    // RX
    link_params.recv_frame_size =
        size_t(hints.cast<double>("recv_frame_size", default_params.recv_frame_size));

    size_t usr_num_recv_frames = static_cast<size_t>(
        hints.cast<double>("num_recv_frames", double(default_params.num_recv_frames)));
    size_t usr_recv_buff_size = static_cast<size_t>(
        hints.cast<double>("recv_buff_size", double(default_params.recv_buff_size)));

    if (hints.has_key("recv_buff_size")) {
        if (usr_recv_buff_size % page_size != 0) {
            throw uhd::value_error(
                (boost::format("recv_buff_size must be multiple of %d") % page_size)
                    .str());
        }
    }

    if (hints.has_key("recv_frame_size") and hints.has_key("num_recv_frames")) {
        if (usr_num_recv_frames * link_params.recv_frame_size % page_size != 0) {
            throw uhd::value_error(
                (boost::format(
                     "num_recv_frames * recv_frame_size must be an even multiple of %d")
                    % page_size)
                    .str());
        }
    }

    if (hints.has_key("num_recv_frames") and hints.has_key("recv_buff_size")) {
        if (usr_recv_buff_size < link_params.recv_frame_size)
            throw uhd::value_error("recv_buff_size must be equal to or greater than "
                                   "(num_recv_frames * recv_frame_size)");

        if ((usr_recv_buff_size / link_params.recv_frame_size) != usr_num_recv_frames)
            throw uhd::value_error(
                "Conflicting values for recv_buff_size and num_recv_frames");
    }

    if (hints.has_key("recv_buff_size")) {
        link_params.num_recv_frames = std::max<size_t>(
            1, usr_recv_buff_size / link_params.recv_frame_size); // Round down
    } else if (hints.has_key("num_recv_frames")) {
        link_params.num_recv_frames = usr_num_recv_frames;
    }

    if (link_params.num_recv_frames * link_params.recv_frame_size % page_size != 0) {
        throw uhd::value_error(
            (boost::format(
                 "num_recv_frames * recv_frame_size must be an even multiple of %d")
                % page_size)
                .str());
    }

    // TX
    link_params.send_frame_size =
        size_t(hints.cast<double>("send_frame_size", default_params.send_frame_size));

    size_t usr_num_send_frames = static_cast<size_t>(
        hints.cast<double>("num_send_frames", default_params.num_send_frames));
    size_t usr_send_buff_size = static_cast<size_t>(
        hints.cast<double>("send_buff_size", default_params.send_buff_size));

    if (hints.has_key("send_buff_size")) {
        if (usr_send_buff_size % page_size != 0) {
            throw uhd::value_error(
                (boost::format("send_buff_size must be multiple of %d") % page_size)
                    .str());
        }
    }

    if (hints.has_key("send_frame_size") and hints.has_key("num_send_frames")) {
        if (usr_num_send_frames * link_params.send_frame_size % page_size != 0) {
            throw uhd::value_error(
                (boost::format(
                     "num_send_frames * send_frame_size must be an even multiple of %d")
                    % page_size)
                    .str());
        }
    }

    if (hints.has_key("num_send_frames") and hints.has_key("send_buff_size")) {
        if (usr_send_buff_size < link_params.send_frame_size)
            throw uhd::value_error("send_buff_size must be equal to or greater than "
                                   "(num_send_frames * send_frame_size)");

        if ((usr_send_buff_size / link_params.send_frame_size) != usr_num_send_frames)
            throw uhd::value_error(
                "Conflicting values for send_buff_size and num_send_frames");
    }

    if (hints.has_key("send_buff_size")) {
        link_params.num_send_frames = std::max<size_t>(
            1, usr_send_buff_size / link_params.send_frame_size); // Round down
    } else if (hints.has_key("num_send_frames")) {
        link_params.num_send_frames = usr_num_send_frames;
    }

    if (link_params.num_send_frames * link_params.send_frame_size % page_size != 0) {
        throw uhd::value_error(
            (boost::format(
                 "num_send_frames * send_frame_size must be an even multiple of %d")
                % page_size)
                .str());
    }

    recv_buff_size = link_params.num_recv_frames * link_params.recv_frame_size;
    send_buff_size = link_params.num_send_frames * link_params.send_frame_size;

    return nirio_link::sptr(new nirio_link(fpga_session, instance, link_params));
}


/******************************************************************************
 * NI-RIO-specific helpers
 *****************************************************************************/
void nirio_link::_flush_rx_buff()
{
    // acquire is called with 0 elements requested first to
    // get the number of elements in the buffer and then
    // repeatedly with the number of remaining elements
    // until the buffer is empty
    for (size_t num_elems_requested = 0, num_elems_acquired = 0, num_elems_remaining = 1;
         num_elems_remaining;
         num_elems_requested = num_elems_remaining) {
        fifo_data_t* elems_buffer = NULL;
        nirio_status status       = _recv_fifo->acquire(elems_buffer,
            num_elems_requested,
            0, // timeout
            num_elems_acquired,
            num_elems_remaining);
        // throw exception if status is fatal
        nirio_status_to_exception(
            status, "NI-RIO PCIe data transfer failed during flush.");
        _recv_fifo->release(num_elems_acquired);
    }
}

void nirio_link::_wait_until_stream_ready()
{
    constexpr auto TIMEOUT_IN_MS = 100ms;

    uint32_t reg_data = 0xffffffff;
    bool tx_busy = true, rx_busy = true;
    nirio_status status = NiRio_Status_Success;

    nirio_status_chain(
        PROXY->peek(PCIE_TX_DMA_REG(DMA_CTRL_STATUS_REG, _fifo_instance), reg_data),
        status);
    tx_busy = (reg_data & DMA_STATUS_BUSY) > 0;
    nirio_status_chain(
        PROXY->peek(PCIE_RX_DMA_REG(DMA_CTRL_STATUS_REG, _fifo_instance), reg_data),
        status);
    rx_busy = (reg_data & DMA_STATUS_BUSY) > 0;

    if (nirio_status_not_fatal(status) && (tx_busy || rx_busy)) {
        const auto end_time = std::chrono::steady_clock::now() + TIMEOUT_IN_MS;
        do {
            std::this_thread::sleep_for(50ms); // Avoid flooding the bus
            nirio_status_chain(
                PROXY->peek(
                    PCIE_TX_DMA_REG(DMA_CTRL_STATUS_REG, _fifo_instance), reg_data),
                status);
            tx_busy = (reg_data & DMA_STATUS_BUSY) > 0;
            nirio_status_chain(
                PROXY->peek(
                    PCIE_RX_DMA_REG(DMA_CTRL_STATUS_REG, _fifo_instance), reg_data),
                status);
            rx_busy = (reg_data & DMA_STATUS_BUSY) > 0;
        } while (nirio_status_not_fatal(status) && (tx_busy || rx_busy)
                 && (std::chrono::steady_clock::now() < end_time));

        if (tx_busy || rx_busy) {
            nirio_status_chain(NiRio_Status_FpgaBusy, status);
        }

        nirio_status_to_exception(status, "Could not create nirio_zero_copy transport.");
    }
}
