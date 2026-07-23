//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/rfnoc/rfnoc_types.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhdlib/transport/link_base.hpp>
#include <uhdlib/transport/links.hpp>
#include <uhdlib/usrp/b300/b300_pcie_fifo.hpp>
#include <uhdlib/usrp/b300/b300_pcie_session.hpp>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace uhd { namespace usrp { namespace b300 {

/**
 * Frame buffer implementation for B300 PCIe transport
 */
class b300_pcie_frame_buff : public uhd::transport::frame_buff
{
public:
    b300_pcie_frame_buff()
    {
        _data = nullptr;
    }

    // Get a reference to the internal FIFO data pointer so it can be modified
    // by the acquire operation
    uint64_t** get_fifo_ptr_ref()
    {
        return reinterpret_cast<uint64_t**>(&_data);
    }
};

/**
 * B300 PCIe transport link implementation.
 * Creates and manages FIFOs for data transport over PCIe.
 */
class b300_pcie_link : public uhd::transport::recv_link_base<b300_pcie_link>,
                       public uhd::transport::send_link_base<b300_pcie_link>
{
public:
    using sptr = std::shared_ptr<b300_pcie_link>;

    /**
     * Create a new B300 PCIe link
     *
     * \param session The PCIe session
     * \param instance DMA channel number
     * \param release_cb Callback to call when link is destroyed
     * \param params Link parameters
     * \return Shared pointer to the created link
     */
    static sptr make(b300_pcie_session::sptr session,
        uint32_t instance,
        std::function<void(uint32_t)>&& release_cb,
        const uhd::transport::link_params_t& params,
        uhd::rfnoc::chdr_w_t chdr_w);

    /**
     * Create a new B300 PCIe link
     *
     * \param session The PCIe session
     * \param instance DMA channel number
     * \param release_cb Callback to call when link is destroyed
     * \param default_params Default link parameters
     * \param hints User hints to override default parameters
     * \param recv_buff_size Output parameter for actual receive buffer size
     * \param send_buff_size Output parameter for actual send buffer size
     * \return Shared pointer to the created link
     */
    static sptr make(b300_pcie_session::sptr session,
        uint32_t instance,
        std::function<void(uint32_t)>&& release_cb,
        const uhd::transport::link_params_t& default_params,
        const uhd::device_addr_t& hints,
        size_t& recv_buff_size,
        size_t& send_buff_size,
        uhd::rfnoc::chdr_w_t chdr_w);

    /**
     * Destructor
     */
    ~b300_pcie_link();

    /**
     * Get the physical adapter ID used for this link
     */
    uhd::transport::adapter_id_t get_send_adapter_id() const override;

    /**
     * Get the physical adapter ID used for this link
     */
    uhd::transport::adapter_id_t get_recv_adapter_id() const override;

    /**
     * Check if this link supports out-of-order send buffer release
     */
    bool supports_send_buff_out_of_order() const override
    {
        return false;
    }

    /**
     * Check if this link supports out-of-order receive buffer release
     */
    bool supports_recv_buff_out_of_order() const override
    {
        return false;
    }

private:
    using recv_link_base_t = uhd::transport::recv_link_base<b300_pcie_link>;
    using send_link_base_t = uhd::transport::send_link_base<b300_pcie_link>;

    // Friend declarations to allow base classes to call private methods
    friend recv_link_base_t;
    friend send_link_base_t;

    /**
     * Constructor
     *
     * \param session The PCIe session
     * \param instance DMA channel number
     * \param release_cb Callback to call when link is destroyed
     * \param params Link parameters
     */
    b300_pcie_link(b300_pcie_session::sptr session,
        uint32_t instance,
        std::function<void(uint32_t)>&& release_cb,
        const uhd::transport::link_params_t& params,
        uhd::rfnoc::chdr_w_t chdr_w);

    // Methods called by recv_link_base
    size_t get_recv_buff_derived(uhd::transport::frame_buff& buff, int32_t timeout_ms);
    void release_recv_buff_derived(uhd::transport::frame_buff& buff);

    // Methods called by send_link_base
    bool get_send_buff_derived(uhd::transport::frame_buff& buff, int32_t timeout_ms);
    void release_send_buff_derived(uhd::transport::frame_buff& buff);

    // Flush the receive buffer to handle overflow/underflow conditions
    void _flush_rx_buff();

    // Wait until DMA stream is ready (not busy)
    void _wait_until_stream_ready();

    // Reference to the PCIe session
    b300_pcie_session::sptr _b300_session;

    // DMA channel index
    const uint32_t _fifo_instance;

    // Callback when link is released
    std::function<void(uint32_t)> _release_cb;

    // Link parameters
    const uhd::transport::link_params_t _link_params;

    // CHDR width
    const uhd::rfnoc::chdr_w_t _chdr_w;

    // Recv and send FIFO objects
    b300_pcie_fifo::sptr _recv_fifo, _send_fifo;

    // Frame buffer objects for reuse
    std::vector<b300_pcie_frame_buff> _recv_buffs;
    std::vector<b300_pcie_frame_buff> _send_buffs;

    // Adapter ID info
    uhd::transport::adapter_id_t _adapter_id;
};

}}} // namespace uhd::usrp::b300
