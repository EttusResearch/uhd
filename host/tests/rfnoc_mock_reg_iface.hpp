//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_TESTS_MOCK_REG_IFACE_HPP
#define INCLUDED_LIBUHD_TESTS_MOCK_REG_IFACE_HPP

#include <uhd/rfnoc/register_iface.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/types/time_spec.hpp>
#include <boost/format.hpp>
#include <unordered_map>
#include <vector>

class mock_reg_iface_t : public uhd::rfnoc::register_iface
{
public:
    mock_reg_iface_t()          = default;
    virtual ~mock_reg_iface_t() = default;

    /**************************************************************************
     * API
     *************************************************************************/
    void poke32(uint32_t addr, uint32_t data, uhd::time_spec_t time, bool ack)
    {
        write_memory[addr] = data;
        _poke_cb(addr, data, time, ack);
    }

    void multi_poke32(const std::vector<uint32_t> addrs,
        const std::vector<uint32_t> data,
        uhd::time_spec_t time,
        bool ack)
    {
        if (addrs.size() != data.size()) {
            throw uhd::value_error("addrs and data vectors must be of the same length");
        }
        for (size_t i = 0; i < addrs.size(); i++) {
            poke32(addrs[i], data[i], time, ack);
        }
    }

    void block_poke32(uint32_t first_addr,
        const std::vector<uint32_t> data,
        uhd::time_spec_t timestamp,
        bool ack)
    {
        for (size_t i = 0; i < data.size(); i++) {
            poke32(first_addr + 4 * i, data[i], timestamp, ack);
        }
    }

    uint32_t peek32(uint32_t addr, uhd::time_spec_t time)
    {
        _peek_cb(addr, time);
        try {
            return read_memory.at(addr);
        } catch (const std::out_of_range&) {
            throw uhd::runtime_error(str(boost::format("No data defined for address: 0x%04X") % addr));
        }
    }

    std::vector<uint32_t> block_peek32(
        uint32_t first_addr, size_t length, uhd::time_spec_t time)
    {
        std::vector<uint32_t> result(length, 0);
        for (size_t i = 0; i < length; ++i) {
            result[i] = peek32(first_addr + i * 4, time);
        }
        return result;
    }

    void poll32(uint32_t addr,
        uint32_t data,
        uint32_t mask,
        uhd::time_spec_t /* timeout */,
        uhd::time_spec_t time = uhd::time_spec_t::ASAP,
        bool /* ack */        = false)
    {
        if (force_timeout) {
            throw uhd::op_timeout("timeout");
        }

        if ((peek32(addr, time) & mask) == data) {
            UHD_LOG_INFO("MOCK_REG_IFACE", "poll32() successful at addr " << addr);
        } else {
            UHD_LOG_INFO("MOCK_REG_IFACE", "poll32() not successful at addr " << addr);
        }
    }

    void sleep(uhd::time_spec_t /*duration*/, bool /*ack*/)
    {
        // nop
    }

    void register_async_msg_handler(async_msg_callback_t /*callback_f*/)
    {
        // nop
    }

    void set_policy(const std::string& name, const uhd::device_addr_t& args)
    {
        UHD_LOG_INFO("MOCK_REG_IFACE",
            "Requested to set policy for " << name << " to " << args.to_string());
    }


    bool force_timeout = false;

    std::unordered_map<uint32_t, uint32_t> read_memory;
    std::unordered_map<uint32_t, uint32_t> write_memory;

protected:
    virtual void _poke_cb(uint32_t /*addr*/, uint32_t /*data*/, uhd::time_spec_t /*time*/, bool /*ack*/)
    {
    }
    virtual void _peek_cb(uint32_t /*addr*/, uhd::time_spec_t /*time*/) {}
};


#endif /* INCLUDED_LIBUHD_TESTS_MOCK_REG_IFACE_HPP */

