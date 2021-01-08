//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_MOCK_LINK_HPP
#define INCLUDED_MOCK_LINK_HPP

#include <uhd/exception.hpp>
#include <uhdlib/transport/link_base.hpp>
#include <boost/shared_array.hpp>
#include <list>
#include <utility>
#include <vector>

namespace uhd { namespace transport {

/*!
 * Frame buffer for mock link.
 */
class mock_frame_buff : public frame_buff
{
public:
    mock_frame_buff() {}

    void set_mem(boost::shared_array<uint8_t> mem)
    {
        _mem  = mem;
        _data = _mem.get();
    }

    boost::shared_array<uint8_t> get_mem() const
    {
        return _mem;
    }

private:
    boost::shared_array<uint8_t> _mem;
};

/*!
 * Test link that allows the contents of packets sent to be inspected
 * by test code.
 */
class mock_send_link : public send_link_base<mock_send_link>
{
public:
    using sptr = std::shared_ptr<mock_send_link>;

    using base_t = send_link_base<mock_send_link>;

    /*!
     * Parameters for link creation.
     */
    struct link_params
    {
        size_t frame_size;
        size_t num_frames;
    };

    /*!
     * Constructor for mock_send_link. If reuse_send_memory is set to
     * false, the link creates a new memory buffer for every packet, which
     * can be read using the pop_send_packet method. If it is set to true, the
     * same array is used for all frame buffers. The latter is useful when a
     * test requires the same buffer contents to be used repeatedly.
     *
     * \param params specifies the number of frames and frame size for this
     *               link.
     * \param reuse_send_memory configures the link to use the same buffer
     *                          memory region for all frame buffers.
     */
    mock_send_link(const link_params& params, const bool reuse_send_memory = false)
        : base_t(params.num_frames, params.frame_size)
        , _reuse_send_memory(reuse_send_memory)
    {
        _buffs.resize(params.num_frames);

        for (auto& buff : _buffs) {
            base_t::preload_free_buff(&buff);
        }

        if (_reuse_send_memory) {
            // If reusing memory, all buffers will use the same memory region,
            // just preconfigure them here.
            _tx_mems.push_back(
                boost::shared_array<uint8_t>(new uint8_t[params.frame_size]));

            for (auto& buff : _buffs) {
                buff.set_mem(_tx_mems.back());
            }
        }
    }

    /*!
     * Return the number of packets stored in the mock link.
     */
    size_t get_num_packets() const
    {
        return _tx_mems.size();
    }

    /*!
     * Retrieve the contents of a packet sent by the link. The link
     * stores packets in a queue in the order they were sent.
     */
    std::pair<boost::shared_array<uint8_t>, size_t> pop_send_packet()
    {
        UHD_ASSERT_THROW(!_reuse_send_memory);
        UHD_ASSERT_THROW(!_tx_mems.empty());
        UHD_ASSERT_THROW(!_tx_lens.empty());

        auto buff = _tx_mems.front();
        auto len  = _tx_lens.front();

        _tx_mems.pop_front();
        _tx_lens.pop_front();
        return std::make_pair(buff, len);
    }

    /*!
     * Configures the link to simulate a timeout in the next call to
     * get_send_buff.
     */
    void set_simulate_io_timeout(const bool simulate_io_timeout)
    {
        _simulate_io_timeout = simulate_io_timeout;
    }

    adapter_id_t get_send_adapter_id() const override
    {
        return NULL_ADAPTER_ID;
    }

private:
    // Friend declaration to allow base class to call private methods
    friend base_t;

    // Method called by send_link_base
    bool get_send_buff_derived(frame_buff& buff, int32_t /*timeout*/)
    {
        if (_simulate_io_timeout) {
            return false;
        }

        if (!_reuse_send_memory) {
            const size_t size = base_t::get_send_frame_size();
            auto mem          = boost::shared_array<uint8_t>(new uint8_t[size]);

            auto* buff_ptr = static_cast<mock_frame_buff*>(&buff);
            buff_ptr->set_mem(mem);
        }
        return true;
    }

    // Method called by send_link_base
    void release_send_buff_derived(frame_buff& buff)
    {
        if (!_reuse_send_memory) {
            auto* buff_ptr = static_cast<mock_frame_buff*>(&buff);
            _tx_mems.push_back(buff_ptr->get_mem());
            _tx_lens.push_back(buff_ptr->packet_size());
            buff_ptr->set_mem(boost::shared_array<uint8_t>());
        }
    }

    std::vector<mock_frame_buff> _buffs;
    std::list<boost::shared_array<uint8_t>> _tx_mems;
    std::list<size_t> _tx_lens;
    bool _reuse_send_memory;

    bool _simulate_io_timeout = false;
};

/*!
 * Test link that allows the caller to enqueue buffers to be returned
 * by the get_recv_buff method.
 */
class mock_recv_link : public recv_link_base<mock_recv_link>
{
public:
    using sptr = std::shared_ptr<mock_recv_link>;

    using base_t = recv_link_base<mock_recv_link>;

    /*!
     * Parameters for link creation.
     */
    struct link_params
    {
        size_t frame_size;
        size_t num_frames;
    };

    /*!
     * Constructor for mock_recv_link. If reuse_recv_memory is set to
     * false, the link pops an array pushed by push_back_recv_packet
     * in every get_recv_buff call. If it is set to true, the same array
     * is used for all frame buffers. The latter is useful when a test requires
     * the same buffer contents to be used repeatedly.
     *
     * \param params specifies the number of frames and frame size for this
     *               link.
     * \param reuse_recv_memory configures the link to use the same buffer
     *                          memory region for all frame buffers.
     */
    mock_recv_link(const link_params& params, const bool reuse_recv_memory = false)
        : base_t(params.num_frames, params.frame_size)
        , _reuse_recv_memory(reuse_recv_memory)
    {
        _buffs.resize(params.num_frames);

        for (auto& buff : _buffs) {
            base_t::preload_free_buff(&buff);
        }
    }

    /*!
     * Queues a data array to be returned by a future call to get_recv_buff.
     * If reuse_recv_memory is set to false, push a buffer for each call to
     * get_recv_buff. If it is set to true, push a single buffer to be used
     * for all packets.
     */
    void push_back_recv_packet(const boost::shared_array<uint8_t> data, const size_t len)
    {
        _rx_mems.push_back(data);
        _rx_lens.push_back(len);
    }

    adapter_id_t get_recv_adapter_id() const override
    {
        return NULL_ADAPTER_ID;
    }

private:
    // Friend declaration to allow base class to call private methods
    friend base_t;

    // Method called by recv_link_base
    size_t get_recv_buff_derived(frame_buff& buff, int32_t)
    {
        if (_rx_mems.empty()) {
            return 0; // timeout
        }

        auto* buff_ptr = static_cast<mock_frame_buff*>(&buff);
        buff_ptr->set_mem(_rx_mems.front());
        buff_ptr->set_packet_size(_rx_lens.front());

        if (not _reuse_recv_memory) {
            _rx_mems.pop_front();
            _rx_lens.pop_front();
        }

        return buff_ptr->packet_size();
    }

    // Method called by recv_link_base
    void release_recv_buff_derived(frame_buff& buff)
    {
        auto* buff_ptr = static_cast<mock_frame_buff*>(&buff);
        buff_ptr->set_mem(boost::shared_array<uint8_t>());
    }

    std::vector<mock_frame_buff> _buffs;
    std::list<boost::shared_array<uint8_t>> _rx_mems;
    std::list<size_t> _rx_lens;
    bool _reuse_recv_memory;
};

}} // namespace uhd::transport

#endif /*INCLUDED_MOCK_LINK_HPP*/
