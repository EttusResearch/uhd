//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_MOCK_XPORT_HPP
#define INCLUDED_MOCK_XPORT_HPP

#include <uhd/exception.hpp>
#include <uhd/transport/chdr.hpp>
#include <uhd/transport/vrt_if_packet.hpp>
#include <uhd/transport/zero_copy.hpp>
#include <uhd/types/endianness.hpp>
#include <uhd/utils/byteswap.hpp>
#include <boost/shared_array.hpp>
#include <list>
#include <memory>
#include <vector>

/***********************************************************************
 * Transport mockups
 **********************************************************************/
/*! A single transport class that implements send() and recv()
 *
 * Tx and Rx are separate. We can access the other end of the FIFOs from
 * this class.
 */
static constexpr size_t DEFAULT_SEND_FRAME_SIZE = 1024;
static constexpr size_t DEFAULT_RECV_FRAME_SIZE = 1024;

/***********************************************************************
 * Dummy managed buffers for testing
 **********************************************************************/
class mock_msb : public uhd::transport::managed_send_buffer
{
public:
    void release(void) override
    { /* nop */
    }

    sptr get_new(boost::shared_array<uint8_t> mem, size_t* len)
    {
        _mem = mem;
        return make(this, mem.get(), *len);
    }

private:
    boost::shared_array<uint8_t> _mem;
};


class mock_mrb : public uhd::transport::managed_recv_buffer
{
public:
    void release(void) override
    { /* nop */
    }

    sptr get_new(boost::shared_array<uint8_t> mem, size_t len)
    {
        _mem = mem;
        return make(this, _mem.get(), len);
    }

private:
    boost::shared_array<uint8_t> _mem;
};

class mock_zero_copy : public uhd::transport::zero_copy_if
{
public:
    typedef std::shared_ptr<mock_zero_copy> sptr;

    mock_zero_copy(uhd::transport::vrt::if_packet_info_t::link_type_t type,
        size_t recv_frame_size = DEFAULT_RECV_FRAME_SIZE,
        size_t send_frame_size = DEFAULT_SEND_FRAME_SIZE);

    uhd::transport::managed_recv_buffer::sptr get_recv_buff(double) override;
    uhd::transport::managed_send_buffer::sptr get_send_buff(double) override;

    size_t get_num_recv_frames(void) const override
    {
        return 1;
    }
    size_t get_num_send_frames(void) const override
    {
        return 1;
    }
    size_t get_recv_frame_size(void) const override
    {
        return _recv_frame_size;
    }
    size_t get_send_frame_size(void) const override
    {
        return _send_frame_size;
    }

    template <typename T>
    void push_back_packet(uhd::transport::vrt::if_packet_info_t& ifpi,
        const std::vector<T>& otw_data = std::vector<T>(),
        uhd::endianness_t endianness   = uhd::ENDIANNESS_BIG);

    void set_reuse_recv_memory(bool reuse_recv);
    void set_reuse_send_memory(bool reuse_send);

    void set_simulate_io_error(bool status)
    {
        _simulate_io_error = status;
    }

    template <typename T, uhd::endianness_t endianness = uhd::ENDIANNESS_BIG>
    void push_back_recv_packet(
        uhd::transport::vrt::if_packet_info_t& ifpi, const std::vector<T>& otw_data);

    template <uhd::endianness_t endianness = uhd::ENDIANNESS_BIG>
    void push_back_inline_message_packet(
        uhd::transport::vrt::if_packet_info_t& ifpi, const uint32_t message);

    template <uhd::endianness_t endianness = uhd::ENDIANNESS_BIG>
    void push_back_flow_ctrl_packet(
        uhd::transport::vrt::if_packet_info_t::packet_type_t type,
        uint32_t packet_count,
        uint32_t byte_count);

    template <uhd::endianness_t endianness = uhd::ENDIANNESS_BIG>
    void pop_send_packet(uhd::transport::vrt::if_packet_info_t& ifpi);

private:
    std::list<boost::shared_array<uint8_t>> _tx_mems;
    std::list<size_t> _tx_lens;

    std::list<boost::shared_array<uint8_t>> _rx_mems;
    std::list<size_t> _rx_lens;

    mock_msb _msb;
    mock_mrb _mrb;

    uhd::transport::vrt::if_packet_info_t::link_type_t _link_type;
    size_t _recv_frame_size = DEFAULT_RECV_FRAME_SIZE;
    size_t _send_frame_size = DEFAULT_RECV_FRAME_SIZE;

    bool _simulate_io_error = false;

    bool _reuse_recv_memory = false;
    bool _reuse_send_memory = false;
};

template <typename T, uhd::endianness_t endianness>
void mock_zero_copy::push_back_recv_packet(
    uhd::transport::vrt::if_packet_info_t& ifpi, const std::vector<T>& otw_data)
{
    using namespace uhd::transport;

    UHD_ASSERT_THROW(
        ifpi.num_payload_words32 * sizeof(uint32_t) == otw_data.size() * sizeof(T));

    const size_t max_hdr_len =
        _link_type == vrt::if_packet_info_t::LINK_TYPE_CHDR
            ? vrt::chdr::max_if_hdr_words64 * sizeof(uint64_t)
            : (vrt::max_if_hdr_words32 + 1 /*tlr*/) * sizeof(uint32_t);

    const size_t max_pkt_len = ifpi.num_payload_words32 * sizeof(uint32_t) + max_hdr_len;

    UHD_ASSERT_THROW(max_pkt_len <= _recv_frame_size);

    // Create recv buffer
    _rx_mems.push_back(boost::shared_array<uint8_t>(new uint8_t[max_pkt_len]));
    uint32_t* rx_buff_ptr = reinterpret_cast<uint32_t*>(_rx_mems.back().get());

    // Copy header
    if (endianness == uhd::ENDIANNESS_BIG) {
        if (_link_type == vrt::if_packet_info_t::LINK_TYPE_CHDR) {
            uhd::transport::vrt::chdr::if_hdr_pack_be(rx_buff_ptr, ifpi);
        } else {
            uhd::transport::vrt::if_hdr_pack_be(rx_buff_ptr, ifpi);
        }
    } else {
        if (_link_type == vrt::if_packet_info_t::LINK_TYPE_CHDR) {
            uhd::transport::vrt::chdr::if_hdr_pack_le(rx_buff_ptr, ifpi);
        } else {
            uhd::transport::vrt::if_hdr_pack_le(rx_buff_ptr, ifpi);
        }
    }

    // Copy data
    uint32_t* data_ptr = (rx_buff_ptr + ifpi.num_header_words32);
    std::copy(otw_data.begin(), otw_data.end(), reinterpret_cast<T*>(data_ptr));
    _rx_lens.push_back(ifpi.num_packet_words32 * sizeof(uint32_t));
}

template <uhd::endianness_t endianness>
void mock_zero_copy::push_back_inline_message_packet(
    uhd::transport::vrt::if_packet_info_t& ifpi, const uint32_t message)
{
    const std::vector<uint32_t> data{message | uhd::byteswap(message)};
    push_back_recv_packet<uint32_t, endianness>(ifpi, data);
}


template <uhd::endianness_t endianness>
void mock_zero_copy::pop_send_packet(uhd::transport::vrt::if_packet_info_t& ifpi)
{
    using namespace uhd::transport;

    ifpi.num_packet_words32 = _tx_lens.front() / sizeof(uint32_t);

    uint32_t* tx_buff_ptr = reinterpret_cast<uint32_t*>(_tx_mems.front().get());

    if (endianness == uhd::ENDIANNESS_BIG) {
        if (_link_type == vrt::if_packet_info_t::LINK_TYPE_CHDR) {
            uhd::transport::vrt::chdr::if_hdr_unpack_be(tx_buff_ptr, ifpi);
        } else {
            uhd::transport::vrt::if_hdr_unpack_be(tx_buff_ptr, ifpi);
        }
    } else {
        if (_link_type == vrt::if_packet_info_t::LINK_TYPE_CHDR) {
            uhd::transport::vrt::chdr::if_hdr_unpack_le(tx_buff_ptr, ifpi);
        } else {
            uhd::transport::vrt::if_hdr_unpack_le(tx_buff_ptr, ifpi);
        }
    }
    _tx_mems.pop_front();
    _tx_lens.pop_front();
}

template <uhd::endianness_t endianness>
void mock_zero_copy::push_back_flow_ctrl_packet(
    uhd::transport::vrt::if_packet_info_t::packet_type_t type,
    uint32_t packet_count,
    uint32_t byte_count)
{
    using namespace uhd::transport;

    UHD_ASSERT_THROW(type == vrt::if_packet_info_t::PACKET_TYPE_FC
                     or type == vrt::if_packet_info_t::PACKET_TYPE_ACK);

    // Only implemented for chdr packets currently
    UHD_ASSERT_THROW(_link_type == vrt::if_packet_info_t::LINK_TYPE_CHDR);

    const size_t packet_len_in_words32 = 2;

    vrt::if_packet_info_t ifpi;
    ifpi.packet_type         = type;
    ifpi.num_payload_words32 = packet_len_in_words32;
    ifpi.num_payload_bytes   = ifpi.num_payload_words32 * sizeof(uint32_t);
    ifpi.has_tsf             = false;

    std::vector<uint32_t> data(packet_len_in_words32, 0);

    if (endianness == uhd::ENDIANNESS_BIG) {
        data[0] = uhd::ntohx(packet_count);
        data[1] = uhd::ntohx(byte_count);
    } else {
        data[0] = uhd::wtohx(packet_count);
        data[1] = uhd::wtohx(byte_count);
    }

    push_back_recv_packet<uint32_t, endianness>(ifpi, data);
}

#endif /*INCLUDED_MOCK_XPORT_HPP*/
