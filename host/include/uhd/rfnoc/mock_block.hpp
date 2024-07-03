//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/mb_controller.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>
#include <uhd/rfnoc/register_iface.hpp>
#include <uhd/types/time_spec.hpp>
#include <uhd/utils/log.hpp>
#include <unordered_map>
#include <boost/format.hpp>
#include <vector>

namespace uhd { namespace rfnoc {

/*! Mock version of a register interface
 *
 * This can be used for mock blocks, usually for the sake of unit testing.
 */
class UHD_API mock_reg_iface_t : public register_iface
{
public:
    mock_reg_iface_t()           = default;
    ~mock_reg_iface_t() override = default;

    /**************************************************************************
     * API
     *************************************************************************/
    void poke32(uint32_t addr, uint32_t data, uhd::time_spec_t time, bool ack) override
    {
        write_memory[addr] = data;
        _poke_cb(addr, data, time, ack);
    }

    void multi_poke32(const std::vector<uint32_t> addrs,
        const std::vector<uint32_t> data,
        uhd::time_spec_t time,
        bool ack) override
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
        bool ack) override
    {
        for (size_t i = 0; i < data.size(); i++) {
            poke32(first_addr + 4 * i, data[i], timestamp, ack);
        }
    }

    uint32_t peek32(uint32_t addr, uhd::time_spec_t time) override
    {
        _peek_cb(addr, time);
        try {
            return read_memory.at(addr);
        } catch (const std::out_of_range&) {
            throw uhd::runtime_error(
                str(boost::format("No data defined for address: 0x%04X") % addr));
        }
    }

    std::vector<uint32_t> block_peek32(
        uint32_t first_addr, size_t length, uhd::time_spec_t time) override
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
        bool /* ack */        = false) override
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

    void sleep(uhd::time_spec_t /*duration*/, bool /*ack*/) override
    {
        // nop
    }

    void register_async_msg_validator(async_msg_validator_t /*callback_f*/) override
    {
        // nop
    }

    void register_async_msg_handler(async_msg_callback_t /*callback_f*/) override
    {
        // nop
    }

    void set_policy(const std::string& name, const uhd::device_addr_t& args) override
    {
        UHD_LOG_INFO("MOCK_REG_IFACE",
            "Requested to set policy for " << name << " to " << args.to_string());
    }

    uint16_t get_src_epid() const override
    {
        return 0;
    }

    uint16_t get_port_num() const override
    {
        return 0;
    }

    void define_custom_register_space(const uint32_t /*start_addr*/,
        const uint32_t /*length*/,
        std::function<void(uint32_t, uint32_t)> /*poke_fn*/,
        std::function<uint32_t(uint32_t)> /*peek_fn*/) override
    {
        // nop
    }

    bool force_timeout = false;

    //! All pokes end up writing to this map
    std::unordered_map<uint32_t, uint32_t> read_memory;
    //! All peeks read from this map. A peek will fail if the address has not
    // been previously set.
    std::unordered_map<uint32_t, uint32_t> write_memory;

protected:
    virtual void _poke_cb(
        uint32_t /*addr*/, uint32_t /*data*/, uhd::time_spec_t /*time*/, bool /*ack*/)
    {
    }

    virtual void _peek_cb(uint32_t /*addr*/, uhd::time_spec_t /*time*/) {}
};

/*! Container for all the items required for running a mock block
 */
struct UHD_API mock_block_container
{
    friend class get_mock_block;
    //! Reference to the register interface object
    std::shared_ptr<mock_reg_iface_t> reg_iface;

    //! Reference to the prop tree object the block sees
    uhd::property_tree::sptr tree;

    //! Use this to retrieve a reference to the block controller. Make sure that
    // the register space is appropiately primed before doing so.
    template <typename block_type = noc_block_base>
    std::shared_ptr<block_type> get_block()
    {
        return std::dynamic_pointer_cast<block_type>(factory(std::move(make_args)));
    }

    //! Factory to get the block. Use get_block() instead.
    std::function<noc_block_base::sptr(noc_block_base::make_args_ptr)> factory;

    // Note: The make args would ideally be captured by the factory function,
    // but std::functions need to be CopyConstructible, and this struct doesn't,
    // so it needs to live out here in the open.
    noc_block_base::make_args_ptr make_args;
};

/*! Factory function for mock block controllers
 */
UHD_API mock_block_container get_mock_block(const noc_id_t noc_id,
    const size_t num_inputs                            = 1,
    const size_t num_outputs                           = 1,
    const uhd::device_addr_t& args                     = uhd::device_addr_t(),
    const size_t mtu                                   = 8000,
    const device_type_t device_id                      = ANY_DEVICE,
    std::shared_ptr<mock_reg_iface_t> client_reg_iface = nullptr,
    mb_controller::sptr mbc                            = nullptr);

}}; // namespace uhd::rfnoc
