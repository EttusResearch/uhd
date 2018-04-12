//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// Original ADF4001 driver written by: bistromath
//                                     Mar 1, 2013
//
// Re-used and re-licensed with permission.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhdlib/usrp/common/adf4001_ctrl.hpp>
#include <uhd/utils/log.hpp>

#include <iostream>
#include <iomanip>

using namespace uhd;
using namespace uhd::usrp;

adf4001_regs_t::adf4001_regs_t(void) {
    ref_counter = 0;
    n = 0;
    charge_pump_current_1 = 0;
    charge_pump_current_2 = 0;
    anti_backlash_width = ANTI_BACKLASH_WIDTH_2_9NS;
    lock_detect_precision = LOCK_DETECT_PRECISION_3CYC;
    charge_pump_gain = CHARGE_PUMP_GAIN_1;
    counter_reset = COUNTER_RESET_NORMAL;
    power_down = POWER_DOWN_NORMAL;
    muxout = MUXOUT_TRISTATE_OUT;
    phase_detector_polarity = PHASE_DETECTOR_POLARITY_NEGATIVE;
    charge_pump_mode = CHARGE_PUMP_TRISTATE;
    fastlock_mode = FASTLOCK_MODE_DISABLED;
    timer_counter_control = TIMEOUT_3CYC;
}


uint32_t adf4001_regs_t::get_reg(uint8_t addr) {
    uint32_t reg = 0;
    switch (addr) {
    case 0:
        reg |= (uint32_t(ref_counter)         & 0x003FFF) << 2;
        reg |= (uint32_t(anti_backlash_width) & 0x000003) << 16;
        reg |= (uint32_t(lock_detect_precision) & 0x000001) << 20;
        break;
    case 1:
        reg |= (uint32_t(n) & 0x001FFF) << 8;
        reg |= (uint32_t(charge_pump_gain) & 0x000001) << 21;
        break;
    case 2:
        reg |= (uint32_t(counter_reset) & 0x000001) << 2;
        reg |= (uint32_t(power_down) & 0x000001) << 3;
        reg |= (uint32_t(muxout) & 0x000007) << 4;
        reg |= (uint32_t(phase_detector_polarity) & 0x000001) << 7;
        reg |= (uint32_t(charge_pump_mode) & 0x000001) << 8;
        reg |= (uint32_t(fastlock_mode) & 0x000003) << 9;
        reg |= (uint32_t(timer_counter_control) & 0x00000F) << 11;
        reg |= (uint32_t(charge_pump_current_1) & 0x000007) << 15;
        reg |= (uint32_t(charge_pump_current_2) & 0x000007) << 18;
        reg |= (uint32_t(power_down) & 0x000002) << 20;
        break;
    case 3:
        reg |= (uint32_t(counter_reset) & 0x000001) << 2;
        reg |= (uint32_t(power_down) & 0x000001) << 3;
        reg |= (uint32_t(muxout) & 0x000007) << 4;
        reg |= (uint32_t(phase_detector_polarity) & 0x000001) << 7;
        reg |= (uint32_t(charge_pump_mode) & 0x000001) << 8;
        reg |= (uint32_t(fastlock_mode) & 0x000003) << 9;
        reg |= (uint32_t(timer_counter_control) & 0x00000F) << 11;
        reg |= (uint32_t(charge_pump_current_1) & 0x000007) << 15;
        reg |= (uint32_t(charge_pump_current_2) & 0x000007) << 18;
        reg |= (uint32_t(power_down) & 0x000002) << 20;
        break;
    default:
        break;
    }

    reg |= (uint32_t(addr) & 0x03);

    return reg;
}


adf4001_ctrl::adf4001_ctrl(uhd::spi_iface::sptr _spi, int slaveno):
    spi_iface(_spi),
    slaveno(slaveno)
    {

    spi_config.mosi_edge = spi_config_t::EDGE_RISE;

    //set defaults
    adf4001_regs.ref_counter = 1;
    adf4001_regs.n = 4;
    adf4001_regs.charge_pump_current_1 = 7;
    adf4001_regs.charge_pump_current_2 = 7;
    adf4001_regs.muxout = adf4001_regs_t::MUXOUT_DLD;
    adf4001_regs.counter_reset = adf4001_regs_t::COUNTER_RESET_NORMAL;
    adf4001_regs.phase_detector_polarity = adf4001_regs_t::PHASE_DETECTOR_POLARITY_POSITIVE;
    adf4001_regs.charge_pump_mode = adf4001_regs_t::CHARGE_PUMP_TRISTATE;

    //everything else should be defaults

    program_regs();
}

void adf4001_ctrl::set_lock_to_ext_ref(bool external) {
    if(external) {
        adf4001_regs.charge_pump_mode = adf4001_regs_t::CHARGE_PUMP_NORMAL;
    } else {
        adf4001_regs.charge_pump_mode = adf4001_regs_t::CHARGE_PUMP_TRISTATE;
    }

    program_regs();
}

void adf4001_ctrl::program_regs(void) {
    //no control over CE, only LE, therefore we use the initialization latch method
    write_reg(3);

    //conduct a function latch (2)
    write_reg(2);

    //write R counter latch (0)
    write_reg(0);

    //write N counter latch (1)
    write_reg(1);
}


void adf4001_ctrl::write_reg(uint8_t addr) {
    uint32_t reg = adf4001_regs.get_reg(addr); //load the reg data

        spi_iface->transact_spi(slaveno,
                                spi_config,
                                reg,
                                24,
                                false);
}
