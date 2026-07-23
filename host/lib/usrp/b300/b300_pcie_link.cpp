// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "./inchworm/user/libnifpga-usrp/src/Exception.h"
#include "b300_regs.hpp"
#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/platform.hpp>
#include <uhd/utils/safe_call.hpp>
#include <uhdlib/transport/adapter.hpp>
#include <uhdlib/usrp/b300/b300_pcie_link.hpp>
#include <boost/format.hpp>
#include <chrono>
#include <thread>

using namespace uhd::usrp::b300;
using namespace uhd::transport;
using namespace std::chrono_literals;

//! Static page size
const size_t page_size = uhd::get_page_size();

namespace {
class b300_pcie_adapter_info : public uhd::transport::adapter_info
{
public:
    explicit b300_pcie_adapter_info(const std::string& resource) : _resource(resource) {}
    ~b300_pcie_adapter_info(){};

    std::string to_string() override
    {
        return std::string("B300-PCIe:") + _resource;
    }

    bool operator==(const b300_pcie_adapter_info& rhs) const
    {
        return (_resource == rhs._resource);
    }

private:
    const std::string _resource;
};
} // namespace

/******************************************************************************
 * Structors
 *****************************************************************************/
b300_pcie_link::b300_pcie_link(b300_pcie_session::sptr session,
    uint32_t instance,
    std::function<void(uint32_t)>&& release_cb,
    const link_params_t& params,
    uhd::rfnoc::chdr_w_t chdr_w)
    : recv_link_base_t(params.num_recv_frames, params.recv_frame_size)
    , send_link_base_t(params.num_send_frames, params.send_frame_size)
    , _b300_session(session)
    , _fifo_instance(instance)
    , _release_cb(std::move(release_cb))
    , _link_params(params)
    , _chdr_w(chdr_w)
{
    UHD_LOG_TRACE("B300", "Creating B300 PCIe link for channel " << instance);
    UHD_LOG_TRACE("B300", "Using page size: " << page_size);

    UHD_LOG_DEBUG("B300",
        boost::format("B300 PCIe RX transport configured with frame size = %u, "
                      "#frames = %u, buffer size = %u")
            % _link_params.recv_frame_size % _link_params.num_recv_frames
            % (_link_params.recv_frame_size * _link_params.num_recv_frames));

    UHD_LOG_DEBUG("B300",
        boost::format("B300 PCIe TX transport configured with frame size = %u, "
                      "#frames = %u, buffer size = %u")
            % _link_params.send_frame_size % _link_params.num_send_frames
            % (_link_params.send_frame_size * _link_params.num_send_frames));

    // --- DMA Register Configuration (like nirio_link) ---
    // 1. Disable DMA streams in case last shutdown was unclean
    _b300_session->poke32(
        PCIE_TX_DMA_REG(DMA_CTRL_STATUS_REG, _fifo_instance), DMA_CTRL_DISABLED);
    _b300_session->poke32(
        PCIE_RX_DMA_REG(DMA_CTRL_STATUS_REG, _fifo_instance), DMA_CTRL_DISABLED);

    _wait_until_stream_ready();

    // 2. Configure frame width
    // DMA register expects frame size in units of (CHDR_width_in_bits / 8) bytes
    const size_t chdr_word_size = uhd::rfnoc::chdr_w_to_bits(_chdr_w) / 8;
    _b300_session->poke32(PCIE_TX_DMA_REG(DMA_FRAME_SIZE_REG, _fifo_instance),
        static_cast<uint32_t>(_link_params.send_frame_size / chdr_word_size));
    _b300_session->poke32(PCIE_RX_DMA_REG(DMA_FRAME_SIZE_REG, _fifo_instance),
        static_cast<uint32_t>(_link_params.recv_frame_size / chdr_word_size));


    // 3. Configure 64-bit word flipping and enable DMA streams
    _b300_session->poke32(PCIE_TX_DMA_REG(DMA_CTRL_STATUS_REG, _fifo_instance),
        DMA_CTRL_SW_BUF_U64 | DMA_CTRL_ENABLED);
    _b300_session->poke32(PCIE_RX_DMA_REG(DMA_CTRL_STATUS_REG, _fifo_instance),
        DMA_CTRL_SW_BUF_U64 | DMA_CTRL_ENABLED);

    // Create FIFOs
    _recv_fifo = _b300_session->create_rx_fifo(_fifo_instance);
    _send_fifo = _b300_session->create_tx_fifo(_fifo_instance + B300_TX_FIFO_OFFSET);

    // Make sure FIFOs were created successfully
    if (_recv_fifo && _send_fifo) {
        // Initialize FIFOs
        _recv_fifo->configure(
            (_link_params.recv_frame_size * _link_params.num_recv_frames)
            / sizeof(uint64_t));

        _send_fifo->configure(
            (_link_params.send_frame_size * _link_params.num_send_frames)
            / sizeof(uint64_t));

        // Start FIFOs
        _recv_fifo->start();
        _send_fifo->start();
    } else {
        throw uhd::runtime_error("Could not create B300 PCIe link!");
    }

    // Preallocate frame_buffs
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

    // Create adapter info and register it to get adapter_id
    auto info   = b300_pcie_adapter_info(_b300_session->get_resource());
    auto& ctx   = uhd::transport::adapter_ctx::get();
    _adapter_id = ctx.register_adapter(info);
}

b300_pcie_link::~b300_pcie_link()
{
    UHD_LOG_TRACE("B300", "Destroying B300 PCIe link for channel " << _fifo_instance);

    try {
        // Call release callback
        if (_release_cb) {
            _release_cb(_fifo_instance);
        }

        // --- Disable DMA streams (like nirio_link) ---
        _b300_session->poke32(
            PCIE_TX_DMA_REG(DMA_CTRL_STATUS_REG, _fifo_instance), DMA_CTRL_DISABLED);
        _b300_session->poke32(
            PCIE_RX_DMA_REG(DMA_CTRL_STATUS_REG, _fifo_instance), DMA_CTRL_DISABLED);

        // Flush any pending buffers
        UHD_SAFE_CALL(_flush_rx_buff());

        // Stop FIFOs
        if (_recv_fifo) {
            _recv_fifo->stop();
        }
        if (_send_fifo) {
            _send_fifo->stop();
        }

    } catch (const std::exception& ex) {
        UHD_LOG_ERROR("B300", "Error during B300 PCIe link cleanup: " << ex.what());
    }
}

/******************************************************************************
 * Factory function
 *****************************************************************************/
b300_pcie_link::sptr b300_pcie_link::make(b300_pcie_session::sptr session,
    uint32_t instance,
    std::function<void(uint32_t)>&& release_cb,
    const link_params_t& params,
    uhd::rfnoc::chdr_w_t chdr_w)
{
    return sptr(
        new b300_pcie_link(session, instance, std::move(release_cb), params, chdr_w));
}

b300_pcie_link::sptr b300_pcie_link::make(b300_pcie_session::sptr session,
    uint32_t instance,
    std::function<void(uint32_t)>&& release_cb,
    const link_params_t& default_params,
    const uhd::device_addr_t& hints,
    size_t& recv_buff_size,
    size_t& send_buff_size,
    uhd::rfnoc::chdr_w_t chdr_w)
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
        hints.cast<size_t>("recv_frame_size", default_params.recv_frame_size);

    size_t usr_num_recv_frames =
        hints.cast<size_t>("num_recv_frames", default_params.num_recv_frames);
    size_t usr_recv_buff_size =
        hints.cast<size_t>("recv_buff_size", default_params.recv_buff_size);

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
        if (usr_recv_buff_size < link_params.recv_frame_size) {
            throw uhd::value_error("recv_buff_size must be equal to or greater than "
                                   "(num_recv_frames * recv_frame_size)");
        }

        if ((usr_recv_buff_size / link_params.recv_frame_size) != usr_num_recv_frames) {
            throw uhd::value_error(
                "Conflicting values for recv_buff_size and num_recv_frames");
        }
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
        hints.cast<size_t>("send_frame_size", default_params.send_frame_size);

    size_t usr_num_send_frames =
        hints.cast<size_t>("num_send_frames", default_params.num_send_frames);
    size_t usr_send_buff_size =
        hints.cast<size_t>("send_buff_size", default_params.send_buff_size);

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
        if (usr_send_buff_size < link_params.send_frame_size) {
            throw uhd::value_error("send_buff_size must be equal to or greater than "
                                   "(num_send_frames * send_frame_size)");
        }

        if ((usr_send_buff_size / link_params.send_frame_size) != usr_num_send_frames) {
            throw uhd::value_error(
                "Conflicting values for send_buff_size and num_send_frames");
        }
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

    return b300_pcie_link::sptr(new b300_pcie_link(
        session, instance, std::move(release_cb), link_params, chdr_w));
}

/******************************************************************************
 * Link API methods
 *****************************************************************************/
size_t b300_pcie_link::get_recv_buff_derived(frame_buff& buff, int32_t timeout_ms)
{
    // This will modify the data pointer in buff if successful:
    uint64_t** data_ptr = static_cast<b300_pcie_frame_buff&>(buff).get_fifo_ptr_ref();
    try {
        auto result         = _recv_fifo->acquire(*data_ptr,
            _link_params.recv_frame_size / sizeof(uint64_t),
            static_cast<uint32_t>(timeout_ms));
        const size_t length = result.elements_acquired * sizeof(uint64_t);
        return length;
    } catch (const nirio::FifoTimeoutException&) {
        return 0; // return 0 for timeout.
    }
}

void b300_pcie_link::release_recv_buff_derived(frame_buff& /*buff*/)
{
    _recv_fifo->release(_link_params.recv_frame_size / sizeof(uint64_t));
}

bool b300_pcie_link::get_send_buff_derived(frame_buff& buff, int32_t timeout_ms)
{
    // This will modify the data pointer in buff if successful:
    uint64_t** data_ptr = static_cast<b300_pcie_frame_buff&>(buff).get_fifo_ptr_ref();
    try {
        _send_fifo->acquire(*data_ptr,
            _link_params.send_frame_size / sizeof(uint64_t),
            static_cast<uint32_t>(timeout_ms));
        return true;
    } catch (const std::exception& ex) {
        UHD_LOG_WARNING("B300", "Error acquiring from TX FIFO: " << ex.what());
        return false;
    }
}

void b300_pcie_link::release_send_buff_derived(frame_buff& /*buff*/)
{
    _send_fifo->release(_link_params.send_frame_size / sizeof(uint64_t));
}

adapter_id_t b300_pcie_link::get_send_adapter_id() const
{
    return _adapter_id;
}

adapter_id_t b300_pcie_link::get_recv_adapter_id() const
{
    return _adapter_id;
}

/******************************************************************************
 * B300 PCIe-specific helpers
 *****************************************************************************/
void b300_pcie_link::_flush_rx_buff()
{
    // Acquire is called with 0 elements requested first to
    // get the number of elements in the buffer and then
    // repeatedly with the number of remaining elements
    // until the buffer is empty
    for (size_t num_elems_requested = 0, num_elems_remaining = 1; num_elems_remaining;
         num_elems_requested = num_elems_remaining) {
        uint64_t* elems_buffer = nullptr;
        try {
            auto result         = _recv_fifo->acquire(elems_buffer,
                num_elems_requested,
                0); // timeout
            num_elems_remaining = result.elements_remaining;
            _recv_fifo->release(result.elements_acquired);
        } catch (const std::exception& ex) {
            UHD_LOG_WARNING(
                "B300", "B300 PCIe data transfer failed during flush: " << ex.what());
            break;
        }
    }
}

void b300_pcie_link::_wait_until_stream_ready()
{
    constexpr size_t max_attempts = 10;
    constexpr auto poll_interval  = std::chrono::milliseconds(10);

    uint32_t tx_status = 0xFFFFFFFF;
    uint32_t rx_status = 0xFFFFFFFF;

    for (size_t attempt = 0; attempt < max_attempts; ++attempt) {
        tx_status =
            _b300_session->peek32(PCIE_TX_DMA_REG(DMA_CTRL_STATUS_REG, _fifo_instance));
        rx_status =
            _b300_session->peek32(PCIE_RX_DMA_REG(DMA_CTRL_STATUS_REG, _fifo_instance));

        bool tx_busy = (tx_status & DMA_STATUS_BUSY) != 0;
        bool rx_busy = (rx_status & DMA_STATUS_BUSY) != 0;

        if (!tx_busy && !rx_busy) {
            return;
        }
        std::this_thread::sleep_for(poll_interval);
    }

    std::string which_busy;
    if (tx_status & DMA_STATUS_BUSY)
        which_busy += "TX ";
    if (rx_status & DMA_STATUS_BUSY)
        which_busy += "RX ";

    UHD_LOG_ERROR("B300",
        "Timeout waiting for DMA stream(s) to become ready for channel "
            << _fifo_instance << ": still busy: " << which_busy);
    throw uhd::runtime_error(
        "Timeout waiting for DMA stream(s) to become ready for channel "
        + std::to_string(_fifo_instance) + ": still busy: " + which_busy);
}
