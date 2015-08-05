/*
 * ltc4155.h
 *
 * Copyright 2015 National Instruments Corp
 */ 


#ifndef LTC4155_H_
#define LTC4155_H_

#include <stdbool.h>
#include <stdint.h>

#ifndef CHARGER_TI

bool ltc4155_init(bool disable_charger);
bool ltc4155_has_interrupt(void);
bool ltc4155_handle_irq(void);
void ltc4155_dump(void);
bool ltc4155_set_charge_current_limit(uint8_t percentage);

#endif // !CHARGER_TI

#endif /* LTC4155_H_ */
