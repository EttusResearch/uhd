/*
 * ad9516.c
 *
 *  Created on: 21.11.2016
 *      Author: dr_zlo
 */

#include "ad9516.h"
#include "spi.h"
#include <memory_map.h>

void
ad9516_write_reg(int regno, uint8_t value)
{
  uint32_t inst = WR | (regno & 0x0fff);
  uint32_t v = (inst << 8) | (value & 0xff);
  spi_transact(SPI_TXONLY, SPI_SS_AD9516, v, 24, SPI_PUSH_FALL);
}

int
ad9516_read_reg(int regno)
{
  uint32_t inst = RD | (regno & 0x0fff);
  uint32_t v = (inst << 8) | 0;
  uint32_t r = spi_transact(SPI_TXRX, SPI_SS_AD9516, v, 24,
			    SPI_PUSH_FALL | SPI_LATCH_FALL);
  return r & 0xff;
}

void
ad9516_clocks_enable_fpga(bool enable, int divisor)
{
  uint8_t enable_word, bypass_word;
  uint8_t div_word = 0x00;
  enable_word = enable ? 0x08 : 0x0A;
  bypass_word = (divisor == 1) ? 0x80 : 0x00;

  if(divisor != 1){
	  uint8_t N = 0;
	  uint8_t M = 0;

	  if(divisor%2 == 1){
		  M = (divisor-1)/2 -1;
	  	  N = (divisor-1)/2;
	  }
	  else M = N = (divisor)/2 -1;

	  div_word =(M & 0x0f) | ((N & 0x0f) << 4);
  }


  //ad9516_write_reg(REG_SPI_CONFIG, 0x24 | 0x18);  //Default Value 0x18
  //ad9516_write_reg(REG_SPI_CONFIG, 0x18);

  ad9516_write_reg(REG_IN_CLKs,0x01); // Bypass the VCO divider as source for distribution section

  //ad9516_write_reg(REG_OUT1, enable_word); // Output to ADC en/dis
  ad9516_write_reg(REG_OUT0, enable_word); // Output to FPGA en/dis
  //ad9516_write_reg(REG_OUT5, enable_word); // Output to DAC en/dis
  //ad9516_write_reg(REG_OUT7, 0x42); 	// Output to TX DB en/dis


  ad9516_write_reg(REG_DIVIDER0,div_word); // Set divisor
  //ad9516_write_reg(REG_DIVIDER1,div_word); // Set divisor
  //ad9516_write_reg(REG_DIVIDER2,div_word); // Set divisor

  ad9516_write_reg(REG_DIVIDER0 + 1,bypass_word); // Divider 1 bypass

/*
  ad9516_write_reg(0x19C, 0x30);  // Bypass 3.1 & 3.2 divider's
 // ad9516_write_reg(0x19C, 0x20);  // Bypass 3.2 divider
 // ad9516_write_reg(0x199, 0x00);  // Set 3.1 divider to 2


  //ad9516_write_reg(0x00F1, 0x00 | enable_word);  //Sets the LVPECL output differential voltage


  ad9516_write_reg(0x010, 0x7C);	//PLL normal operation (PLL on)
  ad9516_write_reg(0x017, 0xB4);	//STATUS - Digital lock detect (DLD); active high.

  ad9516_write_reg(0x011, 0x02);	//R divider LSBs—lower eight bits

  ad9516_write_reg(0x016, 0x01);	//Prescaler P Divide-by-2
  ad9516_write_reg(0x014, 0x0A);	//13-bit B counter,Bits[7:0] (LSB)
  ad9516_write_reg(0x01C, 0x02);	//REF1 power-on
*/


  ad9516_write_reg(REG_UPD_ALL_REGs, 0x01);  // Update Regs
}

void
ad9516_clocks_enable_phy(bool enable, int divisor)
{
  uint8_t enable_word, bypass_word;
  uint8_t div_word = 0x00;
  enable_word = enable ? 0x4A : 0x4B;
  bypass_word = (divisor == 1) ? 0x30 : 0x20;

  if(divisor != 1){
	  uint8_t N = 0;
	  uint8_t M = 0;

	  if(divisor%2 == 1){
		  M = (divisor-1)/2 -1;
	  	  N = (divisor-1)/2;
	  }
	  else M = N = (divisor)/2 -1;

	  div_word =(M & 0x0f) | ((N & 0x0f) << 4);
  }

  ad9516_write_reg(REG_IN_CLKs,0x01); // Bypass the VCO divider as source for distribution section

  ad9516_write_reg(REG_OUT9, enable_word); // Output to PHY en/dis

  ad9516_write_reg(REG_DIVIDER4,div_word); // Set divisor

  ad9516_write_reg(REG_DIVIDER4 + 3,bypass_word); // Divider 1 bypass

  ad9516_write_reg(REG_UPD_ALL_REGs, 0x01);  // Update Regs
}

void
ad9516_clocks_enable(bool enable)
{
  int divisor = 4;

  uint8_t enable_word, bypass_word;
  uint8_t div_word = 0x00;
  enable_word = enable ? 0x4A : 0x4B;
  bypass_word = (divisor == 1) ? 0x30 : 0x20;

  if(divisor != 1){
	  uint8_t N = 0;
	  uint8_t M = 0;

	  if(divisor%2 == 1){
		  M = (divisor-1)/2 -1;
	  	  N = (divisor-1)/2;
	  }
	  else M = N = (divisor)/2 -1;

	  div_word =(M & 0x0f) | ((N & 0x0f) << 4);
  }

  ad9516_write_reg(REG_IN_CLKs,0x01); // Bypass the VCO divider as source for distribution section

  ad9516_write_reg(REG_OUT9, enable_word); // Output to PHY en/dis

  ad9516_write_reg(REG_DIVIDER4,div_word); // Set divisor
  ad9516_write_reg(REG_DIVIDER4 + 3,bypass_word); // Divider 4.2 bypass

  divisor = 1;

  enable_word = enable ? 0x08 : 0x0A;
  bypass_word = (divisor == 1) ? 0x80 : 0x00;

  if(divisor != 1){
	  uint8_t N = 0;
	  uint8_t M = 0;

	  if(divisor%2 == 1){
		  M = (divisor-1)/2 -1;
	  	  N = (divisor-1)/2;
	  }
	  else M = N = (divisor)/2 -1;

	  div_word =(M & 0x0f) | ((N & 0x0f) << 4);
  }

  //ad9516_write_reg(REG_OUT1, enable_word); // Output to ADC en/dis
  ad9516_write_reg(REG_OUT0, enable_word); // Output to FPGA en/dis
  //ad9516_write_reg(REG_OUT5, enable_word); // Output to DAC en/dis
  //ad9516_write_reg(REG_OUT7, 0x42); 	// Output to TX DB en/dis

  //ad9516_write_reg(REG_DIVIDER0,div_word); // Set divisor
  ad9516_write_reg(REG_DIVIDER0,div_word); // Set divisor
  //ad9516_write_reg(REG_DIVIDER2,div_word); // Set divisor

  ad9516_write_reg(REG_DIVIDER0 + 1,bypass_word); // Divider 1 bypass


  ad9516_write_reg(REG_UPD_ALL_REGs, 0x01);  // Update Regs
}


