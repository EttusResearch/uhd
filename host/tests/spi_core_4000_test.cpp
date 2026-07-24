//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <boost/test/unit_test.hpp>

#include <uhd/exception.hpp>
#include <uhd/features/spi_getter_iface.hpp>
#include <uhdlib/usrp/cores/spi_core_4000.hpp>

#include <cstdint>
#include <utility>
#include <vector>

namespace {

constexpr uint32_t SPI_PERIPH_CFG_ADDR      = 0x100;
constexpr uint32_t SPI_TRANSACTION_CFG_ADDR = 0x200;
constexpr uint32_t SPI_TRANSACTION_GO_ADDR  = 0x204;
constexpr uint32_t SPI_STATUS_ADDR          = 0x208;
constexpr uint32_t SPI_CTRL_INFO_ADDR       = 0x20C;

constexpr uint32_t num_peripherals = 2;

using reg_write_t = std::pair<uint32_t, uint32_t>;

uhd::features::spi_periph_config_t make_periph_cfg(
    uint8_t cs, uint8_t sdi, uint8_t sdo, uint8_t clk)
{
    return {cs, sdi, sdo, clk};
}

uint32_t calc_periph_ctrl_expected(const uhd::spi_config_t& config,
    const uhd::features::spi_periph_config_t& periph_cfg,
    size_t num_bits)
{
    auto get_bitfield = [](uint32_t pin, uint32_t offset) {
        const uint32_t adjusted_pin =
            pin
            + ((pin >= uhd::cores::NUM_PINS_PER_PORT) ? static_cast<uint32_t>(
                   uhd::cores::PORT_NUMBER_OFFSET - uhd::cores::NUM_PINS_PER_PORT)
                                                      : 0u);
        return (adjusted_pin & 0x1F) << offset;
    };

    uint32_t periph_ctrl = 0;
    if (config.mosi_edge == uhd::spi_config_t::EDGE_FALL) {
        periph_ctrl |= (1u << 27);
    }
    if (config.miso_edge == uhd::spi_config_t::EDGE_RISE) {
        periph_ctrl |= (1u << 26);
    }
    periph_ctrl |= (static_cast<uint32_t>(num_bits) & 0x3F) << 20;
    periph_ctrl |= get_bitfield(periph_cfg.periph_cs, 15);
    periph_ctrl |= get_bitfield(periph_cfg.periph_sdi, 10);
    periph_ctrl |= get_bitfield(periph_cfg.periph_sdo, 5);
    periph_ctrl |= get_bitfield(periph_cfg.periph_clk, 0);
    return periph_ctrl;
}

} // namespace

BOOST_AUTO_TEST_CASE(spi_core_4000_validates_configuration_and_arguments)
{
    std::vector<reg_write_t> writes;

    auto poke = [&](uint32_t addr, uint32_t data) { writes.emplace_back(addr, data); };
    auto peek = [&](uint32_t addr) -> uint32_t {
        if (addr == SPI_CTRL_INFO_ADDR) {
            return num_peripherals;
        }
        if (addr == SPI_STATUS_ADDR) {
            return (1u << 24) | 0x0055AA;
        }
        return 0;
    };

    auto spi = uhd::cores::spi_core_4000::make(std::move(poke),
        std::move(peek),
        SPI_PERIPH_CFG_ADDR,
        SPI_TRANSACTION_CFG_ADDR,
        SPI_TRANSACTION_GO_ADDR,
        SPI_STATUS_ADDR,
        SPI_CTRL_INFO_ADDR);

    uhd::spi_config_t config;

    BOOST_CHECK_THROW(spi->transact_spi(0, config, 0, 8, false), uhd::value_error);

    std::vector<uhd::features::spi_periph_config_t> too_many_cfgs = {
        make_periph_cfg(0, 1, 2, 3),
        make_periph_cfg(4, 5, 6, 7),
        make_periph_cfg(8, 9, 10, 11),
    };
    BOOST_CHECK_THROW(spi->set_spi_periph_config(too_many_cfgs), uhd::value_error);

    std::vector<uhd::features::spi_periph_config_t> valid_cfgs = {
        make_periph_cfg(0, 1, 2, 3),
        make_periph_cfg(12, 13, 14, 15),
    };
    spi->set_spi_periph_config(valid_cfgs);

    uhd::spi_config_t invalid_divider_config;
    invalid_divider_config.divider = 0x10000;
    BOOST_CHECK_THROW(
        spi->transact_spi(0, invalid_divider_config, 0x1, 1, false), uhd::value_error);
}

BOOST_AUTO_TEST_CASE(spi_core_4000_transact_writes_expected_register_values_and_caches)
{
    std::vector<reg_write_t> writes;

    auto poke = [&](uint32_t addr, uint32_t data) { writes.emplace_back(addr, data); };
    auto peek = [&](uint32_t addr) -> uint32_t {
        if (addr == SPI_CTRL_INFO_ADDR) {
            return num_peripherals;
        }
        if (addr == SPI_STATUS_ADDR) {
            return (1u << 24) | 0x00ABCDEF;
        }
        return 0;
    };

    auto spi = uhd::cores::spi_core_4000::make(std::move(poke),
        std::move(peek),
        SPI_PERIPH_CFG_ADDR,
        SPI_TRANSACTION_CFG_ADDR,
        SPI_TRANSACTION_GO_ADDR,
        SPI_STATUS_ADDR,
        SPI_CTRL_INFO_ADDR);

    std::vector<uhd::features::spi_periph_config_t> cfgs = {
        make_periph_cfg(0, 1, 2, 3),
        // Use port-B range pins to verify 12..15 become 16..19 in hardware bitfield
        make_periph_cfg(12, 13, 14, 15),
    };
    spi->set_spi_periph_config(cfgs);

    uhd::spi_config_t config;
    config.mosi_edge = uhd::spi_config_t::EDGE_FALL;
    config.miso_edge = uhd::spi_config_t::EDGE_RISE;
    config.divider   = 0x1234;

    const uint32_t tx_data = 0xA5;
    const size_t tx_bits   = 8;

    const uint32_t response = spi->transact_spi(1, config, tx_data, tx_bits, true);
    BOOST_CHECK_EQUAL(response, 0x00ABCDEF);

    BOOST_REQUIRE_EQUAL(writes.size(), 3);

    const uint32_t expected_periph_ctrl =
        calc_periph_ctrl_expected(config, cfgs.at(1), tx_bits);
    BOOST_CHECK_EQUAL(writes.at(0).first, SPI_PERIPH_CFG_ADDR + 0x4);
    BOOST_CHECK_EQUAL(writes.at(0).second, expected_periph_ctrl);

    const uint32_t expected_transaction_cfg = (1u << 16) | 0x1234;
    BOOST_CHECK_EQUAL(writes.at(1).first, SPI_TRANSACTION_CFG_ADDR);
    BOOST_CHECK_EQUAL(writes.at(1).second, expected_transaction_cfg);

    const uint32_t expected_data_out = tx_data << (32 - tx_bits);
    BOOST_CHECK_EQUAL(writes.at(2).first, SPI_TRANSACTION_GO_ADDR);
    BOOST_CHECK_EQUAL(writes.at(2).second, expected_data_out);

    // Same transaction should only issue GO write due to periph/transaction caching.
    const uint32_t second_response = spi->transact_spi(1, config, tx_data, tx_bits, true);
    BOOST_CHECK_EQUAL(second_response, 0x00ABCDEF);
    BOOST_REQUIRE_EQUAL(writes.size(), 4);
    BOOST_CHECK_EQUAL(writes.at(3).first, SPI_TRANSACTION_GO_ADDR);
    BOOST_CHECK_EQUAL(writes.at(3).second, expected_data_out);
}
