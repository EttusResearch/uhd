/*
 * ltc4155.c
 */ 

#ifndef CHARGER_TI

#include "config.h"
#include "ltc4155.h"

#include <util/delay.h>

#include "io.h"
#include "i2c.h"
#include "power.h"
#include "debug.h"
#include "global.h"
#include "error.h"

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

const bool _ltc4155_pull_up = false;

#define LTC4155_BASE_ADDRESS    0x12
#define LTC4155_WRITE_ADDRESS   (LTC4155_BASE_ADDRESS + 0)
#define LTC4155_READ_ADDRESS    (LTC4155_BASE_ADDRESS + 1)
/*
#define LTC4155_RETRY_DELAY     1   // us MAGIC
#define LTC4155_MAX_ACK_RETRIES 10  // * LTC4155_RETRY_DELAY us

#define LTC4155_SCL_LOW_PERIOD  2   // 1.3 us
#define LTC4155_SCL_HIGH_PERIOD 1   // 0.6 us
#define LTC4155_BUS_FREE_TIME   2   // 1.3 us
#define LTC4155_STOP_TIME       1   // 0.6 us
*/
enum LTC4155Registers
{
	LTC4155_REG_USB			= 0x00,	// W/R
	LTC4155_REG_WALL		= 0x01,	// W/R
	LTC4155_REG_CHARGE		= 0x02,	// W/R
	LTC4155_REG_STATUS		= 0x03,	// R
	LTC4155_REG_GOOD		= 0x04,	// R
	LTC4155_REG_THERMISTOR	= 0x05,	// R
	LTC4155_REG_ENABLE		= 0x06,	// W/R
	LTC4155_REG_ARM_AND_SHIP= 0x07	// W
};

enum LTC4155InterruptMasks	// LTC4155_REG_ENABLE
{
	LTC4155_ENABLE_USB_OTG	= 1 << 1,
	
	LTC4155_INT_UVCL	= 1 << 2,
	LTC4155_INT_ILIMIT	= 1 << 3,
	LTC4155_INT_USB_OTG	= 1 << 4,
	LTC4155_INT_EXT_PWR	= 1 << 5,
	LTC4155_INT_FAULT	= 1 << 6,
	LTC4155_INT_CHARGER	= 1 << 7
};

enum LTC4155Options	// LTC4155_REG_USB
{
	LTC4155_USB_OTG_LOCKOUT				= 1 << 5,
	LTC4155_ENABLE_BATTERY_CONDITIONER	= 1 << 6,
	LTC4155_DISABLE_INPUT_UVCL			= 1 << 7
};

enum LTC4155Shifts
{
	LTC4155_SHIFTS_CHARGE_CURRENT_LIMIT	= 4,
	LTC4155_SHIFTS_CHARGE_FLOAT_VOLTAGE	= 2,
	LTC4155_SHIFTS_WALL_PRIORITY		= 7,
	LTC4155_SHIFTS_WALL_SAFETY_TIMER	= 5
};

enum LTC4155Statuses	// LTC4155_REG_STATUS
{
	LTC4155_LOW_BATTERY		= 1 << 0,
	LTC4155_BOOST_ENABLE	= 1 << 3,
	LTC4155_ID_PIN_DETECT	= 1 << 4,
};

enum LTC4155Goods	// LTC4155_REG_GOOD
{
	LTC4155_BAD_CELL_FAULT		= 1 << 0,
	LTC4155_OTG_FAULT			= 1 << 1,
	LTC4155_OVP_ACTIVE			= 1 << 2,
	LTC4155_INPUT_UVCL_ACTIVE	= 1 << 3,
	LTC4155_INPUT_CURRENT_LIMIT_ACTIVE = 1 << 4,
	LTC4155_WALLSNS_GOOD		= 1 << 5,
	LTC4155_USBSNS_GOOD			= 1 << 6,
	LTC4155_EXTERNAL_POWER_GOOD	= 1 << 7
};

enum LTC4155BatteryChargerStatues
{
	LTC4155_CHARGER_OFF,
	LTC4155_CHARGER_LOW_BATTERY_VOLTAGE,
	LTC4155_CHARGER_CONSTANT_CURRENT,
	LTC4155_CHARGER_CONSTANT_VOLTAGE_VPROG_GT_VCX,
	LTC4155_CHARGER_CONSTANT_VOLTAGE_VPROG_LT_VCX,
	LTC4155_CHARGER_NTC_TOO_WARM,
	LTC4155_CHARGER_NTC_TOO_COLD,
	LTC4155_CHARGER_NTC_HOT
};

enum LTC4155ThermistorStatuses
{
	LTC4155_NTC_NORMAL,
	LTC4155_NTC_TOO_COLD,
	LTC4155_NTC_TOO_WARM,
	LTC4155_NTC_FAULT
};

static const uint8_t _ltc4155_interrupt_mask =
//	LTC4155_ENABLE_USB_OTG |// Enable +5V on USB connector	// Is this causing the chip to power off the output?!
	LTC4155_INT_UVCL |
	LTC4155_INT_ILIMIT |
	LTC4155_INT_USB_OTG |
	LTC4155_INT_EXT_PWR |	// Turn up current limit
	LTC4155_INT_FAULT |		// Blink error
	LTC4155_INT_CHARGER;	// Illuminate charge LED

static bool _ltc4155_clear_irq(void)
{
	return i2c_write_ex(CHRG_SDA, CHRG_SCL, LTC4155_WRITE_ADDRESS, LTC4155_REG_ENABLE, _ltc4155_interrupt_mask, _ltc4155_pull_up);
}

bool ltc4155_clear_irq(void)
{
	pmc_mask_irqs(true);
	
	bool result = _ltc4155_clear_irq();
	
	pmc_mask_irqs(false);
	
	return result;
}

static uint8_t _ltc4155_last_good, _ltc4155_last_status;

bool _ltc4155_handle_irq(void)
{
	_ltc4155_clear_irq();	// Clear frozen registers to get the real-time ones
	
	_delay_ms(50);	// Wait for registers to clear/update
	
	//////////////////
	
	uint8_t val = 0x00;
	bool result = false;
	
	if (i2c_read2_ex(CHRG_SDA, CHRG_SCL, LTC4155_READ_ADDRESS, LTC4155_REG_GOOD, &val, _ltc4155_pull_up) == false)
		goto _ltc4155_handle_fail;
	
	//if (i2c_read2_ex(CHRG_SDA, CHRG_SCL, LTC4155_READ_ADDRESS, LTC4155_REG_GOOD, &val, _ltc4155_pull_up) == false)
	//	goto _ltc4155_handle_fail;
	
	debug_log_ex("4155GO ", false);
	debug_log_hex(val);
	
	if (val & LTC4155_WALLSNS_GOOD)
	{
		uint8_t wall_state = 0;
		if (i2c_read2_ex(CHRG_SDA, CHRG_SCL, LTC4155_READ_ADDRESS, LTC4155_REG_WALL, &wall_state, _ltc4155_pull_up) == false)
			goto _ltc4155_handle_fail;
		
		wall_state &= ~0x1E;
		wall_state |= 0x0E;
		
		if (i2c_write_ex(CHRG_SDA, CHRG_SCL, LTC4155_WRITE_ADDRESS, LTC4155_REG_WALL, wall_state, _ltc4155_pull_up) == false)
			goto _ltc4155_handle_fail;
		
		debug_log("I+");
	}
	
	_ltc4155_last_good = val;
	
	if (i2c_read2_ex(CHRG_SDA, CHRG_SCL, LTC4155_READ_ADDRESS, LTC4155_REG_STATUS, &val, _ltc4155_pull_up) == false)
		goto _ltc4155_handle_fail;
	
	//if (i2c_read2_ex(CHRG_SDA, CHRG_SCL, LTC4155_READ_ADDRESS, LTC4155_REG_STATUS, &val, _ltc4155_pull_up) == false)
	//	goto _ltc4155_handle_fail;
	
	debug_log_ex("4155ST ", false);
	debug_log_hex(val);
	
	_ltc4155_last_status = val;
	
	val >>= 5;
	
	if (_state.blink_error == BlinkError_None)
	{
		switch (val)
		{
			case LTC4155_CHARGER_CONSTANT_CURRENT:
			case LTC4155_CHARGER_CONSTANT_VOLTAGE_VPROG_GT_VCX:
			case LTC4155_CHARGER_LOW_BATTERY_VOLTAGE:	// If this persists for more than 1/2hr, BAD_CELL_FAULT is enabled and FAULT interrupt is generated
			{
				if ((_state.battery_not_present == false) &&
					(_ltc4155_last_good & (LTC4155_WALLSNS_GOOD | LTC4155_USBSNS_GOOD)))
				{
					//charge_set_led(true);
					charge_notify(true);
					break;
				}						
			}
			case LTC4155_CHARGER_CONSTANT_VOLTAGE_VPROG_LT_VCX:	// Small amount of current still charging the battery but below Vc/x threshold
			//case LTC4155_CHARGER_NTC_TOO_WARM:
			//case LTC4155_CHARGER_NTC_TOO_COLD:
			//case LTC4155_CHARGER_NTC_HOT:
			//	break;
			//case LTC4155_CHARGER_OFF:
			default:
				//charge_set_led(false);
				charge_notify(false);
		}
	}
	
//	ltc4155_dump();
	
	result = true;
_ltc4155_handle_fail:
	_ltc4155_clear_irq();	// Even though it happens first above, this is necessary otherwise future IRQs won't be detected
	
	return result;
}

#define LTC4155_CHARGE_CURRENT_LIMIT	/*0xF*/0x7	// [100%] 50%

bool ltc4155_set_charge_current_limit(uint8_t percentage)
{
	uint8_t val = 0;
	uint8_t limit = 0;
	
	if (percentage > 100)
		return false;
	else if (percentage == 100)
		percentage = 0xF;
	else if (percentage > 12)	// 0..88 -> 0..8800
	{
		uint16_t l = (((uint16_t)percentage - 12) * 100) / 586;
		limit = (uint8_t)l;
	}
	
	if (i2c_read2_ex(CHRG_SDA, CHRG_SCL, LTC4155_WRITE_ADDRESS, LTC4155_REG_CHARGE, &val, _ltc4155_pull_up) == false)
		return false;
	
	val &= ((0x1 << LTC4155_SHIFTS_CHARGE_CURRENT_LIMIT) - 1);
	//val |= (LTC4155_CHARGE_CURRENT_LIMIT << LTC4155_SHIFTS_CHARGE_CURRENT_LIMIT);
	val |= (limit << LTC4155_SHIFTS_CHARGE_CURRENT_LIMIT);
	
	if (i2c_write_ex(CHRG_SDA, CHRG_SCL, LTC4155_WRITE_ADDRESS, LTC4155_REG_CHARGE, val, _ltc4155_pull_up) == false)
		return false;
	
//ltc4155_dump();
	
	return true;
}

bool ltc4155_init(bool disable_charger)
{
	io_input_pin(USBPM_IRQ);
#if !defined(DEBUG) && !defined(ATTINY88_DIP)
	io_set_pin(USBPM_IRQ);	// Enable pull-up for Open Drain
#endif // DEBUG
#ifdef I2C_REWORK
	i2c_init_ex(CHRG_SDA, CHRG_SCL, _ltc4155_pull_up);
#endif // I2C_REWORK
	if (/*_ltc4155_clear_irq()*/_ltc4155_handle_irq() == false)	// Will set interrupt masks	// FIXME: Why does this cause instability?!
		return false;

	const uint8_t charge_state =
		(disable_charger ? 0x0 : LTC4155_CHARGE_CURRENT_LIMIT) << LTC4155_SHIFTS_CHARGE_CURRENT_LIMIT |	// Battery charger I limit = 100%
		0x3 << LTC4155_SHIFTS_CHARGE_FLOAT_VOLTAGE |	// FIXME: Vbatt float = 4.05V - 4.2V for LiPo (default 0x00)
		0x0;	// Full capacity charge threshold = 10%
	if (i2c_write_ex(CHRG_SDA, CHRG_SCL, LTC4155_WRITE_ADDRESS, LTC4155_REG_CHARGE, charge_state, _ltc4155_pull_up) == false)
		return false;

	const uint8_t wall_state =
		0x0 << LTC4155_SHIFTS_WALL_PRIORITY |
		0x0 << LTC4155_SHIFTS_WALL_SAFETY_TIMER |	// Charge safety timer = 4hr	// FIXME: 8hr or Vc/x
		0xE;	// 3 amps, 0x1F - CLPROG1
	if (i2c_write_ex(CHRG_SDA, CHRG_SCL, LTC4155_WRITE_ADDRESS, LTC4155_REG_WALL, wall_state, _ltc4155_pull_up) == false)
		return false;

	// FIXME:
	// Disable ID pin detection & autonomous startup
	// Enable OTG
	//i2c_write_ex(CHRG_SDA, CHRG_SCL, LTC4155_WRITE_ADDRESS, LTC4155_REG_USB, LTC4155_USB_OTG_LOCKOUT, _ltc4155_pull_up);	// Disable autonomous startup
	//i2c_write_ex(CHRG_SDA, CHRG_SCL, LTC4155_WRITE_ADDRESS, LTC4155_REG_ENABLE, LTC4155_ENABLE_USB_OTG, _ltc4155_pull_up);	// Enable OTG
	
	if (_ltc4155_handle_irq() == false)	// One more time (IRQ LED stays lit in dev setup)
		return false;
	
	return true;
}

bool ltc4155_has_interrupt(void)
{
	//bool state = io_test_pin(USBPM_IRQ);
	//debug_log_ex("4155IRQ", false);
	//debug_log_byte(state);
	//return (state != 1);
	return (io_test_pin(USBPM_IRQ) == false);
}

bool ltc4155_handle_irq(void)
{
	pmc_mask_irqs(true);
	
	bool result = _ltc4155_handle_irq();
	
	pmc_mask_irqs(false);
	
	return result;
}

bool ltc4155_arm_ship_and_store(void)
{
	return true;
}

bool ltc4155_get_thermistor(uint8_t* val, bool* warning)
{
	bool result = false;
	uint8_t _val = 0;
	
	pmc_mask_irqs(true);
	
	if (i2c_read2_ex(CHRG_SDA, CHRG_SCL, LTC4155_READ_ADDRESS, LTC4155_REG_THERMISTOR, &_val, _ltc4155_pull_up) == false)
		goto ltc4155_get_thermistor_fail;
	
	if (val)
		(*val) = _val >> 1;
	
	if (warning)
		(*warning) = ((_val & 0x01) != 0x00);
	
	result = true;
ltc4155_get_thermistor_fail:
	pmc_mask_irqs(false);
	return result;
}

void ltc4155_dump(void)
{
	pmc_mask_irqs(true);
	
	uint8_t val = 0x00;
	bool warning = false;
	
	if (ltc4155_get_thermistor(&val, &warning) == false)
		goto ltc4155_dump_fail;
	
	debug_log_ex("\tTHRM", false);
	if (warning)
		debug_log_ex("!", false);
	debug_log_byte(val);
	
	if (i2c_read2_ex(CHRG_SDA, CHRG_SCL, LTC4155_READ_ADDRESS, LTC4155_REG_WALL, &val, _ltc4155_pull_up) == false)
		goto ltc4155_dump_fail;
	
	debug_log_ex("\tWALL", false);
	debug_log_hex(val);
	
	if (i2c_read2_ex(CHRG_SDA, CHRG_SCL, LTC4155_READ_ADDRESS, LTC4155_REG_GOOD, &val, _ltc4155_pull_up) == false)
		goto ltc4155_dump_fail;
	
	debug_log_ex("\t4155GO ", false);
	debug_log_hex(val);
	
	if (i2c_read2_ex(CHRG_SDA, CHRG_SCL, LTC4155_READ_ADDRESS, LTC4155_REG_STATUS, &val, _ltc4155_pull_up) == false)
		goto ltc4155_dump_fail;
	
	debug_log_ex("\t4155ST ", false);
	debug_log_hex(val);
	
ltc4155_dump_fail:
	pmc_mask_irqs(false);
}

#endif // !CHARGER_TI
