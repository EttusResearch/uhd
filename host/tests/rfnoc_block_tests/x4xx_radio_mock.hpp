//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "../../lib/usrp/x400/x400_radio_control.hpp"
#include "x4xx_zbx_mpm_mock.hpp"
#include <uhd/rfnoc/mock_block.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/rfnoc/node_accessor.hpp>
#include <iostream>

using namespace uhd;
using namespace uhd::rfnoc;
using namespace std::chrono_literals;
using namespace uhd::usrp::zbx;
using namespace uhd::experts;

// Redeclare this here, since it's only defined outside of UHD_API
noc_block_base::make_args_t::~make_args_t() = default;

namespace {
/* This class extends mock_reg_iface_t by adding a constructor that initializes
 * some of the read memory to contain the memory size for the radio block.
 */
class x4xx_radio_mock_reg_iface_t : public mock_reg_iface_t
{
    // Start address of CPLD register space
    static constexpr uint32_t cpld_offset = radio_control_impl::regmap::PERIPH_BASE;
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
    x4xx_radio_mock_reg_iface_t(size_t num_channels)
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

        // Are we on the CPLD?
        if (addr >= cpld_offset && addr < rfdc_offset) {
            _poke_cpld_cb(addr, data);
            return;
        }

        // Are we poking the RFDC controls?
        if (addr >= rfdc_offset) {
            _poke_rfdc_cb(addr, data);
            return;
        }
    }

    void _poke_cpld_cb(const uint32_t addr, const uint32_t data)
    {
        switch (addr - cpld_offset) {
            /// CURRENT_CONFIG_REG
            case 0x1000:
                // FIXME: We write to all regs during init
                // BOOST_REQUIRE(false); // Not a write-register
                break;
            /// SW_CONFIG
            case 0x1008: {
                // This register is RW so update read_memory
                read_memory[addr] = data;
                // If we're in SW-defined mode, also update CURRENT_CONFIG_REG
                uint32_t& rf_opt = read_memory[cpld_offset + 0x1004];
                uint32_t& ccr    = read_memory[cpld_offset + 0x1000];
                // Check if RF0_OPTION is SW_DEFINED
                if ((rf_opt & 0x00FF) == 0) {
                    ccr = (ccr & 0xFF00) | (data & 0x00FF);
                }
                // Check if RF1_OPTION is SW_DEFINED
                if ((rf_opt & 0xFF00) == 0) {
                    ccr = (ccr & 0x00FF) | (data & 0xFF00);
                }
            } break;
            /// LO SPI transactions
            case 0x1020:
                _poke_lo_spi(addr, data);
                return;
            /// LO SYNC
            case 0x1024:
                // We make these bits sticky, because they might get strobed in
                // multiple calls. In order to see what was strobed within an
                // API call, we keep bits as they are.
                read_memory[addr] |= data;
                return;
            // TX0 Table Select
            case 0x4000:
            case 0x4004:
            case 0x4008:
            case 0x400C:
            case 0x4010:
            case 0x4014: {
                read_memory[addr]               = data;
                const uint32_t src_table_offset = data * 4;
                const uint32_t dst_table_offset = (addr - cpld_offset) - 0x4000;
                // Now we fake the transaction that copies ?X?_TABLE_* to
                // ?X?_DSA*
                read_memory[cpld_offset + 0x3000 + dst_table_offset] =
                    read_memory[cpld_offset + 0x5000 + src_table_offset];
            }
                return;
            // RX0 Table Select
            case 0x4800:
            case 0x4804:
            case 0x4808:
            case 0x480C:
            case 0x4810:
            case 0x4814: {
                read_memory[addr]               = data;
                const uint32_t src_table_offset = data * 4;
                const uint32_t dst_table_offset = (addr - cpld_offset) - 0x4800;
                // Now we fake the transaction that copies ?X?_TABLE_* to
                // ?X?_DSA*
                read_memory[cpld_offset + 0x3800 + dst_table_offset] =
                    read_memory[cpld_offset + 0x5800 + src_table_offset];
            }
                return;
            default: // All other CPLD registers are read-write
                read_memory[addr] = data;
                return;
        }
    }

    void _poke_rfdc_cb(const uint32_t addr, const uint32_t data)
    {
        read_memory[addr] |= data;
    }

    void _poke_lo_spi(const uint32_t addr, const uint32_t data)
    {
        // UHD_LOG_INFO("TEST", "Detected LO SPI transaction!");
        const uint16_t spi_data = data & 0xFFFF;
        const uint8_t spi_addr  = (data >> 16) & 0x7F;
        const bool read         = bool(data & (1 << 23));
        const uint8_t lo_sel    = (data >> 24) & 0x7;
        const bool start_xact   = bool(data & (1 << 28));
        // UHD_LOG_INFO("TEST",
        //     "Transaction record: Read: "
        //         << (read ? "yes" : "no") << " Address: " << int(spi_addr) << std::hex
        //         << " Data: 0x" << spi_data << " LO sel: " << int(lo_sel) << std::dec
        //         << " Start Transaction: " << start_xact);
        if (!start_xact) {
            // UHD_LOG_INFO("TEST", "Register probably just initialized. Ignoring.");
            return;
        }
        switch (spi_addr) {
            case 0:
                _muxout_to_lock = spi_data & (1 << 2);
                break;
            case 125:
                BOOST_REQUIRE(read);
                read_memory[addr] = 0x2288;
                break;
            default:
                break;
        }
        if (read) {
            read_memory[addr] = (read_memory[addr] & 0xFFFF) | (spi_addr << 16)
                                | (lo_sel << 24) | (1 << 31);
        }
        if (_muxout_to_lock) {
            // UHD_LOG_INFO("TEST", "Muxout set to lock. Returning all ones.");
            read_memory[addr] = 0xFFFF;
            return;
        }
        return;
    }

    bool _muxout_to_lock = false;
}; // class x4xx_radio_mock_reg_iface_t

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
                std::cout << "Unable to parse UHD_UNITTEST_LOG_LEVEL " << env_p << std::endl;
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
        , num_channels(uhd::usrp::zbx::ZBX_NUM_CHANS)
        , num_input_ports(num_channels)
        , num_output_ports(num_channels)
        , reg_iface(std::make_shared<x4xx_radio_mock_reg_iface_t>(num_channels))
        , rpcs(std::make_shared<uhd::test::x4xx_mock_rpc_server>(device_info))
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
    uhd::device_addr_t device_info = uhd::device_addr_t("master_clock_rate=122.88e6");
    std::shared_ptr<x4xx_radio_mock_reg_iface_t> reg_iface;
    std::shared_ptr<uhd::test::x4xx_mock_rpc_server> rpcs;
    mpmd_mb_controller::sptr mbc;

    mock_block_container block_container;
    std::shared_ptr<x400_radio_control_impl> test_radio;
    node_accessor_t node_accessor{};
};
} // namespace
