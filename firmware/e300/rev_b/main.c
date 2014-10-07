/*
 * Copyright 2009 Ettus Research LLC
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

#include "global.h"
#include "power.h"
#include "debug.h"
#include "error.h"
#include "ltc3675.h"
#ifdef CHARGER_TI
#include "bq24190.h"
#else
#include "ltc4155.h"
#endif // CHARGER_TI

#define AUTO_POWER_ON

#define INITIAL_DELAY	250	// ms

FUSES = {	// FIXME: & FUSE_CKSEL1 for low power 128 kHz clock
	.low = (FUSE_CKSEL0 & FUSE_SUT0 & FUSE_CKDIV8),	// Internal 8MHz Oscillator, Slowly rising power (start-up time), Divide Clock by 8
	.high = (FUSE_EESAVE & FUSE_SPIEN),	// Save EEPROM between flashes	// FIXME: Leave SPIEN for programming enabled?
};	// Not using watchdog as it runs during sleep and consumes power

volatile STATE _state;

/*
    - Main/shared variables must be volatile
	- Port pins are tri-stated on reset
	* AVR_IRQ PD(5)
	- Enable pull-ups on all O.D. outputs from regulator chip
	* PS_POR/SRST should be driven HIGH by ATTiny?
	- AVR_RESET -> RESET pin - don't configure fuse (this would disable this functionality and prohibit serial programming)
	* Ship-and-store mode for charge controller?
	* cli before I2C calls
	* PS_TX
	- en5-clk, en2-data
	* Instruction following SEI is executed before interrupts
	* LTC3675 real-time status doesn't contain UV/OT
	* LTC3675 PGOOD -> power down (no point in checking blink state)
	* On WALL, use TX, on battery use OTG switcher
	* PRR - Power Reduction Register (p40)
	- 100% -> 50% battery charge limit
	* Check latched status for UV/OT in 3675
	* If blink error reset, get latest charge status from 4155
	* Fix UV status check from 3675/4155 as they currently share the same error 
	* Use charger termination at 8hr or Vc/x
	* Check PGood on all regs after power on before setting powered=true
	* Re-init 4155 on soft-power on
	- Re-set 3A limit in 4155 after external power connection
	- Removing power when running on battery, 4155GO 0xA0 - but WALL has been removed
	- Why is charger reporting Constant Current when power is removed
	* ltc3675_is_power_button_depressed: check if any reg is on, otherwise value will be invalid
	* When e.g. 3.3V doesn't come up, blink code is correctly 4 but there's a very short blink before re-starting the sequence
	- Vprog<Vc/x
*/

bool pmc_mask_irqs(bool mask)
{
	if (_state.interrupts_enabled == false)
		return false;
	
	if (mask)
	{
		if (_state.interrupt_depth == 0)
			cli();
		++_state.interrupt_depth;
	}		
	else
	{
		if (_state.interrupt_depth == 0)
			return false;
		
		--_state.interrupt_depth;
		if (_state.interrupt_depth == 0)
			sei();
	}
	
	return true;
}

int main(void)
{
	_delay_ms(INITIAL_DELAY);
	
	///////////////////////////////////////////////////////////////////////////
	
	memset((void*)&_state, 0x00, sizeof(STATE));
	
	debug_init();
	debug_blink(1);
	
	//debug_log("#");	// Will not boot if this is 21 chars long?!
	debug_log("Hello world");
	
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);    // SLEEP_MODE_PWR_SAVE combination is documented as Reserved
	
//ltc4155_dump();

    // FIXME: Init as SPI slave (FPGA is master)
	
	// 8-bit timer for blinking errors on charge LED
	TCCR0A = _BV(CTC0);		// CTC mode
	OCR0A = 244;			// 250ms with 1024 prescale
	TIMSK0 = _BV(OCIE0A);	// Enable CTC on Timer 0

	bool init_result = power_init();
	debug_log_ex("Init", false);
	_debug_log(init_result ? "+" : "-");
	debug_blink(2);
	//debug_blink_rev(6);
	
	///////////////////////////////////
#ifdef AUTO_POWER_ON
	power_on(); // Turn on immediately. Need to de-press power button to turn off.
	debug_log("Power");
	debug_blink(3);
	//debug_blink_rev(10);

	//debug_wait();
	
//ltc4155_dump();
#endif // AUTO_POWER_ON
	_state.interrupts_enabled = true;
	sei();	// Enable interrupts

	asm("nop");

	_state.wake_up = false;	// This will fire the first time the regs are turned on
	
	bool one_more = false;
	
	while (true)
	{
		one_more = false;
#ifdef CHARGER_TI
		if (_state.bq24190_irq)
		{
			bq24190_handle_irq();
			
			_state.bq24190_irq = false;
		}
#else
		if ((_state.ltc4155_irq)/* || ltc4155_has_interrupt()*/)	// [Don't know why PCINT ISR misses LTC4155 IRQ on power up, so double-check state of line]
		{
			ltc4155_handle_irq();
//ltc4155_dump();
			_state.ltc4155_irq = false;
		}
#endif // !CHARGER_TI
		if (_state.core_power_bad)	// FIXME: Check whether it's supposed to be on
		{
			if (power_is_subsys_on(PS_FPGA))
			{
				_delay_ms(1);	// Seeing weird 120us drop in PGOOD during boot from flash (no apparent drop in 1.0V though)
				
				if (tps54478_is_power_good() == false)
				{
					debug_log("ML:FPGA!");
			
					//power_off();
					_state.power_off = true;
			
					/*while (_state.wake_up == false)
					{
						blink_error_sequence(1);
					}*/
					pmc_set_blink_error(BlinkError_FPGA_Power);	// [If a blink error was set in power_off, this will supercede it]
				}
			}			
			
			_state.core_power_bad = false;
		}
		
		if ((_state.ltc3675_irq)/* || ltc3675_has_interrupt()*/)	// This is fired on initial power up
		{
			debug_log("ML:3675+");
			
			ltc3675_handle_irq();
			
			if (ltc3675_is_power_good(ltc3675_get_last_status()) == false)
			{
				debug_log("ML:3675!");
				
				//power_off();
				_state.power_off = true;
			}
			
			_state.ltc3675_irq = false;
		}
		
		if (_state.power_off)
		{
			debug_log("ML:Off..");
			
			power_off();
			
			_state.power_off = false;
			_state.wake_up = false;
		}
		else if (_state.wake_up)
		{
			_delay_ms(1);	// Tapping 3.1 ohm load ing 4155 in dev setup causes transient on this line and causes power on sequence to begin again
			
			//if (_state.powered == false)	// Don't check in case button is held long enough to force LTC3675 shutdown (will not change 'powered' value)
			if (ltc3675_is_waking_up())
			{
				debug_log("ML:On..");
				
				power_on();
			}
			
			_state.wake_up = false;
		}
		
		// Check to see if the error state has resolved itself at the end of each sequence of the current blink error
		
		if ((_state.blink_error != BlinkError_None) && (_state.blink_last_loop != _state.blink_loops))
		{
			// [Check IRQs periodically]
			
			bool ltc3675_use_last_status = false;
			/*if (ltc3675_has_interrupt())
			{
				//debug_set(IO_PB(6), ((_state.blink_loops % 2) == 0));
				ltc3675_use_last_status = true;
				ltc3675_handle_irq();
			}*/
			
			///////////////////////////
			
			switch (_state.blink_error)
			{
				case BlinkError_LTC3675_UnderVoltage:
				case BlinkError_LTC3675_OverTemperature:
				case BlinkError_DRAM_Power:
				case BlinkError_3_3V_Peripherals_Power:
				case BlinkError_1_8V_Peripherals_Power:
				case BlinkError_TX_Power:
					if (((ltc3675_use_last_status) && (ltc3675_status_to_error(ltc3675_get_last_status()) != BlinkError_None)) || 
						((ltc3675_use_last_status == false) && (ltc3675_check_status() != BlinkError_None)))
						break;
					debug_log("BE:3675-");
					goto cancel_blink_error;
				case BlinkError_FPGA_Power:
					if (tps54478_is_power_good() == false)
						break;
					debug_log("BE:FPGA-");
					goto cancel_blink_error;
				default:
cancel_blink_error:				
					//debug_set(IO_PB(7), true);
					pmc_set_blink_error(BlinkError_None);
			}
			
			////////////////////////////////////
			
			// More periodic checks
			// Need to do this has some interrupts are on PCINT, and while GIE is disabled, might change & change back
			//	E.g. LTC3675 IRQ due to UV, reset IRQ, re-asserts UV
#ifndef CHARGER_TI
			if (ltc4155_has_interrupt())
			{
				debug_log("BE:4155");
				
				_state.ltc4155_irq = true;
				one_more = true;
			}
#endif // !CHARGER_TI
			if (ltc3675_has_interrupt())
			{
				debug_log("BE:3675");
				
				_state.ltc3675_irq = true;
				one_more = true;
			}
			
			if (power_is_subsys_on(PS_FPGA))
			{
				if (tps54478_is_power_good() == false)
				{
					debug_log("BE:FPGA!");
				
					_state.core_power_bad = true;
					one_more = true;
				}
			}
			
			////////////////////////////////////
			
			_state.blink_last_loop = _state.blink_loops;
		}
		
		//if (_state.timers_running == false)
		if ((_state.active_timers == 0) && (one_more == false))
		{
			debug_log("^");
			sleep_mode();
			debug_log("$");
		}			
	}

	return 0;
}

uint8_t pmc_get_blink_error(void)
{
	return _state.blink_error;
}

void pmc_set_blink_error(uint8_t count)
{
	if ((_state.blink_error != BlinkError_None) && (count /*> _state.blink_error*/!= BlinkError_None))	// [Prioritise] Always keep first sequence running
		return;
	else if (_state.blink_error == count)	// Don't restart if the same
		return;
	
	if (count == BlinkError_None)
	{
		debug_log("BLNK-");
		_state.blink_stop = true;
		return;
	}
	
	//char msg[25];
	//sprintf(msg, "Blink code = %i\n", count);
	//debug_log(msg);
	debug_log_ex("BLNK ", false);
	debug_log_byte(count);
	
	_state.blink_error = count;
	_state.blink_loops = 0;
	_state.blink_last_loop = 0;
	_state.blinker_state = 0;
	_state.blink_stop = false;

	charge_set_led(false);
	
	TCNT0 = 0;
	if ((TCCR0A & 0x07) == 0x00)	// Might already be active with existing error
		_state.active_timers++;
	TCCR0A |= 0x05;	// Start with 1024 prescale
}

ISR(TIMER0_COMPA_vect)	// Blink the sequence, and leave one slot at the beginning and end where the LED is off so one can get a sense of how many blinks occurred
{
	pmc_mask_irqs(true);
	
	if (_state.blinker_state < (2 * _state.blink_error + 1))
		charge_set_led((_state.blinker_state % 2) == 1);
	
	_state.blinker_state++;
	
	if (_state.blinker_state == (2 * _state.blink_error + 1 + 1))
	{
		_state.blinker_state = 0;
		
		if (_state.blink_stop)
		{
			if ((TCCR0A & 0x07) != 0x00)
				_state.active_timers--;
			TCCR0A &= ~0x07;
			
			_state.blink_error = BlinkError_None;
			
			debug_log("BLNK.");
		}
		else
		{
			_state.blink_loops++;
		}
	}
	
	pmc_mask_irqs(false);
}
