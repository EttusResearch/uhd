//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhdlib/rfnoc/chdr_packet_writer.hpp>
#include <cassert>
#include <functional>
#include <memory>

using namespace uhd;
using namespace uhd::rfnoc;
using namespace uhd::rfnoc::chdr;

chdr_packet_writer::~chdr_packet_writer() = default;

//------------------------------------------------------------
// chdr_packet
//------------------------------------------------------------
// endianness is the link endianness, not the host endianness
template <size_t chdr_w, endianness_t endianness>
class chdr_packet_impl : public chdr_packet_writer
{
public:
    chdr_packet_impl() = delete;
    chdr_packet_impl(size_t mtu_bytes) : _mtu_bytes(mtu_bytes) {}
    ~chdr_packet_impl() override = default;

    void refresh(const void* pkt_buff) const override
    {
        assert(pkt_buff);
        _pkt_buff = const_cast<uint64_t*>(reinterpret_cast<const uint64_t*>(pkt_buff));
        _mdata_offset = _compute_mdata_offset(get_chdr_header());
    }

    void refresh(void* pkt_buff, chdr_header& header, uint64_t timestamp = 0) override
    {
        assert(pkt_buff);
        _pkt_buff    = reinterpret_cast<uint64_t*>(pkt_buff);
        _pkt_buff[0] = u64_from_host(header);
        if (_has_timestamp(header)) {
            _pkt_buff[1] = u64_from_host(timestamp);
        }
        _mdata_offset = _compute_mdata_offset(get_chdr_header());
    }

    void update_payload_size(size_t payload_size_bytes) override
    {
        chdr_header header = get_chdr_header();
        header.set_length(((_mdata_offset + header.get_num_mdata()) * chdr_w_bytes)
                          + payload_size_bytes);
        _pkt_buff[0] = u64_from_host(header);
    }

    endianness_t get_byte_order() const override
    {
        return endianness;
    }

    size_t get_mtu_bytes() const override
    {
        return _mtu_bytes;
    }

    chdr_header get_chdr_header() const override
    {
        assert(_pkt_buff);
        return chdr_header(u64_to_host(_pkt_buff[0]));
    }

    boost::optional<uint64_t> get_timestamp() const override
    {
        if (_has_timestamp(get_chdr_header())) {
            // In a unit64_t buffer, the timestamp is always immediately after the header
            // regardless of chdr_w.
            return u64_to_host(_pkt_buff[1]);
        } else {
            return boost::none;
        }
    }

    size_t get_mdata_size() const override
    {
        return get_chdr_header().get_num_mdata() * chdr_w_bytes;
    }

    const void* get_mdata_const_ptr() const override
    {
        return const_cast<void*>(
            const_cast<chdr_packet_impl<chdr_w, endianness>*>(this)->get_mdata_ptr());
    }

    void* get_mdata_ptr() override
    {
        return reinterpret_cast<void*>(_pkt_buff + (chdr_w_stride * _mdata_offset));
    }

    size_t get_payload_size() const override
    {
        return get_chdr_header().get_length() - get_mdata_size()
               - (chdr_w_bytes * _mdata_offset);
    }

    const void* get_payload_const_ptr() const override
    {
        return const_cast<void*>(
            const_cast<chdr_packet_impl<chdr_w, endianness>*>(this)->get_payload_ptr());
    }

    void* get_payload_ptr() override
    {
        return reinterpret_cast<void*>(
            _pkt_buff
            + (chdr_w_stride * (_mdata_offset + get_chdr_header().get_num_mdata())));
    }

    size_t calculate_payload_offset(
        const packet_type_t pkt_type, const uint8_t num_mdata = 0) const override
    {
        chdr_header header;
        header.set_pkt_type(pkt_type);
        return (_compute_mdata_offset(header) + num_mdata) * chdr_w_bytes;
    }

private:
    inline bool _has_timestamp(const chdr_header& header) const
    {
        return (header.get_pkt_type() == PKT_TYPE_DATA_WITH_TS);
    }

    inline size_t _compute_mdata_offset(const chdr_header& header) const
    {
        // The metadata offset depends on the chdr_w and whether we have a timestamp
        if (chdr_w == 64) {
            return _has_timestamp(header) ? 2 : 1;
        } else {
            return 1;
        }
    }

    inline static uint64_t u64_to_host(uint64_t word)
    {
        return (endianness == ENDIANNESS_BIG) ? uhd::ntohx<uint64_t>(word)
                                              : uhd::wtohx<uint64_t>(word);
    }

    inline static uint64_t u64_from_host(uint64_t word)
    {
        return (endianness == ENDIANNESS_BIG) ? uhd::htonx<uint64_t>(word)
                                              : uhd::htowx<uint64_t>(word);
    }

    static const size_t chdr_w_bytes  = (chdr_w / 8);
    static const size_t chdr_w_stride = (chdr_w / 64);

    // Packet state
    const size_t _mtu_bytes      = 0;
    mutable uint64_t* _pkt_buff  = nullptr;
    mutable size_t _mdata_offset = 0;
};

chdr_packet_factory::chdr_packet_factory(chdr_w_t chdr_w, endianness_t endianness)
    : _chdr_w(chdr_w), _endianness(endianness)
{
}

chdr_packet_writer::uptr chdr_packet_factory::make_generic(size_t mtu_bytes) const
{
    if (_endianness == ENDIANNESS_BIG) {
        switch (_chdr_w) {
            case CHDR_W_512:
                return std::make_unique<chdr_packet_impl<512, ENDIANNESS_BIG>>(mtu_bytes);
            case CHDR_W_256:
                return std::make_unique<chdr_packet_impl<256, ENDIANNESS_BIG>>(mtu_bytes);
            case CHDR_W_128:
                return std::make_unique<chdr_packet_impl<128, ENDIANNESS_BIG>>(mtu_bytes);
            case CHDR_W_64:
                return std::make_unique<chdr_packet_impl<64, ENDIANNESS_BIG>>(mtu_bytes);
            default:
                assert(0);
        }
    } else {
        switch (_chdr_w) {
            case CHDR_W_512:
                return std::make_unique<chdr_packet_impl<512, ENDIANNESS_LITTLE>>(
                    mtu_bytes);
            case CHDR_W_256:
                return std::make_unique<chdr_packet_impl<256, ENDIANNESS_LITTLE>>(
                    mtu_bytes);
            case CHDR_W_128:
                return std::make_unique<chdr_packet_impl<128, ENDIANNESS_LITTLE>>(
                    mtu_bytes);
            case CHDR_W_64:
                return std::make_unique<chdr_packet_impl<64, ENDIANNESS_LITTLE>>(
                    mtu_bytes);
            default:
                assert(0);
        }
    }
    return chdr_packet_writer::uptr();
}

chdr_ctrl_packet::uptr chdr_packet_factory::make_ctrl(size_t mtu_bytes) const
{
    return std::make_unique<chdr_ctrl_packet>(make_generic(mtu_bytes));
}

chdr_strs_packet::uptr chdr_packet_factory::make_strs(size_t mtu_bytes) const
{
    return std::make_unique<chdr_strs_packet>(make_generic(mtu_bytes));
}

chdr_strc_packet::uptr chdr_packet_factory::make_strc(size_t mtu_bytes) const
{
    return std::make_unique<chdr_strc_packet>(make_generic(mtu_bytes));
}

chdr_mgmt_packet::uptr chdr_packet_factory::make_mgmt(size_t mtu_bytes) const
{
    return std::make_unique<chdr_mgmt_packet>(make_generic(mtu_bytes));
}
