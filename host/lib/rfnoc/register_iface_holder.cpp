//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/register_iface_holder.hpp>
#include <uhd/utils/log.hpp>

using namespace uhd::rfnoc;

/*! Special type of register interface: Invalidated interface
 *
 * This interface does nothing, other than log error messages and implement
 * register_iface. It is meant to be used as a replacement for another
 * register_iface when that interface is no longer accessible.
 * Because this should be safely usable in a destructor, it never throws.
 */
class invalid_register_iface : public register_iface
{
public:
    ~invalid_register_iface() override = default;

    void poke32(uint32_t, uint32_t, uhd::time_spec_t, bool) override
    {
        UHD_LOG_ERROR("REGS", "Attempting to use invalidated register interface!");
    }

    void multi_poke32(const std::vector<uint32_t>,
        const std::vector<uint32_t>,
        uhd::time_spec_t,
        bool) override
    {
        UHD_LOG_ERROR("REGS", "Attempting to use invalidated register interface!");
    }

    void block_poke32(
        uint32_t, const std::vector<uint32_t>, uhd::time_spec_t, bool) override
    {
        UHD_LOG_ERROR("REGS", "Attempting to use invalidated register interface!");
    }

    uint32_t peek32(uint32_t, uhd::time_spec_t) override
    {
        UHD_LOG_ERROR("REGS", "Attempting to use invalidated register interface!");
        return {};
    }

    std::vector<uint32_t> block_peek32(uint32_t, size_t, uhd::time_spec_t) override
    {
        UHD_LOG_ERROR("REGS", "Attempting to use invalidated register interface!");
        return {};
    }

    void poll32(
        uint32_t, uint32_t, uint32_t, uhd::time_spec_t, uhd::time_spec_t, bool) override
    {
        UHD_LOG_ERROR("REGS", "Attempting to use invalidated register interface!");
    }

    void sleep(uhd::time_spec_t, bool) override
    {
        UHD_LOG_ERROR("REGS", "Attempting to use invalidated register interface!");
    }

    void register_async_msg_handler(async_msg_callback_t) override
    {
        UHD_LOG_ERROR("REGS", "Attempting to use invalidated register interface!");
    }

    void register_async_msg_validator(async_msg_validator_t) override
    {
        UHD_LOG_ERROR("REGS", "Attempting to use invalidated register interface!");
    }

    void set_policy(const std::string&, const uhd::device_addr_t&) override
    {
        UHD_LOG_ERROR("REGS", "Attempting to use invalidated register interface!");
    }

    uint16_t get_src_epid() const override
    {
        UHD_LOG_ERROR("REGS", "Attempting to use invalidated register interface!");
        return 0;
    }

    uint16_t get_port_num() const override
    {
        UHD_LOG_ERROR("REGS", "Attempting to use invalidated register interface!");
        return 0;
    }

    void define_custom_register_space(uint32_t,
        uint32_t,
        std::function<void(uint32_t, uint32_t)>,
        std::function<uint32_t(uint32_t)>) override
    {
        UHD_LOG_ERROR("REGS", "Attempting to use invalidated register interface!");
    }
}; // class invalid_register_iface

void register_iface_holder::update_reg_iface(register_iface::sptr new_iface)
{
    if (new_iface) {
        _reg = new_iface;
    } else {
        _reg = std::make_shared<invalid_register_iface>();
    }
}
