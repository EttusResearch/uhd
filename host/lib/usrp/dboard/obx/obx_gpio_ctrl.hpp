//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/types/ranges.hpp>
#include <uhd/types/wb_iface.hpp>
#include <uhd/usrp/dboard_iface.hpp>
#include <uhd/utils/noncopyable.hpp>
#include <map>
#include <mutex>

namespace uhd { namespace usrp { namespace dboard { namespace obx {

enum obx_gpio_field_id_t {
    SPI_ADDR,
    TX_EN_N,
    RX_EN_N,
    RX2_EN_N,
    TX_LO_LOCKED,
    RX_LO_LOCKED,
    CPLD_RST_N,
    TX_GAIN,
    RX_GAIN,
    RXLO1_SYNC,
    RXLO2_SYNC,
    TXLO1_SYNC,
    TXLO2_SYNC
};

struct obx_gpio_reg_t
{
    bool dirty          = false;
    uint32_t value      = 0;
    uint32_t dirty_mask = 0;
};

struct obx_gpio_field_info_t
{
    obx_gpio_field_info_t(dboard_iface::unit_t unit,
        uint32_t offset,
        uint32_t mask,
        bool is_input,
        bool is_atr_controlled,
        uint32_t atr_idle,
        uint32_t atr_tx,
        uint32_t atr_rx,
        uint32_t atr_full_duplex)
        : unit(unit)
        , offset(offset)
        , mask(mask)
        , is_input(is_input)
        , is_atr_controlled(is_atr_controlled)
        , atr_idle(atr_idle)
        , atr_tx(atr_tx)
        , atr_rx(atr_rx)
        , atr_full_duplex(atr_full_duplex)
    {
    }
    dboard_iface::unit_t unit;
    uint32_t offset;
    uint32_t mask;
    bool is_input;
    bool is_atr_controlled;
    uint32_t atr_idle;
    uint32_t atr_tx;
    uint32_t atr_rx;
    uint32_t atr_full_duplex;
};

class obx_gpio_ctrl final
{
public:
    typedef std::shared_ptr<obx_gpio_ctrl> sptr;

    obx_gpio_ctrl(dboard_iface::sptr db_iface);
    ~obx_gpio_ctrl();

    void set_field(obx_gpio_field_id_t id, uint32_t value);
    uint32_t get_field(obx_gpio_field_id_t id);
    void write();
    // clang-format off
    const std::map<obx_gpio_field_id_t, obx_gpio_field_info_t> gpio_map = {
        //Field                               Unit                  Offset  Mask    Input?  ATR?   IDLE,TX,RX,FDX
        {SPI_ADDR,     obx_gpio_field_info_t(dboard_iface::UNIT_TX,   0,  0x7,      true,  false,  0,  0,  0,  0)},
        {CPLD_RST_N,   obx_gpio_field_info_t(dboard_iface::UNIT_TX,   3,  0x1<<3,   true,  false,  0,  0,  0,  0)},
        {RX2_EN_N,     obx_gpio_field_info_t(dboard_iface::UNIT_TX,   4,  0x1<<4,   true,  false,  0,  0,  0,  0)},
        {TX_EN_N,      obx_gpio_field_info_t(dboard_iface::UNIT_TX,   5,  0x1<<5,   true,  true,   1,  0,  1,  0)},
        {RX_EN_N,      obx_gpio_field_info_t(dboard_iface::UNIT_TX,   6,  0x1<<6,   true,  true,   1,  1,  0,  0)},
        {TXLO1_SYNC,   obx_gpio_field_info_t(dboard_iface::UNIT_TX,   7,  0x1<<7,   true,  true,   0,  0,  0,  0)},
        {TXLO2_SYNC,   obx_gpio_field_info_t(dboard_iface::UNIT_TX,   9,  0x1<<9,   true,  true,   0,  0,  0,  0)},
        {TX_GAIN,      obx_gpio_field_info_t(dboard_iface::UNIT_TX,   10, 0x3F<<10, true,  false,  0,  0,  0,  0)},
        {RX_LO_LOCKED, obx_gpio_field_info_t(dboard_iface::UNIT_RX,   0,  0x1,      false, false,  0,  0,  0,  0)},
        {TX_LO_LOCKED, obx_gpio_field_info_t(dboard_iface::UNIT_RX,   1,  0x1<<1,   false, false,  0,  0,  0,  0)},
        {RXLO1_SYNC,   obx_gpio_field_info_t(dboard_iface::UNIT_RX,   5,  0x1<<5,   true,  true,   0,  0,  0,  0)},
        {RXLO2_SYNC,   obx_gpio_field_info_t(dboard_iface::UNIT_RX,   7,  0x1<<7,   true,  true,   0,  0,  0,  0)},
        {RX_GAIN,      obx_gpio_field_info_t(dboard_iface::UNIT_RX,   10, 0x3F<<10, true,  false,  0,  0,  0,  0)}};
    // clang-format on

private:
    dboard_iface::sptr _db_iface;
    std::mutex _gpio_mutex;
    obx_gpio_reg_t _tx_reg;
    obx_gpio_reg_t _rx_reg;
};

}}}} // namespace uhd::usrp::dboard::obx
