//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "obx_cpld_ctrl.hpp"
#include "obx_gpio_ctrl.hpp"
#include <uhd/utils/safe_call.hpp>
#include <chrono>
#include <thread>

namespace uhd { namespace usrp { namespace dboard { namespace obx {

obx_cpld_ctrl::obx_cpld_ctrl(dboard_iface::sptr db_iface, obx_gpio_ctrl::sptr gpio_ctrl)
    : _db_iface(db_iface), _gpio(gpio_ctrl)
{
    // Reset the CPLD
    _gpio->set_field(CPLD_RST_N, 0);
    _gpio->write();
    std::this_thread::sleep_for(
        std::chrono::milliseconds(20)); // hold CPLD reset for minimum of 20 ms
    _gpio->set_field(CPLD_RST_N, 1);
    _gpio->write();

    // Initialize CPLD register
    _last_tx_value_written = 0xFFFF;
    _last_rx_value_written = 0xFFFF;
    _tx_value              = 0;
    _rx_value              = 0;
    write();
}

obx_cpld_ctrl::~obx_cpld_ctrl()
{
    UHD_SAFE_CALL(
        // Reset CPLD values
        _tx_value = 0; _rx_value = 0; write();)
}

// CPLD Control
void obx_cpld_ctrl::set_field(obx_tx_cpld_field_id_t field, uint32_t value)
{
    UHD_ASSERT_THROW(value == (value & 0x1));
    if (value)
        _tx_value |= uint32_t(1) << field;
    else
        _tx_value &= ~(uint32_t(1) << field);
}

void obx_cpld_ctrl::set_field(obx_rx_cpld_field_id_t field, uint32_t value)
{
    UHD_ASSERT_THROW(value == (value & 0x1));
    if (value)
        _rx_value |= uint32_t(1) << field;
    else
        _rx_value &= ~(uint32_t(1) << field);
}

uint32_t obx_cpld_ctrl::get_tx_value()
{
    return _tx_value;
}

uint32_t obx_cpld_ctrl::get_rx_value()
{
    return _rx_value;
}

void obx_cpld_ctrl::write()
{
    if (_tx_value != _last_tx_value_written || _rx_value != _last_rx_value_written) {
        std::lock_guard<std::mutex> lock(_spi_mutex);
        _gpio->set_field(SPI_ADDR, CPLD);
        _gpio->write();
        // For SPI commands, the MSB is the read/write bit, 0 for write and 1 for read
        // The next 7 bits are the register address, 0x0 for Tx and 0x4 for Rx
        // The remaining 24 bits are the data to be written to the register
        if (_tx_value != _last_tx_value_written) {
            uint32_t value_to_write = (0 << 31) | (0x00 << 24) | (_tx_value & 0xFFFFFF);
            _db_iface->write_spi(
                dboard_iface::UNIT_TX, spi_config_t::EDGE_RISE, value_to_write, 32);
            _last_tx_value_written = _tx_value;
        }
        if (_rx_value != _last_rx_value_written) {
            uint32_t value_to_write = (0 << 31) | (0x04 << 24) | (_rx_value & 0xFFFFFF);
            _db_iface->write_spi(
                dboard_iface::UNIT_TX, spi_config_t::EDGE_RISE, value_to_write, 32);
            _last_rx_value_written = _rx_value;
        }
    }
}

// LO Control Through CPLD
void obx_cpld_ctrl::write_lo_regs(spi_dest_t lo_dest, std::vector<uint32_t> values)
{
    std::lock_guard<std::mutex> lock(_spi_mutex);
    _gpio->set_field(SPI_ADDR, lo_dest);
    _gpio->write();
    for (uint32_t val : values)
        _db_iface->write_spi(dboard_iface::UNIT_TX, spi_config_t::EDGE_RISE, val, 32);
}

void obx_cpld_ctrl::set_tx_path(lo1_filter_path_t lo_filter, band_select_t band)
{
    switch (band) {
        case LOW_BAND:
            set_field(TXHB_SEL2, 1);
            set_field(TXHB_SEL, 0);
            set_field(TXLB_SEL2, 0);
            set_field(TXLB_SEL, 1);
            break;
        case MID_BAND:
            set_field(TXHB_SEL2, 0);
            set_field(TXHB_SEL, 1);
            set_field(TXLB_SEL2, 1);
            set_field(TXLB_SEL, 0);
            break;
        case HIGH_BAND:
            set_field(TXHB_SEL2, 0);
            set_field(TXHB_SEL, 0);
            set_field(TXLB_SEL2, 1);
            set_field(TXLB_SEL, 1);
            break;
        default:
            throw uhd::runtime_error("Invalid band selection");
    }
    switch (lo_filter) {
        case LFCN_800:
            set_field(TXLO1_FSEL1, 1);
            set_field(TXLO1_FSEL2, 1);
            set_field(TXLO1_FSEL3, 0);
            set_field(TXLO1_FSEL4, 0);
            break;
        case LFCN_2250:
            set_field(TXLO1_FSEL1, 0);
            set_field(TXLO1_FSEL2, 0);
            set_field(TXLO1_FSEL3, 1);
            set_field(TXLO1_FSEL4, 1);
            break;
        case NO_FILTER:
            set_field(TXLO1_FSEL1, 1);
            set_field(TXLO1_FSEL2, 0);
            set_field(TXLO1_FSEL3, 0);
            set_field(TXLO1_FSEL4, 1);
            break;
        default:
            throw uhd::runtime_error("Invalid LO filter selection");
    }
}

void obx_cpld_ctrl::set_rx_path(
    lo1_filter_path_t lo_filter, band_select_t band, rx_lna_path_t lna)
{
    switch (band) {
        case LOW_BAND:
            set_field(RXHB_SEL2, 0);
            set_field(RXHB_SEL, 1);
            set_field(RXLB_SEL2, 1);
            set_field(RXLB_SEL, 0);
            break;
        case MID_BAND:
            set_field(RXHB_SEL2, 1);
            set_field(RXHB_SEL, 1);
            set_field(RXLB_SEL2, 0);
            set_field(RXLB_SEL, 1);
            break;
        case HIGH_BAND:
            set_field(RXHB_SEL2, 0);
            set_field(RXHB_SEL, 0);
            set_field(RXLB_SEL2, 1);
            set_field(RXLB_SEL, 1);
            break;
        default:
            throw uhd::runtime_error("Invalid band selection");
    }
    switch (lo_filter) {
        case LFCN_800:
            set_field(RXLO1_FSEL1, 1);
            set_field(RXLO1_FSEL2, 1);
            set_field(RXLO1_FSEL3, 0);
            set_field(RXLO1_FSEL4, 0);
            break;
        case LFCN_2250:
            set_field(RXLO1_FSEL1, 0);
            set_field(RXLO1_FSEL2, 0);
            set_field(RXLO1_FSEL3, 1);
            set_field(RXLO1_FSEL4, 1);
            break;
        case NO_FILTER:
            set_field(RXLO1_FSEL1, 0);
            set_field(RXLO1_FSEL2, 1);
            set_field(RXLO1_FSEL3, 0);
            set_field(RXLO1_FSEL4, 1);
            break;
        default:
            throw uhd::runtime_error("Invalid LO filter selection");
    }
    switch (lna) {
        case SEL_LNA_PMA3:
            set_field(SEL_LNA1, 0);
            set_field(SEL_LNA2, 1);
            break;
        case SEL_LNA_MAAL:
            set_field(SEL_LNA1, 1);
            set_field(SEL_LNA2, 0);
            break;
        default:
            throw uhd::runtime_error("Invalid LNA selection");
    }
}

}}}} // namespace uhd::usrp::dboard::obx
