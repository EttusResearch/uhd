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
    serdes(0),
    exp(0),
    tx_db(0),
    rx_db(0) {}

  usrp2_clk_regs_t(usrp2_iface::rev_type rev) {
    fpga = adc = serdes = exp = tx_db = 0;
    test = 0;
    fpga = 1;
    dac = 3;

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
    }

    rx_db = 7;
  }

  static int output(int clknum) { return 0x3C + clknum; }
  static int div_lo(int clknum) { return 0x48 + 2 * clknum; }
  static int div_hi(int clknum) { return 0x49 + 2 * clknum; }

  const static int acounter = 0x04;
  const static int bcounter_msb = 0x05;
  const static int bcounter_lsb = 0x06;
  const static int pll_1 = 0x07;
  const static int pll_2 = 0x08;
  const static int pll_3 = 0x09;
  const static int pll_4 = 0x0A;
  const static int ref_counter_msb = 0x0B;
  const static int ref_counter_lsb = 0x0C;
  const static int pll_5 = 0x0D;
  const static int update = 0x5A;

  int test;
  int fpga;
  int adc;
  int dac;
  int serdes;
  int exp;
  int tx_db;
  int rx_db;
};

#endif //INCLUDED_USRP2_CLK_REGS_HPP
