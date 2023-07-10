//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_KINTEX7SDR_CLK_REGS_HPP
#define INCLUDED_KINTEX7SDR_CLK_REGS_HPP

#include "kintex7sdr_iface.hpp"

class kintex7sdr_clk_regs_t {
public:
    kintex7sdr_clk_regs_t(void) :
        acounter(0), bcounter_msb(0), bcounter_lsb(0), pll_1(0), pll_2(0), pll_3(0), pll_4(0), ref_counter_msb(0),
        ref_counter_lsb(0), pll_5(0), update(0), pll_6(0), pll_7(0), pll_8(0), pll_9(0), pll_PFD(0), pll_rdbck(0),
        test(0), fpga(0), adc(0), dac(0), dac_ref(0), serdes(0), exp(0), tx_db(0), rx_db(0),
        revision(kintex7sdr_iface::USRP_NXXX){}

    kintex7sdr_clk_regs_t(kintex7sdr_iface::rev_type rev) {
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

        switch (rev) {
            case kintex7sdr_iface::USRP_NXXX:
                // dont throw, it may be unitialized
                break;
            case kintex7sdr_iface::USRP_N210_XK: // It's my device on Kintex-7 board
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
            case kintex7sdr_iface::USRP_N210_XA:
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
            default:
                throw uhd::not_implemented_error("kintex7sdr_clk_regs_t: unknown hardware version");
        }
    }

    int output(int clknum) {
        switch (revision) {
            case kintex7sdr_iface::USRP_N210_XK:
            case kintex7sdr_iface::USRP_N210_XA:
                return clknum < 6 ? 0x0F0 + clknum : 0x140 + clknum - 6;
                break;
            default:
                throw uhd::not_implemented_error("output: unknown hardware version");
        }
    }

    int div_lo(int clknum) {
        switch (revision) {
            case kintex7sdr_iface::USRP_N210_XK:
            case kintex7sdr_iface::USRP_N210_XA:
                return clknum < 6 ? 0x190 + 3 * (int) (clknum / 2)
                                  : 0x199 + 5 * (int) ((clknum - 6) / 2);
            default:
                throw uhd::not_implemented_error("div_lo: unknown hardware version");
        }
    }

    int div_hi(int clknum) {
        switch (revision) {
            case kintex7sdr_iface::USRP_N210_XK:
            case kintex7sdr_iface::USRP_N210_XA:
                return clknum < 6 ? 0x191 + 3 * (int) (clknum / 2)
                                  : 0x19A + 5 * (int) ((clknum - 6) / 2);
            default:
                throw uhd::not_implemented_error("div_lo: unknown hardware version");
        }
    }

    int acounter = 0x04;
    int bcounter_msb = 0x05;
    int bcounter_lsb = 0x06;
    int pll_1 = 0x07;
    int pll_2 = 0x08;
    int pll_3 = 0x09;
    int pll_4 = 0x0A;
    int ref_counter_msb = 0x0B;
    int ref_counter_lsb = 0x0C;
    int pll_5 = 0x0D;
    int update = 0x5A;

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
    int dac_ref; // It's for my device on Kintex-7 board
    int serdes;
    int exp;
    int tx_db;
    int rx_db;

private:
    kintex7sdr_iface::rev_type revision;
};

#endif // INCLUDED_KINTEX7SDR_CLK_REGS_HPP
