#!/usr/bin/env python3
#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Register map for the Rhodium CPLD. This is controlled via SPI.
"""

########################################################################
# Template for raw text data describing registers
# name addr[bit range inclusive] default optional enums
########################################################################
REGS_TMPL = """\
########################################################################
## address 5 Scratch
########################################################################
scratch                 5[0:15]     0x0
########################################################################
## address 6 Rx Band Select
########################################################################
reg6_reserved0          6[0]        0x0
rx_sw1                  6[1:2]      1       cal_loopback, rx2, isolation, txrx
rx_sw2_sw7              6[3]        0       lowband, highband
rx_sw3                  6[4:5]      0       rx_sw4, filter7, filter6, filter5
rx_sw4_sw5              6[6:9]      1       filter2=1, filter1=2, filter4=4, filter3=8
rx_sw6                  6[10:11]    3       filter5, filter6, filter7, rx_sw5
rx_gain_tbl_sel         6[12]       0       lowband, highband
reg6_reserved1          6[13:15]    0x0
########################################################################
## address 7 Tx Band Select
########################################################################
reg7_reserved0          7[0:1]      0x0
tx_sw1                  7[2:3]      3       lowband_if, tx_sw2, cal_loopback, isolation
tx_sw2                  7[4:5]      0       tx_sw3, filter7, filter6, filter5
tx_sw3_sw4              7[6:9]      1       filter2=1, filter1=2, filter4=4, filter3=8
tx_sw5                  7[10:11]    0       filter5, filter6, filter7, tx_sw4
tx_gain_tbl_sel         7[12]       0       lowband, highband
reg7_reserved1          7[13:15]    0x0
########################################################################
## address 8 Misc Switches
########################################################################
reg8_reserved0          8[0:2]      0x0
cal_iso_sw              8[3]        0       isolation, cal_loopback
tx_hb_lb_sel            8[4]        0       lowband, highband
reg8_reserved1          8[5]        0
tx_lo_input_sel         8[6]        0       internal, external
rx_hb_lb_sel            8[7]        0       lowband, highband
reg8_reserved2          8[8]        0
rx_lo_input_sel         8[9]        1       external, internal
rx_demod_adj            8[10:11]    0       res_open=0, res_200_ohm=1, res_1500_ohm=2
tx_lo_filter_sel        8[12:13]    3       0_9ghz_lpf, 5_85ghz_lpf, 2_25ghz_lpf, isolation
rx_lo_filter_sel        8[14:15]    3       0_9ghz_lpf, 5_85ghz_lpf, 2_25ghz_lpf, isolation
"""

########################################################################
# Template for methods in the body of the struct
########################################################################
BODY_TMPL = """\
uint32_t get_reg(uint8_t addr){
    uint32_t reg = 0;
    switch(addr){
    % for addr in range(5, 14+1):
    case ${addr}:
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
        reg |= (uint32_t(${reg.get_name()}) & ${reg.get_mask()}) << ${reg.get_shift()};
        % endfor
        break;
    % endfor
    }
    return reg;
}

std::set<size_t> get_all_addrs()
{
    std::set<size_t> addrs;
    % for reg in regs:
    // Hopefully, compilers will optimize out this mess...
    addrs.insert(${reg.get_addr()});
    % endfor
    return addrs;
}
"""

if __name__ == "__main__":
    import common

    common.generate(
        name="rhodium_cpld_regs",
        regs_tmpl=REGS_TMPL,
        body_tmpl=BODY_TMPL,
        file=__file__,
    )
