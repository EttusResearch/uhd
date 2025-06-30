//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "obx_gpio_ctrl.hpp"
#include <uhd/utils/safe_call.hpp>
#include <chrono>
#include <cstring>
#include <map>
#include <thread>

namespace uhd { namespace usrp { namespace dboard { namespace obx {

obx_gpio_ctrl::obx_gpio_ctrl(dboard_iface::sptr db_iface)
    : _db_iface(db_iface), _tx_reg(), _rx_reg()
{
    for (std::map<obx_gpio_field_id_t, obx_gpio_field_info_t>::const_iterator entry =
             gpio_map.begin();
         entry != gpio_map.end();
         entry++) {
        obx_gpio_field_info_t field_info = entry->second;
        if (field_info.is_input)
            // Registers initialize to 0 (All Output)
            // Input is 1, so we can pass the mask as both the value and mask
            _db_iface->set_gpio_ddr(field_info.unit, field_info.mask, field_info.mask);
        if (field_info.is_atr_controlled) {
            // Registers initialize to 0 (All GPIO Control)
            // ATR Control is 1, so we can pass the mask as both the value and mask
            _db_iface->set_pin_ctrl(field_info.unit, field_info.mask, field_info.mask);
            _db_iface->set_atr_reg(field_info.unit,
                gpio_atr::ATR_REG_IDLE,
                (field_info.atr_idle << field_info.offset),
                field_info.mask);
            _db_iface->set_atr_reg(field_info.unit,
                gpio_atr::ATR_REG_TX_ONLY,
                (field_info.atr_tx << field_info.offset),
                field_info.mask);
            _db_iface->set_atr_reg(field_info.unit,
                gpio_atr::ATR_REG_RX_ONLY,
                (field_info.atr_rx << field_info.offset),
                field_info.mask);
            _db_iface->set_atr_reg(field_info.unit,
                gpio_atr::ATR_REG_FULL_DUPLEX,
                (field_info.atr_full_duplex << field_info.offset),
                field_info.mask);
        }
    }

    // Set default GPIO values
    set_field(TX_GAIN, 0);
    set_field(RX2_EN_N, 0);
    set_field(TX_EN_N, 1);
    set_field(RX_EN_N, 1);
    set_field(SPI_ADDR, 0x7);
    set_field(RX_GAIN, 0);
    set_field(TXLO1_SYNC, 0);
    set_field(TXLO2_SYNC, 0);
    set_field(RXLO1_SYNC, 0);
    set_field(RXLO1_SYNC, 0);
    write();
}

obx_gpio_ctrl::~obx_gpio_ctrl()
{
    UHD_SAFE_CALL(
        // Reset GPIO values
        set_field(TX_GAIN, 0); set_field(CPLD_RST_N, 0); set_field(RX2_EN_N, 0);
        set_field(TX_EN_N, 1);
        set_field(RX_EN_N, 1);
        set_field(SPI_ADDR, 0x7);
        set_field(RX_GAIN, 0);
        set_field(TXLO1_SYNC, 0);
        set_field(TXLO2_SYNC, 0);
        set_field(RXLO1_SYNC, 0);
        set_field(RXLO1_SYNC, 0);
        write();)
}

void obx_gpio_ctrl::set_field(obx_gpio_field_id_t id, uint32_t value)
{
    // Look up field info
    auto entry = gpio_map.find(id);
    if (entry == gpio_map.end()) {
        throw uhd::lookup_error("Trying to set invalid GPIO field");
    }
    obx_gpio_field_info_t field_info = entry->second;
    if (!field_info.is_input)
        throw uhd::runtime_error("Trying to set output GPIO field");
    obx_gpio_reg_t* reg =
        (field_info.unit == dboard_iface::UNIT_TX ? &_tx_reg : &_rx_reg);

    // Set field and mask; if the value has changed, mark the register dirty
    uint32_t previous_value = reg->value;
    reg->value &= ~field_info.mask;
    reg->value |= (value << field_info.offset) & field_info.mask;
    reg->dirty_mask |= field_info.mask;
    if (reg->value != previous_value) {
        reg->dirty = true;
    }
}

uint32_t obx_gpio_ctrl::get_field(obx_gpio_field_id_t id)
{
    std::lock_guard<std::mutex> lock(_gpio_mutex);
    // Look up field info
    auto entry = gpio_map.find(id);
    if (entry == gpio_map.end()) {
        throw uhd::lookup_error("Trying to get invalid GPIO field");
    }
    obx_gpio_field_info_t field_info = entry->second;
    if (field_info.is_input) {
        obx_gpio_reg_t* reg =
            (field_info.unit == dboard_iface::UNIT_TX ? &_tx_reg : &_rx_reg);
        return (reg->value >> field_info.offset) & field_info.mask;
    }

    // Read register
    uint32_t value = _db_iface->read_gpio(field_info.unit);
    value &= field_info.mask;
    value >>= field_info.offset;

    // Return field value
    return value;
}

void obx_gpio_ctrl::write()
{
    std::lock_guard<std::mutex> lock(_gpio_mutex);
    if (_tx_reg.dirty) {
        _db_iface->set_gpio_out(dboard_iface::UNIT_TX, _tx_reg.value, _tx_reg.dirty_mask);
        _tx_reg.dirty      = false;
        _tx_reg.dirty_mask = 0;
    }
    if (_rx_reg.dirty) {
        _db_iface->set_gpio_out(dboard_iface::UNIT_RX, _rx_reg.value, _rx_reg.dirty_mask);
        _rx_reg.dirty      = false;
        _rx_reg.dirty_mask = 0;
    }
}

}}}} // namespace uhd::usrp::dboard::obx
