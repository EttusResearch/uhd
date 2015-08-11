/*
 * bq24190.c
 *
 * Copyright 2015 National Instruments Corp
 */

#ifdef CHARGER_TI

#include "config.h"
#include "bq24190.h"

#include <util/delay.h>

#include "io.h"
#include "i2c.h"
#include "debug.h"
#include "global.h"
#include "error.h"

#ifndef I2C_REWORK
#include "power.h"
#endif // I2C_REWORK

static io_pin_t USBPM_IRQ	= IO_PB(1);

#ifdef ATTINY88_DIP

static io_pin_t CHRG_SDA     = IO_PC(2);
static io_pin_t CHRG_SCL     = IO_PC(3);

#else

#ifdef I2C_REWORK

static io_pin_t CHRG_SDA     = IO_PC(4);
static io_pin_t CHRG_SCL     = IO_PC(5);

#else

#define CHRG_SDA	PWR_SDA
#define CHRG_SCL	PWR_SCL

#endif // I2C_REWORK

#endif // ATTINY88_DIP

const bool _bq24190_pull_up = false;

#define BQ24190_BASE_ADDRESS    (0x6B << 1)
#define BQ24190_WRITE_ADDRESS   (BQ24190_BASE_ADDRESS + 0)
#define BQ24190_READ_ADDRESS    (BQ24190_BASE_ADDRESS + 1)

enum BQ24190Registers
{
	BQ24190_REG_INPUT_SOURCE_CTL= 0,
	BQ24190_REG_PWR_ON_CONFIG	= 1,
	BQ24190_REG_CHARGE_CURRENT	= 2,
	BQ24190_REG_PRE_TERM_CURRENT= 3,
	BQ24190_REG_CHARGE_VOLTAGE	= 4,
	BQ24190_REG_TIMER_CONTROL	= 5,
	BQ24190_REG_SYSTEM_STATUS	= 8,
	BQ24190_REG_FAULT			= 9
};
/*
enum BQ24190TimerControl
{
	
};
*/
enum BQ24190Shifts
{
	BQ24190_SHIFTS_CHARGER_CONFIG	= 4,
	BQ24190_SHIFTS_I2C_WATCHDOG		= 4,
	BQ24190_SHIFTS_CHARGER_STATUS	= 4,
	BQ24190_SHIFTS_CHARGER_FAULT	= 4,
};

enum BQ24190VBusStatus
{
	BQ24190_VBUS_UNKNOWN,
	BQ24190_VBUS_USB,
	BQ24190_VBUS_ADAPTER,
	BQ24190_VBUS_OTG
};

enum BQ24190ChargerStatus
{
	BQ24190_CHRG_STAT_NOT_CHARGING,
	BQ24190_CHRG_STAT_PRE_CHARGE,
	BQ24190_CHRG_STAT_FAST_CHARGING,
	BQ24190_CHRG_STAT_CHARGE_TERMINATION_DONE,
	BQ24190_CHRG_STAT_MASK = BQ24190_CHRG_STAT_CHARGE_TERMINATION_DONE
};

enum BQ24190SystemStatus
{
	BQ24190_STATUS_DPM					= 0x08,
	BQ24190_STATUS_POWER_GOOD			= 0x04,
	BQ24190_STATUS_THERMAL_REGULATION	= 0x02,
	BQ24190_STATUS_VSYSMIN_REGULATION	= 0x01
};

enum BQ24190Faults
{
	BQ24190_FAULT_WATCHDOG_EXPIRED	= 0x80,
	BQ24190_FAULT_VBUS_OVERLOADED	= 0x40,
	BQ24190_FAULT_BATOVP			= 0x08
};

enum BQ24190ChargerFaults
{
	BQ24190_CHRGFAULT_NORMAL,
	BQ24190_CHRGFAULT_INPUT,
	BQ24190_CHRGFAULT_THERMAL,
	BQ24190_CHRGFAULT_SAFETY_TIMER
};

enum BQ24190NTCFaults
{
	BQ24190_NTCFAULT_NORMAL,
	BQ24190_NTCFAULT_TS1_COLD,
	BQ24190_NTCFAULT_TS1_HOT,
	BQ24190_NTCFAULT_TS2_COLD,
	BQ24190_NTCFAULT_TS2_HOT,
	BQ24190_NTCFAULT_BOTH_COLD,
	BQ24190_NTCFAULT_BOTH_HOT
};

bool bq24190_toggle_charger(bool on)
{
	uint8_t config = 0;
	if (i2c_read2_ex(CHRG_SDA, CHRG_SCL, BQ24190_READ_ADDRESS, BQ24190_REG_PWR_ON_CONFIG, &config, _bq24190_pull_up) == false)
		return false;
	
	debug_log_ex("BQPC ", false);
	debug_log_hex(config);
	
	config &= ~(0x3 << BQ24190_SHIFTS_CHARGER_CONFIG);
	if (on)
		config |= (0x01 << BQ24190_SHIFTS_CHARGER_CONFIG);	// Enable charger
	
	if (i2c_write_ex(CHRG_SDA, CHRG_SCL, BQ24190_WRITE_ADDRESS, BQ24190_REG_PWR_ON_CONFIG, config, _bq24190_pull_up) == false)
		return false;
	
	////////
/*
	if (i2c_read2_ex(CHRG_SDA, CHRG_SCL, BQ24190_READ_ADDRESS, BQ24190_REG_PWR_ON_CONFIG, &config, _bq24190_pull_up) == false)
		return false;
	
	debug_log_ex("BQPC ", false);
	debug_log_hex(config);
*/
	////////
	
	return true;
}

bool bq24190_init(bool disable_charger)
{
#ifdef I2C_REWORK
	i2c_init_ex(CHRG_SDA, CHRG_SCL, _bq24190_pull_up);
#endif // I2C_REWORK
	io_input_pin(USBPM_IRQ);
#if !defined(DEBUG) && !defined(ATTINY88_DIP)
	//io_set_pin(USBPM_IRQ);	// [Enable pull-up for Open Drain] AVR pull-up not enough
#endif // DEBUG
	if (disable_charger)
	{
		if (bq24190_toggle_charger(false) == false)
			return false;
	}
	
	///////////////////////////////////
	
	uint8_t timer_control = 0;
	if (i2c_read2_ex(CHRG_SDA, CHRG_SCL, BQ24190_READ_ADDRESS, BQ24190_REG_TIMER_CONTROL, &timer_control, _bq24190_pull_up) == false)
		return false;
	
	debug_log_ex("BQTC ", false);
	debug_log_hex(timer_control);
	
	timer_control &= ~(0x3 << BQ24190_SHIFTS_I2C_WATCHDOG);
	timer_control |= (0x00 << BQ24190_SHIFTS_I2C_WATCHDOG);	// Disable I2C watch dog
	
	if (i2c_write_ex(CHRG_SDA, CHRG_SCL, BQ24190_WRITE_ADDRESS, BQ24190_REG_TIMER_CONTROL, timer_control, _bq24190_pull_up) == false)
		return false;
	
	///////////////////////////////////
	
	//BQ24190_REG_PWR_ON_CONFIG
	// Minimum System Voltage Limit: (default) 101 3.5V
	
	//BQ24190_REG_CHARGE_CURRENT
	// Fast Charge Current Limit: (default) 011000 2048mA
	
	//BQ24190_REG_PRE_TERM_CURRENT
	// Pre-charge current limit: (default) 0001 256mA
	// Termination current limit: (default) 0001 256mA
	
	//BQ24190_REG_CHARGE_VOLTAGE
	// Charge voltage limit: (default) 101100 4.208V
	
	///////////////////////////////////
	
	uint8_t input_src_ctl = 0;
	if (i2c_read2_ex(CHRG_SDA, CHRG_SCL, BQ24190_READ_ADDRESS, BQ24190_REG_INPUT_SOURCE_CTL, &input_src_ctl, _bq24190_pull_up) == false)
		return false;
	
	debug_log_ex("BQIS ", false);
	debug_log_hex(input_src_ctl);
	
	// Input voltage limit: (default) 0110 4.36V
	
	//input_src_ctl &= ~(0x07);
	input_src_ctl |= (0x07);	// Set 3A limit
	
	if (i2c_write_ex(CHRG_SDA, CHRG_SCL, BQ24190_WRITE_ADDRESS, BQ24190_REG_INPUT_SOURCE_CTL, input_src_ctl, _bq24190_pull_up) == false)
		return false;
	
	return true;
}

bool bq24190_has_interrupt(void)
{
	//bool state = io_test_pin(USBPM_IRQ);
	//debug_log_ex("BQIRQ", false);
	//debug_log_byte(state);
	//return (state != 1);
	return (io_test_pin(USBPM_IRQ) == false);
}

static uint8_t _bq24190_last_status, _bq24190_last_fault;

bool _bq24190_handle_irq(void)
{
	uint8_t val = 0x00;
	bool result = false;
	
	if (i2c_read2_ex(CHRG_SDA, CHRG_SCL, BQ24190_READ_ADDRESS, BQ24190_REG_SYSTEM_STATUS, &val, _bq24190_pull_up) == false)
		goto _bq24190_handle_fail;
	
	debug_log_ex("BQST ", false);
	debug_log_hex(val);
	
	if (i2c_read2_ex(CHRG_SDA, CHRG_SCL, BQ24190_READ_ADDRESS, BQ24190_REG_SYSTEM_STATUS, &val, _bq24190_pull_up) == false)
		goto _bq24190_handle_fail;
	
	_bq24190_last_status = val;

	debug_log_ex("BQST ", false);
	debug_log_hex(val);
	
	/*if (val & LTC4155_WALLSNS_GOOD)
	{
		uint8_t wall_state = 0;
		if (i2c_read2_ex(CHRG_SDA, CHRG_SCL, LTC4155_READ_ADDRESS, LTC4155_REG_WALL, &wall_state, _ltc4155_pull_up) == false)
			goto _bq24190_handle_fail;
		
		wall_state &= ~0x1E;
		wall_state |= 0x0E;
		
		if (i2c_write_ex(CHRG_SDA, CHRG_SCL, LTC4155_WRITE_ADDRESS, LTC4155_REG_WALL, wall_state, _ltc4155_pull_up) == false)
			goto _bq24190_handle_fail;
		
		debug_log("I+");
	}*/
	
	if (i2c_read2_ex(CHRG_SDA, CHRG_SCL, BQ24190_READ_ADDRESS, BQ24190_REG_FAULT, &val, _bq24190_pull_up) == false)
		goto _bq24190_handle_fail;
	
	debug_log_ex("BQF  ", false);
	debug_log_hex(val);
	
	if (i2c_read2_ex(CHRG_SDA, CHRG_SCL, BQ24190_READ_ADDRESS, BQ24190_REG_FAULT, &val, _bq24190_pull_up) == false)
		goto _bq24190_handle_fail;
	
	_bq24190_last_fault = val;
	
	debug_log_ex("BQF  ", false);
	debug_log_hex(val);
	
	val = (_bq24190_last_status >> BQ24190_SHIFTS_CHARGER_STATUS) & BQ24190_CHRG_STAT_MASK;
	
	if (_state.blink_error == BlinkError_None)
	{
		switch (val)
		{
			case BQ24190_CHRG_STAT_PRE_CHARGE:
			case BQ24190_CHRG_STAT_FAST_CHARGING:
			//case BQ24190_CHRG_STAT_CHARGE_TERMINATION_DONE:
			{
				if ((_state.battery_not_present == false)/* &&
					(_ltc4155_last_good & (LTC4155_WALLSNS_GOOD | LTC4155_USBSNS_GOOD))*/)
				{
					//charge_set_led(true);
					charge_notify(true);
					break;
				}
			}
			//case BQ24190_CHRG_STAT_NOT_CHARGING:
			default:
				//charge_set_led(false);
				charge_notify(false);
		}
	}
	
//	bq24190_dump();
	
	result = true;
_bq24190_handle_fail:
	return result;
}

bool bq24190_handle_irq(void)	// IRQ is pulsed (not held)
{
	pmc_mask_irqs(true);
	
	//_delay_ms(250);	// [Wait for registers to update]
	
	bool result = _bq24190_handle_irq();
	
	pmc_mask_irqs(false);
	
	return result;
}

//void bq24190_dump(void)
/*
bool bq24190_set_charge_current_limit(uint8_t deciamps)
{
	return true;
}
*/
#endif // CHARGER_TI
