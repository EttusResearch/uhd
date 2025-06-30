//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "obx_gpio_ctrl.hpp"
#include <uhd/types/ranges.hpp>
#include <uhd/usrp/dboard_iface.hpp>
#include <mutex>
#include <vector>

namespace uhd { namespace usrp { namespace dboard { namespace obx {

enum band_select_t { HIGH_BAND, MID_BAND, LOW_BAND };

enum lo1_filter_path_t { LFCN_800, LFCN_2250, NO_FILTER };

enum rx_lna_path_t { SEL_LNA_PMA3, SEL_LNA_MAAL };

enum obx_tx_cpld_field_id_t {
    TXHB_SEL2         = 0,
    TXHB_SEL          = 1,
    TXLB_SEL2         = 2,
    TXLB_SEL          = 3,
    TXLO1_FSEL1       = 4,
    TXLO1_FSEL2       = 5,
    TXLO1_FSEL3       = 6,
    TXLO1_FSEL4       = 7,
    FE_SEL_CAL_TX2    = 8,
    TXLO1_FORCEON     = 12,
    TXLO2_FORCEON     = 13,
    TXMOD_FORCEON     = 14,
    TXMIXER_FORCEON   = 15,
    TXDRV_FORCEON     = 16,
    TXDOUBLER_FORCEON = 22,
};

enum obx_rx_cpld_field_id_t {
    RXHB_SEL2         = 0,
    RXHB_SEL          = 1,
    RXLB_SEL2         = 2,
    RXLB_SEL          = 3,
    RXLO1_FSEL1       = 4,
    RXLO1_FSEL2       = 5,
    RXLO1_FSEL3       = 6,
    RXLO1_FSEL4       = 7,
    FE_SEL_CAL_RX2    = 8,
    SEL_LNA1          = 9,
    SEL_LNA2          = 10,
    RXLO1_FORCEON     = 12,
    RXLO2_FORCEON     = 13,
    RXDEMOD_FORCEON   = 14,
    RXMIXER_FORCEON   = 15,
    RXDRV_FORCEON     = 16,
    RXAMP_FORCEON     = 17,
    RXLNA1_FORCEON    = 20,
    RXLNA2_FORCEON    = 21,
    RXDOUBLER_FORCEON = 22,
};

enum spi_dest_t {
    TXLO1 = 0x0, // 0x00: TXLO1, the main TXLO
    TXLO2 = 0x1, // 0x01: TXLO2, the low/high band mixer TXLO
    RXLO1 = 0x2, // 0x02: RXLO1, the main RXLO
    RXLO2 = 0x3, // 0x03: RXLO2, the low/high band mixer RXLO
    CPLD  = 0x4 // 0x04: CPLD SPI Register
};

class obx_cpld_ctrl final
{
public:
    typedef std::shared_ptr<obx_cpld_ctrl> sptr;

    obx_cpld_ctrl(dboard_iface::sptr db_iface, obx_gpio_ctrl::sptr gpio_ctrl);

    ~obx_cpld_ctrl();

    void set_field(obx_tx_cpld_field_id_t field, uint32_t value);
    void set_field(obx_rx_cpld_field_id_t field, uint32_t value);
    uint32_t get_tx_value();
    uint32_t get_rx_value();
    void write();
    void write_lo_regs(spi_dest_t lo, std::vector<uint32_t> values);
    void set_tx_path(lo1_filter_path_t lo_filter, band_select_t band);
    void set_rx_path(lo1_filter_path_t lo_filter, band_select_t band, rx_lna_path_t lna);

private:
    dboard_iface::sptr _db_iface;
    obx_gpio_ctrl::sptr _gpio;
    std::mutex _spi_mutex;
    uint32_t _tx_value;
    uint32_t _rx_value;
    uint32_t _last_tx_value_written;
    uint32_t _last_rx_value_written;
};

}}}} // namespace uhd::usrp::dboard::obx
