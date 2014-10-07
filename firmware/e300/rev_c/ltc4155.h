/*
 * ltc4155.h
 *
 * Created: 17/08/2012 8:09:43 PM
 *  Author: Balint Seeber
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
