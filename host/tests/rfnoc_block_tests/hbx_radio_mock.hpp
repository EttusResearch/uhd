//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "../../lib/usrp/x400/x400_radio_control.hpp"
#include "x4xx_hbx_mpm_mock.hpp"
#include <uhd/rfnoc/mock_block.hpp>
#include <uhd/rfnoc/node_accessor.hpp>
#include <uhd/utils/log.hpp>
#include <array>
#include <chrono>
#include <iostream>
#include <map>

using namespace uhd;
using namespace uhd::rfnoc;
using namespace std::chrono_literals;
using namespace uhd::usrp::hbx;
using namespace uhd::experts;

namespace {
/* This class extends mock_reg_iface_t by adding a constructor that initializes
 * some of the read memory to contain the memory size for the radio block.
 */
class hbx_radio_mock_reg_iface_t : public mock_reg_iface_t
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

    // In HBX, SPI info register encoding is:
    // - bits [7:0]   data width
    // - bits [15:8]  address width
    // - bits [31:16] FIFO depth
    static constexpr uint32_t spi_info_value = 16 /* data width */
                                               | (8 /* address width */ << 8)
                                               | (1 /* fifo depth */ << 16);
    static constexpr uint32_t spi_status_ready = (1u << 31) | (1u << 30);

public:
    hbx_radio_mock_reg_iface_t(size_t num_channels)
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

        // Initialize all HBX CPLD SPI engines that are used by the HBX dboard init
        for (const uint32_t spi_info_offset :
            {0x1000u, 0x1010u, 0x1020u, 0x1030u, 0x1040u, 0x1050u, 0x1060u}) {
            read_memory[cpld_offset + spi_info_offset]        = spi_info_value;
            read_memory[cpld_offset + spi_info_offset + 0x8]  = spi_status_ready;
            _lo_muxout_to_lock[cpld_offset + spi_info_offset] = false;
        }

        // Setup the GPIO addresses
        read_memory[gpio_offset + 0x4] = 0;

        // Disable the complex gain feature
        read_memory[radio_control_impl::regmap::RADIO_BASE_ADDR
                    + radio_control_impl::regmap::REG_FEATURES_PRESENT] = 0;

        // RFDC IQ/DC correction metadata used by HBX experts.
        // Address 0x8A020 (PERIPH_BASE + 0xA020) is read during init to determine
        // the available number of IQ correction coefficients.
        // Addresses PERIPH_BASE + 0xA000 (TX) and +0xA020 (RX) are read during
        // init to determine the available number of IQ correction coefficients.
        read_memory[rfdc_offset + 0x2000] = 64;
        read_memory[rfdc_offset + 0x2020] = 64;
    }

    void _poke_cb(uint32_t addr, uint32_t data, uhd::time_spec_t, bool) override
    {
        // Are we on the peripheral?
        if (addr >= radio_control_impl::regmap::PERIPH_BASE) {
            // handle all the periphs stuff that is not CPLD here
        } else {
            return;
        }

        // Are we poking the RFDC controls?
        if (addr >= rfdc_offset) {
            _poke_rfdc_cb(addr, data);
            return;
        }

        // Handle CPLD register space
        if (addr >= cpld_offset && addr < rfdc_offset) {
            if (_is_spi_setup_addr(addr)) {
                _poke_spi_setup(addr, data);
                return;
            }
            read_memory[addr] = data;
            return;
        }
    }

    void _poke_rfdc_cb(const uint32_t addr, const uint32_t data)
    {
        read_memory[addr] |= data;
        // Track the last value written to each address so tests can verify
        // exact register contents without the OR-accumulation of read_memory.
        write_log[addr] = data;
    }

    // Records the last value poked to each address in the RFDC register space.
    std::map<uint32_t, uint32_t> write_log;

    void _peek_cb(uint32_t addr, uhd::time_spec_t /*time*/) override
    {
        // HBX radio code may read peripheral registers that have no explicit mock
        // behavior. Default those reads to zero instead of throwing from
        // mock_reg_iface_t.
        if (addr >= cpld_offset && read_memory.count(addr) == 0) {
            read_memory[addr] = 0;
        }
    }

    bool _is_spi_setup_addr(const uint32_t addr) const
    {
        const uint32_t offset = addr - cpld_offset;
        switch (offset) {
            case 0x1004:
            case 0x1014:
            case 0x1024:
            case 0x1034:
            case 0x1044:
            case 0x1054:
            case 0x1064:
                return true;
            default:
                return false;
        }
    }

    void _poke_spi_setup(const uint32_t addr, const uint32_t data)
    {
        const uint32_t TX_LO_SPI_INFO    = 0x1000;
        const uint32_t RX_LO_SPI_INFO    = 0x1010;
        const uint32_t IQ_DEMOD_SPI_INFO = 0x1040;
        const uint32_t spi_base          = addr - 0x4;
        const uint32_t spi_info          = read_memory[spi_base];
        uint32_t data_width              = spi_info & 0xFF;
        uint32_t addr_width              = (spi_info >> 8) & 0xFF;
        if (data_width == 0) {
            data_width = 16;
        }
        if (addr_width == 0) {
            addr_width = 8;
        }

        const bool start_xact = (data & (1u << 31)) != 0;
        if (!start_xact) {
            return;
        }

        const bool is_read       = (data & (1u << 30)) != 0;
        const uint32_t spi_addr  = (data >> data_width) & ((1u << addr_width) - 1);
        const uint16_t spi_wdata = static_cast<uint16_t>(data & ((1u << data_width) - 1));

        if (!is_read) {
            _spi_registers[spi_base][spi_addr] = spi_wdata;
            // LMX lock-detect emulation. Register 0 bit 2 selects lock detect mode.
            if ((spi_base == cpld_offset + TX_LO_SPI_INFO)
                || (spi_base == cpld_offset + RX_LO_SPI_INFO)) {
                if (spi_addr == 0) {
                    _lo_muxout_to_lock[spi_base] = (spi_wdata & (1u << 2)) != 0;
                }
            }
        }

        uint16_t spi_rdata = 0;
        {
            auto& spi_regs = _spi_registers[spi_base];
            const auto it  = spi_regs.find(spi_addr);
            if (it != spi_regs.end()) {
                spi_rdata = it->second;
            } else if (spi_base == cpld_offset + IQ_DEMOD_SPI_INFO
                       && (spi_addr == 0x0C || spi_addr == 0x0D)) {
                // LTC5594 reset defaults for HD2IY and HD2IX.
                spi_rdata = 0x80;
            }
        }

        if (is_read
            && ((spi_base == cpld_offset + TX_LO_SPI_INFO)
                || (spi_base == cpld_offset + RX_LO_SPI_INFO))
            && spi_addr == 125) {
            // LMX2572 ID register readback expected by initialization code.
            spi_rdata = 0x2288;
        }
        if (is_read
            && ((spi_base == cpld_offset + TX_LO_SPI_INFO)
                || (spi_base == cpld_offset + RX_LO_SPI_INFO))
            && _lo_muxout_to_lock[spi_base]) {
            // LMX returns 0xFFFF when lock detect is routed to MUXOUT and PLL is locked.
            spi_rdata = 0xFFFF;
        }

        read_memory[spi_base + 0x8] = spi_status_ready | (spi_addr << data_width)
                                      | (spi_rdata & ((1u << data_width) - 1));
    }

    // SPI register model keyed by SPI base address and SPI register address
    std::map<uint32_t, std::map<uint32_t, uint16_t>> _spi_registers;
    std::map<uint32_t, bool> _lo_muxout_to_lock;
}; // class hbx_radio_mock_reg_iface_t

/*
 * x400_hbx_radio_fixture is a class which is instantiated before each test
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

struct x400_hbx_radio_fixture
{
    x400_hbx_radio_fixture()
        : ule(uhd::log::trace) // Note: When debugging this test, either set
                               // this to a lower level, or create a
                               // uhd_log_enabler in the test-under-test
        , num_channels(uhd::usrp::hbx::HBX_MAX_NUM_CHANS)
        , num_input_ports(num_channels)
        , num_output_ports(num_channels)
        , reg_iface(std::make_shared<hbx_radio_mock_reg_iface_t>(num_channels))
        , rpcs(std::make_shared<uhd::test::x4xx_hbx_mock_rpc_server>(device_info))
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

    ~x400_hbx_radio_fixture() {}


    // Must remain the first member so we make sure the log level is high
    uhd_log_enabler ule;
    const size_t num_channels;
    const size_t num_input_ports;
    const size_t num_output_ports;
    uhd::device_addr_t device_info = uhd::device_addr_t("master_clock_rate=1.25e9");
    std::shared_ptr<hbx_radio_mock_reg_iface_t> reg_iface;
    std::shared_ptr<uhd::test::x4xx_hbx_mock_rpc_server> rpcs;
    mpmd_mb_controller::sptr mbc;

    mock_block_container block_container;
    std::shared_ptr<x400_radio_control_impl> test_radio;
    node_accessor_t node_accessor{};
};
} // namespace
