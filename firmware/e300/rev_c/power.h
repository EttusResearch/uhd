#ifndef POWER_H
#define POWER_H

#include <stdbool.h>
#include <stdint.h>

void tps54478_init(bool enable);
void tps54478_set_power(bool on);   // Zynq core power (1.0V for FPGA)
bool tps54478_is_power_good(void);

void charge_set_led(bool on);		// Here for error blink codes
void charge_notify(bool charging);

void power_signal_interrupt(void);

void fpga_reset(bool delay);

typedef enum power_subsystems {
    PS_UNKNOWN,
    PS_FPGA,
    PS_VDRAM,
    PS_PERIPHERALS_1_8,
    PS_PERIPHERALS_3_3,
    PS_TX,
	PS_MAX
} power_subsystem_t;

enum Regulators
{
	REG_UNKNOWN,
	REG_TPS54478,
	REG_LTC3675
};

bool power_enable(power_subsystem_t subsys, bool on);

void battery_init(void);
uint16_t battery_get_voltage(void);  // mV

bool power_init(void);
bool power_on(void);
uint8_t power_off(void);

//bool power_is_subsys_on(int8_t index);
bool power_is_subsys_on(power_subsystem_t index);
//int8_t power_get_regulator_index(uint8_t device, uint8_t address);
//bool ltc3675_reg_helper(uint8_t address);

void usbhub_reset(void);

#ifndef I2C_REWORK
#include "io.h"

extern io_pin_t PWR_SDA;
extern io_pin_t PWR_SCL;
#endif // I2C_REWORK

#endif // POWER_H
