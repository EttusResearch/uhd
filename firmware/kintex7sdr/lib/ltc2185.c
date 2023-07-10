/*
 * adc_ltc2184.c
 *
 *  Created on: 23.11.2016
 *      Author: dr_zlo
 */
#include "ltc2185.h"
#include "spi.h"
#include <memory_map.h>

//printf headers
#include "nonstdio.h"

#define RD (1 << 7)
#define WR (0 << 7)

void
ltc2185_write_reg(uint8_t regno, uint8_t value)
{
  uint32_t inst = WR | (regno & 0x0f);
  uint32_t v = (inst << 8) | (value & 0xff);
  spi_transact(SPI_TXONLY, SPI_SS_LTC2184, v, 16, SPI_PUSH_FALL);
}

int
ltc2185_read_reg(uint8_t regno)
{
  uint32_t inst = RD | (regno & 0x0f);
  uint32_t v = (inst << 8) |  0;
  uint32_t r = spi_transact(SPI_TXRX, SPI_SS_LTC2184, v, 16,
		  SPI_PUSH_FALL | SPI_LATCH_FALL);
  return r & 0xff;
}

void
ltc2185_init(void)
{
  ltc2185_write_reg(0x00, 0x80); // SOFT RESET LTC2184
  ltc2185_write_reg(0x04, 0x01); //0 = Offset Binary Data Format
  	  	  	  	  	  	  	  	 //1 = Two’s Complement Data Format
  ltc2185_write_reg(0x03, 0x09); //DDR LVDS mode, termination ON
  ltc2185_write_reg(0x02, 0x0d);
  ltc2185_write_reg(0x01, 0x00);


  //for(uint8_t i = 1; i< 5;i++)
  //printf("adress %x is %x\n", i, ltc2185_read_reg(i));
}
