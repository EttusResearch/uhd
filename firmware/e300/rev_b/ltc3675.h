/*
 * Copyright 2012 Ettus Research LLC
 */

#ifndef LTC3675_H
#define LTC3675_H

//#include "types.h"
#include <stdbool.h>
#include <stdint.h>

typedef bool (*ltc3675_reg_helper_fn)(uint8_t address);

bool ltc3675_init(ltc3675_reg_helper_fn helper);

typedef enum ltc3675_regulators {
    LTC3675_REG_1,  // 1A Buck
    LTC3675_REG_2,  // 1A Buck
    LTC3675_REG_3,  // 500mA Buck
    LTC3675_REG_4,  // 500mA Buck
    LTC3675_REG_5,  // 1A Boost
    LTC3675_REG_6   // 1A Buck-Boost
    // LED Boost
} ltc3675_regulator_t;

bool ltc3675_enable_reg(ltc3675_regulator_t reg, bool on);
bool ltc3675_set_voltage(ltc3675_regulator_t reg, uint16_t voltage);
bool ltc3675_is_power_button_depressed(void);
bool ltc3675_has_interrupt(void);
bool ltc3675_handle_irq(void);
int8_t ltc3675_check_status(void);
uint8_t ltc3675_get_last_status(void);
uint8_t ltc3675_status_to_error(uint8_t val);
bool ltc3675_is_power_good(uint8_t val);
bool ltc3675_is_waking_up(void);

#endif /* LTC3675_H */
