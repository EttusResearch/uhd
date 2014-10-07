/*
 * bq24190.h
 *
 * Created: 11/12/2012 4:58:23 PM
 *  Author: Balint Seeber
 */ 


#ifndef BQ24190_H_
#define BQ24190_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef CHARGER_TI

bool bq24190_init(bool disable_charger);
bool bq24190_has_interrupt(void);
bool bq24190_handle_irq(void);
//void bq24190_dump(void);
//bool bq24190_set_charge_current_limit(uint8_t deciamps);
bool bq24190_toggle_charger(bool on);

#endif // !CHARGER_TI

#endif /* BQ24190_H_ */
