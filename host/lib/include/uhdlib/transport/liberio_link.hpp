//
// Copyright 2019 Ettus Research, a National Instruments brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHDLIB_TRANSPORT_LIBERIO_LINK_HPP
#define INCLUDED_UHDLIB_TRANSPORT_LIBERIO_LINK_HPP

#include <uhd/config.hpp>
#include <uhd/transport/buffer_pool.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhdlib/transport/adapter_info.hpp>
#include <uhdlib/transport/link_base.hpp>
#include <liberio/liberio.h>
#include <cassert>
#include <string>
#include <vector>

namespace uhd { namespace transport {

class liberio_frame_buff : public frame_buff
{
public:
    liberio_frame_buff(struct liberio_chan* chan) : _chan(chan)
    {
        assert(_chan);
        // Acquire channel reference
        liberio_chan_get(_chan);
    }

    ~liberio_frame_buff()
    {
        if (_buff) {
            // Set payload size to signify "empty"
            liberio_buf_set_payload(_buff, 0, 0);

            // Release buffer back to liberio
            liberio_chan_buf_enqueue(_chan, _buff);
            _buff = nullptr;
            _data = nullptr;
        }
        // Release channel reference
        liberio_chan_put(_chan);
    }

    liberio_frame_buff(const liberio_frame_buff& src) : liberio_frame_buff(src._chan) {}

    UHD_FORCE_INLINE size_t get(int32_t timeout_ms)
    {
        // Dequeue timeout in microseconds
        _buff = liberio_chan_buf_dequeue(_chan, timeout_ms * 1000);
        if (!_buff) {
            return 0;
        }
        _packet_size = liberio_buf_get_len(_buff, 0);
        _data        = liberio_buf_get_mem(_buff, 0);
        return _packet_size;
    }

    UHD_FORCE_INLINE void release()
    {
        assert(_buff);
        liberio_buf_set_payload(_buff, 0, _packet_size);
        liberio_chan_buf_enqueue(_chan, _buff);
        _buff = nullptr;
        _data = nullptr;
    }

private:
    struct liberio_buf* _buff = nullptr;
    struct liberio_chan* _chan;
};

class liberio_adapter_info : public adapter_info
{
public:
    liberio_adapter_info()  = default;
    ~liberio_adapter_info() = default;

    std::string to_string()
    {
        // Currently, there is only ever one liberio adapter
        // If that changes, fix this!
        return std::string("Liberio");
    }

    bool operator==(const liberio_adapter_info& /*rhs*/) const
    {
        // Currently, there is only ever one liberio adapter
        // If that changes, fix this!
        return true;
    }
};

/*!
 * A zero copy transport interface to the liberio DMA library.
 */
class liberio_link : public recv_link_base<liberio_link>,
                     public send_link_base<liberio_link>
{
public:
    using sptr = std::shared_ptr<liberio_link>;

    liberio_link(const std::string& tx_path,
        const std::string& rx_path,
        const link_params_t& params);

    ~liberio_link();

    /*!
     * Make a new liberio link.
     *
     * \param tx_path a path string to the TX DMA device
     * \param rx_path a path string to the RX DMA device
     * \param params Values for frame sizes, num frames, and buffer sizes
     * \return a shared_ptr to a new liberio link
     */
    static sptr make(const std::string& tx_path,
        const std::string& rx_path,
        const link_params_t& params);

    /*!
     * Get the physical adapter ID used for this link
     */
    adapter_id_t get_send_adapter_id() const
    {
        return _adapter_id;
    }

    /*!
     * Get the physical adapter ID used for this link
     */
    adapter_id_t get_recv_adapter_id() const
    {
        return _adapter_id;
    }

private:
    using recv_link_base_t = recv_link_base<liberio_link>;
    using send_link_base_t = send_link_base<liberio_link>;

    // Friend declarations to allow base classes to call private methods
    friend recv_link_base_t;
    friend send_link_base_t;

    // Methods called by recv_link_base
    size_t get_recv_buff_derived(frame_buff& buff, int32_t timeout_ms)
    {
        auto& buffer = static_cast<liberio_frame_buff&>(buff);
        return buffer.get(timeout_ms);
    }

    void release_recv_buff_derived(frame_buff& buff)
    {
        auto& buffer = static_cast<liberio_frame_buff&>(buff);
        buffer.release();
    }

    bool get_send_buff_derived(frame_buff& buff, int32_t timeout_ms)
    {
        auto& buffer = static_cast<liberio_frame_buff&>(buff);
        return buffer.get(timeout_ms);
    }

    void release_send_buff_derived(frame_buff& buff)
    {
        auto& buffer = static_cast<liberio_frame_buff&>(buff);
        buffer.release();
    }

    struct liberio_chan* _tx_chan;
    struct liberio_chan* _rx_chan;
    std::vector<liberio_frame_buff> _recv_buffs;
    std::vector<liberio_frame_buff> _send_buffs;
    adapter_id_t _adapter_id;
};

}} // namespace uhd::transport

#endif /* INCLUDED_UHDLIB_TRANSPORT_LIBERIO_LINK_HPP */
