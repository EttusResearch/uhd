//
// Copyright 2010 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_USRP2_CLK_REGS_HPP
#define INCLUDED_USRP2_CLK_REGS_HPP

#include "usrp2_iface.hpp"

class usrp2_clk_regs_t {
public:
  usrp2_clk_regs_t(void):

	test(0),
    fpga(0),
    adc(0),
    dac(0),

	dac_ref(0),

    serdes(0),
    exp(0),
    tx_db(0),
    rx_db(0),

	acounter(0),
	bcounter_msb(0),
	bcounter_lsb(0),
	pll_1(0),
	pll_2(0),
	pll_3(0),
	pll_4(0),
	ref_counter_msb(0),
	ref_counter_lsb(0),
	pll_5(0),
	update(0),

	pll_6(0),
	pll_7(0),
	pll_8(0),
	pll_9(0),
	pll_PFD(0),
	pll_rdbck(0),

	revision(usrp2_iface::USRP_NXXX) {}

  usrp2_clk_regs_t(usrp2_iface::rev_type rev) {
	revision = rev;

    fpga = adc = serdes = exp = tx_db = 0;
    test = 0;
    fpga = 1;
    dac = 3;
    rx_db = 7;

    acounter = 0x04;
    bcounter_msb = 0x05;
    bcounter_lsb = 0x06;
    pll_1 = 0x07;
    pll_2 = 0x08;
    pll_3 = 0x09;
    pll_4 = 0x0A;
    ref_counter_msb = 0x0B;
    ref_counter_lsb = 0x0C;
    pll_5 = 0x0D;
    update = 0x5A;

    pll_6 = 0;
    pll_7 = 0;
    pll_8 = 0;
    pll_9 = 0;
    pll_PFD = 0;
    pll_rdbck = 0;

    switch(rev) {
    case usrp2_iface::USRP2_REV3:
        exp = 2;
        adc = 4;
        serdes = 2;
        tx_db = 6;
        break;
    case usrp2_iface::USRP2_REV4:
        exp = 5;
        adc = 4;
        serdes = 2;
        tx_db = 6;
        break;
    case usrp2_iface::USRP_N200:
    case usrp2_iface::USRP_N210:
    case usrp2_iface::USRP_N200_R4:
    case usrp2_iface::USRP_N210_R4:
        exp = 6;
        adc = 2;
        serdes = 4;
        tx_db = 5;
        break;
    case usrp2_iface::USRP_NXXX:
        //dont throw, it may be unitialized
        break;

    case usrp2_iface::USRP_N210_XK:				//It's my device on Kintex-7 board
    	fpga = 0;
    	adc = 1;
    	dac = 2;
    	dac_ref = 3;
    	rx_db = 6;
    	tx_db = 7;

    	acounter = 0x013;
    	bcounter_msb = 0x015;
    	bcounter_lsb = 0x014;
    	pll_1 = 0x016;
    	pll_2 = 0x017;
    	pll_3 = 0x018;
    	pll_4 = 0x019;
    	ref_counter_msb = 0x012;
    	ref_counter_lsb = 0x011;
    	pll_5 = 0x01A;
    	update = 0x232;

    	pll_6 = 0x01B;
    	pll_7 = 0x01C;
    	pll_8 = 0x01D;
    	pll_9 = 0x01E;
    	pll_PFD = 0x010;
    	pll_rdbck = 0x01F;
    break;
    case usrp2_iface::USRP_N210_XA:
    	fpga = 3;
    	adc = 1;
    	dac = 0;
    	rx_db = 6;
    	tx_db = 7;
    	test = 5;

    	acounter = 0x013;
    	bcounter_msb = 0x015;
    	bcounter_lsb = 0x014;
    	pll_1 = 0x016;
    	pll_2 = 0x017;
    	pll_3 = 0x018;
    	pll_4 = 0x019;
    	ref_counter_msb = 0x012;
    	ref_counter_lsb = 0x011;
    	pll_5 = 0x01A;
    	update = 0x232;

    	pll_6 = 0x01B;
    	pll_7 = 0x01C;
    	pll_8 = 0x01D;
    	pll_9 = 0x01E;
    	pll_PFD = 0x010;
    	pll_rdbck = 0x01F;
    	break;
    }
  }

  int output(int clknum) {
	  switch(revision){
	  case usrp2_iface::USRP_N210_XK:
	  case usrp2_iface::USRP_N210_XA:
		  return clknum < 6 ? 0x0F0 + clknum : 0x140 + clknum - 6;
		  break;
	  default:
		  return 0x3C + clknum;
	  }
  }

  int div_lo(int clknum) {
	  switch(revision){
	  case usrp2_iface::USRP_N210_XK:
	  case usrp2_iface::USRP_N210_XA:
		  return clknum < 6 ? 0x190 + 3*(int)(clknum/2): 0x199 + 5*(int)((clknum - 6)/2);
		  break;
	  default:
		  return 0x48 + 2 * clknum;
	  }
  }

  int div_hi(int clknum) {
	  switch(revision){
	  case usrp2_iface::USRP_N210_XK:
	  case usrp2_iface::USRP_N210_XA:
		  return clknum < 6 ? 0x191 + 3*(int)(clknum/2): 0x19A + 5*(int)((clknum - 6)/2);
		  break;
	  default:
		  return 0x49 + 2 * clknum;
	  }
  }

  int acounter;
  int bcounter_msb;
  int bcounter_lsb;
  int pll_1;
  int pll_2;
  int pll_3;
  int pll_4;
  int ref_counter_msb;
  int ref_counter_lsb;
  int pll_5;
  int update;

  int pll_6;
  int pll_7;
  int pll_8;
  int pll_9;
  int pll_PFD;
  int pll_rdbck;

  int test;
  int fpga;
  int adc;
  int dac;

  int dac_ref;			//It's for my device on Kintex-7 board

  int serdes;
  int exp;
  int tx_db;
  int rx_db;

private:
  usrp2_iface::rev_type revision;
};

#endif //INCLUDED_USRP2_CLK_REGS_HPP
