/*
 * Copyright 2012 Ettus Research LLC
 */

/*
    ? STOP condition after writing address on read
    - Default buck/boost register values are OK
*/

#include "config.h"
#include "ltc3675.h"

//#include <stdio.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include "io.h"
#include "i2c.h"
#include "debug.h"
#include "global.h"
#include "error.h"

#ifndef I2C_REWORK
#include "power.h"
#endif // I2C_REWORK

const bool _ltc3675_pull_up =
#ifdef I2C_REWORK
	true
#else
	false
#endif // I2C_REWORK
;

volatile ltc3675_reg_helper_fn _ltc3675_reg_helper;

//#define HARDWIRE_ENABLE	// Use hardware enable pins instead of I2C on regulators that support it

#ifdef ATTINY88_DIP

#ifdef HARDWIRE_ENABLE
static io_pin_t PWR_EN1     = IO_PC(7);	// Not routed by card
static io_pin_t PWR_EN2     = IO_PA(0);	// Not available on DIP
static io_pin_t PWR_EN3     = IO_PA(1);	// Not available on DIP
static io_pin_t PWR_EN4     = IO_PB(6);	// Instead of FTDI_BCD
static io_pin_t PWR_EN5     = IO_PB(7);	// Instead of FTDI_PWREN2
#endif // HARDWIRE_ENABLE

//static io_pin_t PWR_SDA     = IO_PC(4);
//static io_pin_t PWR_SCL     = IO_PC(5);

#else

#ifdef HARDWIRE_ENABLE
static io_pin_t PWR_EN1     = IO_PC(1);
//static io_pin_t PWR_EN2     = IO_PC(2);	// Now used by I2C for charge controller
//static io_pin_t PWR_EN3     = IO_PC(3);	// Now used by I2C for charge controller
static io_pin_t PWR_EN4     = IO_PA(1);
static io_pin_t PWR_EN5     = IO_PA(2);
#endif // HARDWIRE_ENABLE

#ifdef I2C_REWORK
static io_pin_t PWR_SDA     = IO_PC(2);		// Instead of EN5
static io_pin_t PWR_SCL     = IO_PA(2);		// Instead of EN2
#endif // I2C_REWORK

#endif // ATTINY88_DIP

static io_pin_t PWR_IRQ     = IO_PD(0);
static io_pin_t WAKEUP      = IO_PD(2);
static io_pin_t ONSWITCH_DB = IO_PD(3);
static io_pin_t PWR_RESET   = IO_PD(4);

#define LTC3675_BASE_ADDRESS    0x12
#define LTC3675_WRITE_ADDRESS   (LTC3675_BASE_ADDRESS + 0)
#define LTC3675_READ_ADDRESS    (LTC3675_BASE_ADDRESS + 1)

#define LTC3675_RETRY_DELAY     1   // us MAGIC
#define LTC3675_MAX_ACK_RETRIES 10  // * LTC3675_RETRY_DELAY us

#define LTC3675_SCL_LOW_PERIOD  2   // 1.3 us
#define LTC3675_SCL_HIGH_PERIOD 1   // 0.6 us
#define LTC3675_BUS_FREE_TIME   2   // 1.3 us
#define LTC3675_STOP_TIME       1   // 0.6 us

#define LTC3675_REGULATOR_ENABLE_DELAY	10	// 50	// ms (some arbitrary value so that the external power supply can settle)

enum LTC3675Registers
{
	LTC3675_REG_NONE			= 0x00,
	LTC3675_REG_BUCK1			= 0x01,
	LTC3675_REG_BUCK2			= 0x02,
	LTC3675_REG_BUCK3			= 0x03,
	LTC3675_REG_BUCK4			= 0x04,
	LTC3675_REG_BOOST			= 0x05,
	LTC3675_REG_BUCK_BOOST		= 0x06,
	LTC3675_REG_LED_CONFIG		= 0x07,
	LTC3675_REG_LED_DAC			= 0x08,
	LTC3675_REG_UVOT			= 0x09,
	LTC3675_REG_RSTB			= 0xA0,
	LTC3675_REG_IRQB_MASK		= 0x0B,
	LTC3675_REG_REALTIME_STATUS	= 0x0C,
	LTC3675_REG_LATCHED_STATUS	= 0x0D,
	LTC3675_REG_CLEAR_IRQ		= 0x0F
};

enum LTC3675StatusBits
{
	LTC3675_UnderVoltage	= 1 << 7,
	LTC3675_OverTemperature	= 1 << 6,
	LTC3675_BuckBoost_PGood	= 1 << 5,
	LTC3675_Boost_PGood		= 1 << 4,
	LTC3675_Buck4_PGood		= 1 << 3,
	LTC3675_Buck3_PGood		= 1 << 2,
	LTC3675_Buck2_PGood		= 1 << 1,
	LTC3675_Buck1_PGood		= 1 << 0
};

#define LTC3675_DEFAULT_BUCK_REG_VAL		0x6F
#define LTC3675_DEFAULT_BOOST_REG_VAL		0x0F
#define LTC3675_DEFAULT_BUCK_BOOST_REG_VAL	0x0F

#define LTC3675_ENABLE_REGISTER_BIT			0x80

// Max I2C rate = 400kHz

static void _ltc3675_clear_irq()
{
	// Two-stage clear
	i2c_write_ex(PWR_SDA, PWR_SCL, LTC3675_WRITE_ADDRESS, LTC3675_REG_CLEAR_IRQ, 0x00, _ltc3675_pull_up);
	i2c_write_ex(PWR_SDA, PWR_SCL, LTC3675_WRITE_ADDRESS, LTC3675_REG_NONE, 0x00, _ltc3675_pull_up);
}

volatile uint8_t _ltc3675_last_status = 0x00;

uint8_t ltc3675_get_last_status(void)
{
	return _ltc3675_last_status;
}

uint8_t ltc3675_reg_status_to_error(uint8_t val)
{
	if (((val & LTC3675_BuckBoost_PGood) == 0) && ((_ltc3675_reg_helper)(LTC3675_REG_6)))
		return BlinkError_3_3V_Peripherals_Power;
	
	if (((val & LTC3675_Boost_PGood) == 0) && ((_ltc3675_reg_helper)(LTC3675_REG_5)))
		return BlinkError_TX_Power;
	
	//if (((val & LTC3675_Buck4_PGood) == 0) && ((_ltc3675_reg_helper)(LTC3675_REG_4)))
	
	if (((val & LTC3675_Buck3_PGood) == 0) && ((_ltc3675_reg_helper)(LTC3675_REG_3)))
		return BlinkError_1_8V_Peripherals_Power;
	
	//if (((val & LTC3675_Buck2_PGood) == 0) && ((_ltc3675_reg_helper)(LTC3675_REG_2)))
	
	if (((val & LTC3675_Buck1_PGood) == 0) && ((_ltc3675_reg_helper)(LTC3675_REG_1)))
		return BlinkError_DRAM_Power;
	
	return BlinkError_None;
}

bool ltc3675_is_power_good(uint8_t val)
{
	return (ltc3675_reg_status_to_error(val) == BlinkError_None);
}

uint8_t ltc3675_status_to_error(uint8_t val)
{
	if (val & LTC3675_UnderVoltage)
		return BlinkError_LTC3675_UnderVoltage;
	
	if (val & LTC3675_OverTemperature)
		return BlinkError_LTC3675_OverTemperature;
	
	uint8_t reg_error = ltc3675_reg_status_to_error(val);
	if (reg_error != BlinkError_None)
		return reg_error;
	
	return BlinkError_None;
}

bool _ltc3675_handle_irq(void)
{
	uint8_t val = 0x00;
	bool result = false;
	
	if (i2c_read2_ex(PWR_SDA, PWR_SCL, LTC3675_READ_ADDRESS, LTC3675_REG_LATCHED_STATUS, &val, _ltc3675_pull_up))
	{
		debug_log_ex("3675LTCH ", false);
		debug_log_hex(val);
	}
	
	if (i2c_read2_ex(PWR_SDA, PWR_SCL, LTC3675_READ_ADDRESS, /*LTC3675_REG_LATCHED_STATUS*/LTC3675_REG_REALTIME_STATUS, &val, _ltc3675_pull_up))	// No point acting on latched because could have been resolved
	{
		//debug_log_ex("3675LTCH ", false);
		debug_log_ex("3675RT ", false);
		debug_log_hex(val);
		
		_ltc3675_last_status = val;
		
		uint8_t error = ltc3675_status_to_error(val);
		
		/*if (val & LTC3675_UnderVoltage)
		{
			pmc_set_blink_error(BlinkError_LTC3675_UnderVoltage);
			//_state.low_battery = true;
		}*/
		
		if (error)
		{
			pmc_set_blink_error(error);
			
			/*_i2c_disable_ack_check = true;
			uint8_t chk = 0x00;
			chk |= (_ltc3675_reg_helper)(LTC3675_REG_6) << 0;
			chk |= (_ltc3675_reg_helper)(LTC3675_REG_5) << 1;
			chk |= (_ltc3675_reg_helper)(LTC3675_REG_3) << 2;
			chk |= (_ltc3675_reg_helper)(LTC3675_REG_1) << 3;
			i2c_write_ex(PWR_SDA, PWR_SCL, 0xFE, 0xFF, chk, _ltc3675_pull_up);
			_i2c_disable_ack_check = false;*/
		}			
		
		result = true;
	}
	
	_ltc3675_clear_irq();
	
	return result;
}

static bool _ltc3675_get_realtime_status(uint8_t* val)
{
	//cli();
	
	if (i2c_read2_ex(PWR_SDA, PWR_SCL, LTC3675_READ_ADDRESS, LTC3675_REG_REALTIME_STATUS, val, _ltc3675_pull_up) == false)
		return false;
	
	debug_log_ex("3675RT ", false);
	debug_log_hex(*val);
	
	//sei();
	
	return true;
}

int8_t ltc3675_check_status(void)
{
	uint8_t val = 0x00;
	
	pmc_mask_irqs(true);
	
	bool result = _ltc3675_get_realtime_status(&val);
	
	pmc_mask_irqs(false);
	
	if (result == false)
		return -1;
	
	//_ltc3675_last_status = val;
	
	/*if (val & LTC3675_UnderVoltage)
		return BlinkError_LTC3675_UnderVoltage;
	
	if (val & LTC3675_OverTemperature)
		return BlinkError_LTC3675_OverTemperature;
	
	return BlinkError_None;*/
	
	return ltc3675_status_to_error(val);
}

bool ltc3675_handle_irq(void)
{
	pmc_mask_irqs(true);
	
	/*uint8_t*/bool result = _ltc3675_handle_irq();
	
	pmc_mask_irqs(false);
	
	return result;
}

static bool _ltc3675_default_reg_helper(uint8_t address)
{
	uint8_t val = 0x00;
	i2c_read2_ex(PWR_SDA, PWR_SCL, LTC3675_READ_ADDRESS, address, &val, _ltc3675_pull_up);
	return ((val & LTC3675_ENABLE_REGISTER_BIT) == LTC3675_ENABLE_REGISTER_BIT);
}

bool ltc3675_init(ltc3675_reg_helper_fn helper)
{
	if (helper)
		_ltc3675_reg_helper = helper;
	else
		_ltc3675_reg_helper = _ltc3675_default_reg_helper;
#ifdef HARDWIRE_ENABLE
    io_output_pin(PWR_EN1);
    io_output_pin(PWR_EN2);
    io_output_pin(PWR_EN3);
    io_output_pin(PWR_EN4);
    io_output_pin(PWR_EN5);
#endif // HARDWIRE_ENABLE

 /*	io_output_pin(PWR_SDA);
    io_output_pin(PWR_SCL);

    // Must remain HIGH when idle
    io_set_pin(PWR_SDA);
    io_set_pin(PWR_SCL);
*/
#ifdef I2C_REWORK
	i2c_init_ex(PWR_SDA, PWR_SCL, _ltc3675_pull_up);
#endif // I2C_REWORK
    io_input_pin(PWR_IRQ);
#if !defined(DEBUG) && !defined(ATTINY88_DIP)
	io_set_pin(PWR_IRQ);	// Enable pull-up for Open Drain
#endif // DEBUG
	
    io_input_pin(WAKEUP);
	io_set_pin(WAKEUP);	// Enable pull-up for Open Drain
	
    io_input_pin(ONSWITCH_DB);
	io_set_pin(ONSWITCH_DB);	// Enable pull-up for Open Drain
	
    io_input_pin(PWR_RESET);
	io_set_pin(PWR_RESET);	// Enable pull-up for Open Drain
	
	_ltc3675_clear_irq();	// Clear old interrupt - state might have changed (e.g. undervoltage might have been resolved)

    if (i2c_write_ex(PWR_SDA, PWR_SCL, LTC3675_WRITE_ADDRESS, LTC3675_REG_IRQB_MASK, 0xFF, _ltc3675_pull_up) == false)	// Any PGOOD fault will pull IRQB low
		return false;
	
	if (i2c_write_ex(PWR_SDA, PWR_SCL, LTC3675_WRITE_ADDRESS, LTC3675_REG_UVOT, 0x70, _ltc3675_pull_up) == false)	// 3.4V UV
		return false;
	
	if (ltc3675_has_interrupt())
		_ltc3675_handle_irq();
	
	// Non-maskable:
	//	UV warning threshold (default): 2.7V
	//	Over temp warning threshold (default): 10 degrees below

    return true;
}

bool ltc3675_is_waking_up(void)
{
	return io_test_pin(WAKEUP);
}

static bool _ltc3675_is_pgood(uint8_t reg)
{
	uint8_t val = 0x00;
	if (_ltc3675_get_realtime_status(&val) == false)
		return false;
	return ((reg & val) == reg);
}

static bool _ltc3675_toggle_reg(uint8_t addr, uint8_t def_reg, bool on)
{
	bool result = true;
	
	//cli();
	
	uint8_t val = 0x00 | def_reg;
	if (i2c_read2_ex(PWR_SDA, PWR_SCL, LTC3675_READ_ADDRESS, addr, &val, _ltc3675_pull_up) == false)
		return false;
	
	val &= ~LTC3675_ENABLE_REGISTER_BIT;
	
	if (i2c_write_ex(PWR_SDA, PWR_SCL, LTC3675_WRITE_ADDRESS, addr, /*def_reg*/val | (on ? LTC3675_ENABLE_REGISTER_BIT : 0x00), _ltc3675_pull_up) == false)
		//return true;
		result = false;
	
	if (on)
	{
		_delay_ms(LTC3675_REGULATOR_ENABLE_DELAY);
	}
	
	//sei();

	return result;
	//return true;
}

bool ltc3675_enable_reg(ltc3675_regulator_t reg, bool on)
{
	//debug_blink2(reg + 1);
	debug_log_ex("3675 ", false);
	debug_log_byte_ex(reg, true);
	
	// Sub-address: index of regulator
	// Data: <default reg contents> | <enable>
	
	bool result = false;
	
    switch (reg)
    {
        case LTC3675_REG_1: // Master
        case LTC3675_REG_2: // Slave
#ifdef HARDWIRE_ENABLE
            io_enable_pin(PWR_EN1, on);
			//break;
#else
			//debug_blink2(reg + 1);
			if (_ltc3675_toggle_reg(LTC3675_REG_BUCK1, LTC3675_DEFAULT_BUCK_REG_VAL, on) == false) {
				//debug_blink2(reg + 1);
				return false;
			}
			//debug_blink2(reg + 1);
#endif // HARDWIRE_ENABLE
			result = (_ltc3675_is_pgood(LTC3675_Buck1_PGood) == on);
			break;
        case LTC3675_REG_3: // Master
        case LTC3675_REG_4: // Slave
#ifdef HARDWIRE_ENABLE
            io_enable_pin(PWR_EN3, on);
            //break;
#else
			if (_ltc3675_toggle_reg(LTC3675_REG_BUCK3, LTC3675_DEFAULT_BUCK_REG_VAL, on) == false)
				return false;
#endif // HARDWIRE_ENABLE
			result = (_ltc3675_is_pgood(LTC3675_Buck3_PGood) == on);
			break;
        case LTC3675_REG_5: // I2C only
            if (_ltc3675_toggle_reg(LTC3675_REG_BOOST, LTC3675_DEFAULT_BOOST_REG_VAL, on) == false)    // (Boost address, Default reg contents | Enable)
				return false;
			result = (_ltc3675_is_pgood(LTC3675_Boost_PGood) == on);
			break;
        case LTC3675_REG_6: // Single
#ifdef HARDWIRE_ENABLE
            io_enable_pin(PWR_EN5, on);
            //break;
#else
			if (_ltc3675_toggle_reg(LTC3675_REG_BUCK_BOOST, LTC3675_DEFAULT_BUCK_BOOST_REG_VAL, on) == false)
				return false;
#endif // HARDWIRE_ENABLE
			result = (_ltc3675_is_pgood(LTC3675_BuckBoost_PGood) == on);
			break;
        //default:
		//	return false;
    }
	
	_debug_log((result ? "+" : "-"));

    return result;
}

bool ltc3675_set_voltage(ltc3675_regulator_t reg, uint16_t voltage)
{
    // Not necessary due to R-bridges and default DAC registers

    // VRAM will be 1.3579 - a little high? (re-program DAC reference)
    //  No: minimum FB step will put Vout < 1.35
	
	uint16_t max_voltage = 0;
	uint8_t reg_subaddr = 0;
	
	switch (reg)
	{
		case LTC3675_REG_1:	// 1A Buck
		case LTC3675_REG_2:	// 1A Buck
			max_voltage = 1500;
			reg_subaddr = LTC3675_REG_BUCK1;
			break;
		case LTC3675_REG_3:	// 500mA Buck
		case LTC3675_REG_4:	// 500mA Buck
			max_voltage = 1800;
			reg_subaddr = LTC3675_REG_BUCK3;
			break;
		case LTC3675_REG_5:	// 1A Boost
			max_voltage = 5000;
			reg_subaddr = LTC3675_REG_BOOST;
			break;
		case LTC3675_REG_6:	// 1A Buck-Boost
			max_voltage = 3300;
			reg_subaddr = LTC3675_REG_BUCK_BOOST;
			break;
	}
	
	if (voltage > max_voltage)
		return false;
	
	//uint32_t rMax = ((uint32_t)voltage * 1000) / (uint32_t)max_voltage;
	//uint32_t rFB = ((uint32_t)max_voltage * 1000) / (uint32_t)800;
	uint32_t rFB = ((uint32_t)max_voltage * 1000) / (uint32_t)800;	// 800mV full-scale feedback voltage
	uint32_t r = ((uint32_t)voltage * 1000) / (uint32_t)rFB;
	if (r < 450)
		return false;
	
	uint16_t rDAC = (16 * ((uint16_t)r - 450)) / (800 - 450);
	
	debug_log_ex("Vr ", false);
	debug_log_byte_ex(reg, false);
	debug_log_ex("=", false);
	debug_log_byte_ex((uint8_t)rDAC, false);
	
	uint8_t val = 0x00;
	if (i2c_read2_ex(PWR_SDA, PWR_SCL, LTC3675_READ_ADDRESS, reg_subaddr, &val, _ltc3675_pull_up) == false)
	{
		debug_log("-");
		return false;
	}		
	
	val = (val & 0xF0) | (uint8_t)rDAC;
	if (i2c_write_ex(PWR_SDA, PWR_SCL, LTC3675_WRITE_ADDRESS, reg_subaddr, val, _ltc3675_pull_up) == false)
	{
		debug_log("-");
		return false;
	}
	
	debug_log("+");

	return true;
}

bool ltc3675_is_power_button_depressed(void)
{
	return (io_test_pin(ONSWITCH_DB) == false);
}

bool ltc3675_has_interrupt(void)
{
	return (io_test_pin(PWR_IRQ) == false);
}
