#ifndef LED_H
#define LED_H

#include <stdint.h>

#include "interrupt.h"

enum led_state {
	LED_BLINK_GREEN_SLOW,
	LED_BLINK_GREEN_FAST,
	LED_BLINK_RED_FAST,
	LED_BLINK_ORANGE,
	LED_ORANGE,
	LED_GREEN,
	LED_RED,
	LED_OFF
};

void led_set_blink_seq(uint8_t n_blinks, enum led_state state);

void led_set_blink(enum led_state state);

void led_set_solid(enum led_state state);

extern irqreturn_t led_wdt_handler(void);

#endif /* LED_H */
