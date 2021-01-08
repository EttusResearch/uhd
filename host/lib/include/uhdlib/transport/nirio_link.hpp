//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/transport/buffer_pool.hpp>
#include <uhd/transport/nirio/nirio_fifo.h>
#include <uhd/transport/nirio/niriok_proxy.h>
#include <uhd/transport/nirio/niusrprio_session.h>
#include <uhd/types/device_addr.hpp>
#include <uhdlib/transport/adapter_info.hpp>
#include <uhdlib/transport/link_base.hpp>
#include <uhdlib/transport/links.hpp>
#include <memory>
#include <vector>

namespace uhd { namespace transport {

using fifo_data_t = uint64_t;

/*! NI-RIO frame_buff
 *
 * NI-RIO internally manages the memory, so these frame_buff objects just need
 * a way of internally manipulating the _data pointer
 */
class nirio_frame_buff : public frame_buff
{
public:
    nirio_frame_buff()
    {
        _data = nullptr;
    }

    fifo_data_t** get_fifo_ptr_ref()
    {
        return reinterpret_cast<fifo_data_t**>(&_data);
    };
};

class nirio_adapter_info : public adapter_info
{
public:
    nirio_adapter_info(const std::string& resource) : _resource(resource) {}

    ~nirio_adapter_info() {}

    std::string to_string() override
    {
        return std::string("NIRIO:") + _resource;
    }

    bool operator==(const nirio_adapter_info& rhs) const
    {
        return (_resource == rhs._resource);
    }

private:
    const std::string _resource;
};

/*! Link object to talking to NI-RIO USRPs (X310 variants using PCIe)
 *
 * \b Note: This link cannot release frame buffers out of order, which means it
 *          can't be used with an IO service that does that.
 */
class nirio_link : public recv_link_base<nirio_link>, public send_link_base<nirio_link>
{
public:
    using sptr = std::shared_ptr<nirio_link>;

    ~nirio_link();

    /*! Make a new NI-RIO link.
     *
     * \param addr a string representing the destination address
     * \param port a string representing the destination port
     * \param params Values for frame sizes, num frames, and buffer sizes
     * \param[out] recv_buff_size Returns the recv buffer size
     * \param[out] send_buff_size Returns the send buffer size
     */
    static sptr make(uhd::niusrprio::niusrprio_session::sptr fpga_session,
        const uint32_t instance,
        const link_params_t& params,
        const uhd::device_addr_t& hints,
        size_t& recv_buff_size,
        size_t& send_buff_size);

    /*!
     * Get the physical adapter ID used for this link
     */
    adapter_id_t get_send_adapter_id() const override
    {
        return _adapter_id;
    }

    /*!
     * Get the physical adapter ID used for this link
     */
    adapter_id_t get_recv_adapter_id() const override
    {
        return _adapter_id;
    }

    /*!
     * Returns whether this link type supports releasing the frame buffers
     * in an order different from that in which they were acquired.
     */
    bool supports_send_buff_out_of_order() const override
    {
        return false;
    }

    /*!
     * Returns whether this link type supports releasing the frame buffers
     * in an order different from that in which they were acquired.
     */
    bool supports_recv_buff_out_of_order() const override
    {
        return false;
    }

private:
    using recv_link_base_t = recv_link_base<nirio_link>;
    using send_link_base_t = send_link_base<nirio_link>;

    // Friend declarations to allow base classes to call private methods
    friend recv_link_base_t;
    friend send_link_base_t;

    nirio_link(uhd::niusrprio::niusrprio_session::sptr fpga_session,
        uint32_t instance,
        const link_params_t& params);

    /**************************************************************************
     * NI-RIO specific helpers
     *************************************************************************/
    void _flush_rx_buff();

    void _wait_until_stream_ready();

    /**************************************************************************
     * recv_link/send_link API
     *************************************************************************/
    // Methods called by recv_link_base
    UHD_FORCE_INLINE size_t get_recv_buff_derived(frame_buff& buff, int32_t timeout_ms)
    {
        using namespace uhd::niusrprio;
        nirio_status status    = 0;
        size_t elems_acquired  = 0;
        size_t elems_remaining = 0;
        // This will modify the data pointer in buff if successful:
        fifo_data_t** data_ptr = static_cast<nirio_frame_buff&>(buff).get_fifo_ptr_ref();
        nirio_status_chain(_recv_fifo->acquire(*data_ptr,
                               _link_params.recv_frame_size / sizeof(fifo_data_t),
                               static_cast<uint32_t>(timeout_ms),
                               elems_acquired,
                               elems_remaining),
            status);
        const size_t length = elems_acquired * sizeof(fifo_data_t);

        if (nirio_status_not_fatal(status)) {
            return length;
        } else if (status == NiRio_Status_CommunicationTimeout) {
            nirio_status_to_exception(status, "NI-RIO PCIe data transfer failed.");
        }
        return 0; // zero for timeout or error.
    }

    UHD_FORCE_INLINE void release_recv_buff_derived(frame_buff& /*buff*/)
    {
        _recv_fifo->release(_link_params.recv_frame_size / sizeof(fifo_data_t));
    }

    // Methods called by send_link_base
    UHD_FORCE_INLINE bool get_send_buff_derived(frame_buff& buff, int32_t timeout_ms)
    {
        using namespace uhd::niusrprio;
        nirio_status status    = 0;
        size_t elems_acquired  = 0;
        size_t elems_remaining = 0;
        // This will modify the data pointer in buff if successful:
        fifo_data_t** data_ptr = static_cast<nirio_frame_buff&>(buff).get_fifo_ptr_ref();
        nirio_status_chain(_send_fifo->acquire(*data_ptr,
                               _link_params.send_frame_size / sizeof(fifo_data_t),
                               static_cast<uint32_t>(timeout_ms),
                               elems_acquired,
                               elems_remaining),
            status);
        // const size_t length = elems_acquired * sizeof(fifo_data_t);

        if (nirio_status_not_fatal(status)) {
            return true;
        } else if (status == NiRio_Status_CommunicationTimeout) {
            nirio_status_to_exception(status, "NI-RIO PCIe data transfer failed.");
        }
        return false;
    }

    UHD_FORCE_INLINE void release_send_buff_derived(frame_buff& /*buff*/)
    {
        _send_fifo->release(_link_params.send_frame_size / sizeof(fifo_data_t));
    }

    /**************************************************************************
     * Private attributes
     *************************************************************************/
    //! Reference to the NI-RIO session
    niusrprio::niusrprio_session::sptr _fpga_session;
    //! DMA channel index
    const uint32_t _fifo_instance;
    //! Recv and send FIFO objects
    uhd::niusrprio::nirio_fifo<fifo_data_t>::sptr _recv_fifo, _send_fifo;

    const link_params_t _link_params;

    std::vector<nirio_frame_buff> _recv_buffs;
    std::vector<nirio_frame_buff> _send_buffs;

    adapter_id_t _adapter_id;
};

}} // namespace uhd::transport

