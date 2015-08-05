/*
 * global.h
 *
 * Copyright 2015 National Instruments Corp
 */ 

#ifndef GLOBAL_H_
#define GLOBAL_H_

#include <stdbool.h>
#include <stdint.h>
#include <avr/pgmspace.h>

typedef struct State
{
	bool interrupts_enabled;
	uint8_t interrupt_depth;
	//bool timers_running;
	uint8_t active_timers;
	bool powered;
	bool battery_not_present;
	bool battery_charging;
	bool wake_up;
	bool power_off;
	bool core_power_bad;
	bool ltc3675_irq;
#ifdef CHARGER_TI
	bool bq24190_irq;
#else
	bool ltc4155_irq;
#endif // CHARGER_TI
	//bool low_battery;
	uint8_t blink_error;
	uint8_t blinker_state;
	uint8_t blink_loops;
	uint8_t blink_last_loop;
	bool blink_stop;
} STATE;

//extern volatile bool _timers_running;
extern volatile STATE _state;

void pmc_set_blink_error(uint8_t count);
uint8_t pmc_get_blink_error(void);

bool pmc_mask_irqs(bool mask);

#endif /* GLOBAL_H_ */
