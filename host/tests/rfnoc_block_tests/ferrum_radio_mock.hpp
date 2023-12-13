//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "../../lib/usrp/x400/x400_radio_control.hpp"
#include "x4xx_fbx_mpm_mock.hpp"
#include <uhd/rfnoc/mock_block.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/rfnoc/node_accessor.hpp>
#include <iostream>

using namespace uhd;
using namespace uhd::rfnoc;
using namespace std::chrono_literals;
using namespace uhd::usrp::fbx;
using namespace uhd::experts;

// Redeclare this here, since it's only defined outside of UHD_API
noc_block_base::make_args_t::~make_args_t() = default;

namespace {
/* This class extends mock_reg_iface_t by adding a constructor that initializes
 * some of the read memory to contain the memory size for the radio block.
 */
class ferrum_radio_mock_reg_iface_t : public mock_reg_iface_t
{
    // Start address of RFDC control register space
    static constexpr uint32_t rfdc_offset =
        radio_control_impl::regmap::PERIPH_BASE + 0x8000;
    static constexpr uint32_t spi_offset =
        radio_control_impl::regmap::PERIPH_BASE
        + 0xC000 /*DIO Window*/ + 0x2000 /*DIO Regmap*/;
    static constexpr uint32_t gpio_offset =
        radio_control_impl::regmap::PERIPH_BASE
        + 0xC000 /*DIO Window*/ + 0x1000 /*GPIO Regmap*/;

public:
    ferrum_radio_mock_reg_iface_t(size_t num_channels)
    {
        for (size_t chan = 0; chan < num_channels; chan++) {
            const uint32_t reg_compat =
                radio_control_impl::regmap::REG_COMPAT_NUM
                + chan * radio_control_impl::regmap::REG_CHAN_OFFSET;
            read_memory[reg_compat] = (radio_control_impl::MINOR_COMPAT
                                       | (radio_control_impl::MAJOR_COMPAT << 16));
        }
        read_memory[radio_control_impl::regmap::REG_RADIO_WIDTH] =
            (32 /* bits per sample */ << 16) | 1 /* sample per clock */;
        // Ensure that the SPI Status is always SPI_READY
        read_memory[spi_offset + 0x18] |= 1 << 24;

        // Setup the GPIO addresses
        read_memory[gpio_offset + 0x4] = 0;
    }

    void _poke_cb(uint32_t addr, uint32_t data, uhd::time_spec_t, bool) override
    {
        // Are we on the peripheral?
        if (addr >= radio_control_impl::regmap::PERIPH_BASE) {
            // handle all the periphs stuff that is not CPLD here
        } else {
            return;
        }

        // This is the register space that used to be CPLD in ZBX but we shortened it here
        // to just write R/W registers.
        if (addr >= radio_control_impl::regmap::PERIPH_BASE && addr < rfdc_offset) {
            read_memory[addr] = data;
        } else {
            return;
        }

        // Are we poking the RFDC controls?
        if (addr >= rfdc_offset) {
            _poke_rfdc_cb(addr, data);
            return;
        }
    }

    void _poke_rfdc_cb(const uint32_t addr, const uint32_t data)
    {
        read_memory[addr] |= data;
    }
}; // class ferrum_radio_mock_reg_iface_t

/*
 * x400_radio_fixture is a class which is instantiated before each test
 * case is run. It sets up the block container, mock register interface,
 * and x400_radio_control object, all of which are accessible to the test
 * case. The instance of the object is destroyed at the end of each test
 * case.
 */
constexpr size_t DEFAULT_MTU = 8000;

//! Helper class to make sure we get the most logging. The logging level can be
// overridden using a special test-specific environment variable,
// `UHD_UNITTEST_LOG_LEVEL`.
// TODO: Put into own header
struct uhd_log_enabler
{
    uhd_log_enabler(uhd::log::severity_level level)
    {
        const char* env_p = std::getenv("UHD_UNITTEST_LOG_LEVEL");
        if (env_p) {
            auto parsed_level = uhd::log::parse_log_level_from_string(env_p);
            if (parsed_level) {
                level = *parsed_level;
            } else {
                std::cout << "Unable to parse UHD_UNITTEST_LOG_LEVEL " << env_p
                          << std::endl;
            }
        }

        std::cout << "Setting log level to " << level << "..." << std::endl;
        uhd::log::set_log_level(level);
        uhd::log::set_console_level(level);
        std::this_thread::sleep_for(10ms);
    }
};

struct x400_radio_fixture
{
    x400_radio_fixture()
        : ule(uhd::log::trace) // Note: When debugging this test, either set
                               // this to a lower level, or create a
                               // uhd_log_enabler in the test-under-test
        , num_channels(uhd::usrp::fbx::FBX_MAX_NUM_CHANS)
        , num_input_ports(num_channels)
        , num_output_ports(num_channels)
        , reg_iface(std::make_shared<ferrum_radio_mock_reg_iface_t>(num_channels))
        , rpcs(std::make_shared<uhd::test::ferrum_mock_rpc_server>(device_info))
        , mbc(std::make_shared<mpmd_mb_controller>(rpcs, device_info))
        , block_container(get_mock_block(RADIO_BLOCK,
              num_channels,
              num_channels,
              device_info,
              DEFAULT_MTU,
              X400,
              reg_iface,
              mbc))
        , test_radio(block_container.get_block<x400_radio_control_impl>())
    {
        node_accessor.init_props(test_radio.get());
    }

    ~x400_radio_fixture() {}


    // Must remain the first member so we make sure the log level is high
    uhd_log_enabler ule;
    const size_t num_channels;
    const size_t num_input_ports;
    const size_t num_output_ports;
    uhd::device_addr_t device_info = uhd::device_addr_t("master_clock_rate=4e9");
    std::shared_ptr<ferrum_radio_mock_reg_iface_t> reg_iface;
    std::shared_ptr<uhd::test::ferrum_mock_rpc_server> rpcs;
    mpmd_mb_controller::sptr mbc;

    mock_block_container block_container;
    std::shared_ptr<x400_radio_control_impl> test_radio;
    node_accessor_t node_accessor{};
};
} // namespace
