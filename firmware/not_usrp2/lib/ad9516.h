/*
 * ad9516.h
 *
 *  Created on: 21.11.2016
 *      Author: dr_zlo
 */

#ifndef AD9516_H_
#define AD9516_H_

#include <stdint.h>
#include <stdbool.h>


#define RD (1 << 15)
#define WR (0 << 15)


#define REG_SPI_CONFIG	 	0x0000
#define REG_PFD_AND_CP	 	0x0010
#define REG_R_CNT		 	0x0011
#define REG_A_CNT		 	0x0013
#define REG_B_CNT		 	0x0014
#define REG_PLL_CTRL1	 	0x0016
#define REG_PLL_CTRL2	 	0x0017
#define REG_PLL_CTRL3	 	0x0018
#define REG_PLL_CTRL4	 	0x0019
#define REG_PLL_CTRL5	 	0x001a
#define REG_PLL_CTRL6	 	0x001b
#define REG_PLL_CTRL7	 	0x001c
#define REG_PLL_CTRL8	 	0x001d
#define REG_PLL_CTRL9	 	0x001e
#define REG_PLL_READBACK	0x001f
#define REG_OUT6_DLY_BPS	0x00a0
#define REG_OUT6_DLY_FS		0x00a1
#define REG_OUT6_DLY_FRN	0x00a2
#define REG_OUT7_DLY_BPS	0x00a3
#define REG_OUT7_DLY_FS		0x00a4
#define REG_OUT7_DLY_FRN	0x00a5
#define REG_OUT8_DLY_BPS	0x00a6
#define REG_OUT8_DLY_FS		0x00a7
#define REG_OUT8_DLY_FRN	0x00a8
#define REG_OUT9_DLY_BPS	0x00a9
#define REG_OUT9_DLY_FS		0x00aa
#define REG_OUT9_DLY_FRN	0x00ab
#define REG_OUT0			0x00f0
#define REG_OUT1			0x00f1
#define REG_OUT2			0x00f2
#define REG_OUT3			0x00f3
#define REG_OUT4			0x00f4
#define REG_OUT5			0x00f5
#define REG_OUT6			0x0140
#define REG_OUT7			0x0141
#define REG_OUT8			0x0142
#define REG_OUT9			0x0143
#define REG_DIVIDER0		0x0190
#define REG_DIVIDER1		0x0193
#define REG_DIVIDER2		0x0196
#define REG_DIVIDER3		0x0199
#define REG_DIVIDER4		0x019e
#define REG_VCO_DIVIDER		0x01e0
#define REG_IN_CLKs			0x01e1
#define REG_PD_AND_SYNC		0x0230
#define REG_UPD_ALL_REGs	0x0232

#define LVPECL_OUTn_ON		0x08
#define DIVIDERs_OFF		0x60

/*
 * Analog Device AD9516 1.6 GHz Clock Distribution IC w/ PLL
 */

void ad9516_write_reg(int regno, uint8_t value);

int  ad9516_read_reg(int regno);

void ad9516_clocks_enable_fpga(bool enable, int divisor);

void ad9516_clocks_enable_phy(bool enable, int divisor);

void ad9516_clocks_enable(bool enable);

#endif /* AD9516_H_ */
