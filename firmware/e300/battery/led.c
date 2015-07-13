#include "led.h"
#include "io.h"

/* hardware io */
static io_pin_t POWER_LED = IO_PC(7);
static io_pin_t CHARGE = IO_PD(1);

enum led_color {
	LED_C_RED,
	LED_C_GREEN,
	LED_C_OFF
};

static enum led_color led_color;

static inline void led_set(enum led_color color)
{
	switch (color) {
	case LED_C_RED:
		io_clear_pin(POWER_LED);
		io_set_pin(CHARGE);
		break;
	case LED_C_GREEN:
		io_clear_pin(CHARGE);
		io_set_pin(POWER_LED);
		break;
	case LED_C_OFF:
	default:
		io_clear_pin(CHARGE);
		io_clear_pin(POWER_LED);
		break;
	}
	led_color = color;
}

/* blinken lights */
static uint8_t blink_cnt;
static uint8_t orange_cnt;

/* state for sequence */
static enum led_state state;
static enum led_state state_next;
static uint8_t seq_max;
static uint8_t seq_cnt;
static const uint8_t T_SEQ = 196;
static const uint8_t T_ON = 98;

static bool counting;

void led_set_blink_seq(uint8_t n_blinks, enum led_state color)
{
	if (color == state)
		return;

	blink_cnt = 0;
	seq_cnt = 0;
	seq_max = 2 * n_blinks + 1;

	state = color;
}

void led_set_solid(enum led_state color)
{
	if (state != LED_BLINK_RED_FAST)
		state = color;
	state_next = color;
}

void led_set_blink(enum led_state color)
{
	if (state != LED_BLINK_RED_FAST)
		state = color;
	state_next = color;
}

irqreturn_t led_wdt_handler(void)
{
	counting = false;
	switch (state) {
	case LED_BLINK_GREEN_SLOW:
		if (blink_cnt < T_ON)
			led_set(LED_C_GREEN);
		else
			led_set(LED_C_OFF);
		blink_cnt += 1;
		break;

	case LED_BLINK_GREEN_FAST:
		if (blink_cnt < T_ON)
			led_set(LED_C_GREEN);
		else
			led_set(LED_C_OFF);
		blink_cnt += 4;
		break;

	case LED_BLINK_RED_FAST:
		counting = true;
		if (!seq_cnt) {
			led_set(LED_C_OFF);
		} else if (blink_cnt < T_ON)
			led_set(seq_cnt % 2 ? LED_C_OFF : LED_C_RED);
		else
			led_set(LED_C_OFF);
		blink_cnt += 16;
		break;

	case LED_BLINK_ORANGE:
		if (blink_cnt < T_ON)
			led_set(orange_cnt % 2 ? LED_C_GREEN : LED_C_RED);
		else
			led_set(LED_C_OFF);

		orange_cnt++;
		blink_cnt += 4;
		break;

	case LED_ORANGE:
		led_set(orange_cnt % 2 ? LED_C_GREEN : LED_C_RED);
		orange_cnt++;
		blink_cnt+=4;
		break;

	case LED_GREEN:
		led_set(LED_C_GREEN);
		break;

	case LED_RED:
		led_set(LED_C_RED);
		break;

	case LED_OFF:
	default:
		led_set(LED_C_OFF);
		break;
	}

	if (blink_cnt >= T_SEQ - 1) {
		blink_cnt = 0;
		if (counting) {
			if (seq_cnt < seq_max) {
				seq_cnt++;
			} else {
				state = state_next;
				seq_cnt = 0;
				counting = false;
			}
		} else {
			state = state_next;
		}
	}

	return IRQ_HANDLED;
}
