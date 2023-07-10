/*
 * adc_ltc2184.h
 *
 *  Created on: 23.11.2016
 *      Author: dr_zlo
 */

#ifndef LTC2185_H_
#define LTC2185_H_

#include <stdint.h>


void ltc2185_init(void);
void ltc2185_write_reg(uint8_t regno, uint8_t value);
int  ltc2185_read_reg(uint8_t regno);

#endif /* LTC2185_H_ */
