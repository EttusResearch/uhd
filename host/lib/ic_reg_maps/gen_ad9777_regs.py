#!/usr/bin/env python
#
# Copyright 2010 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# Template for raw text data describing registers
# name addr[bit range inclusive] default optional enums
########################################################################
REGS_TMPL="""\
########################################################################
## address 0
########################################################################
sdio_bidirectional      0[7]     0      input, io
lsb_msb_first           0[6]     0      msb, lsb
soft_reset              0[5]     0
sleep_mode              0[4]     0
power_down_mode         0[3]     0
x_1r_2r_mode            0[2]     0      2r, 1r
pll_lock_indicator      0[1]     0
########################################################################
## address 1
########################################################################
filter_interp_rate      1[6:7]   0      1x, 2x, 4x, 8x
modulation_mode         1[4:5]   0      none, fs_2, fs_4, fs_8
zero_stuff_mode         1[3]     0
mix_mode                1[2]     1      complex, real
modulation_form         1[1]     0      e_minus_jwt, e_plus_jwt
data_clk_pll_lock_sel   1[0]     0      pll_lock, data_clk
########################################################################
## address 2
########################################################################
signed_input_data       2[7]     0      signed, unsigned
two_port_mode           2[6]     0      two_port, one_port
dataclk_driver_strength 2[5]     0      weak, strong
dataclk_invert          2[4]     0
oneportclk_invert       2[2]     0
iqsel_invert            2[1]     0
iq_first                2[0]     0      i_first, q_first
########################################################################
## address 3
########################################################################
data_rate_clock_output  3[7]     0      pll_lock, spi_sdo
pll_divide_ratio        3[0:1]   0      div1, div2, div4, div8
########################################################################
## address 4
########################################################################
pll_state               4[7]     0      off, on
auto_cp_control         4[6]     0      auto, manual
pll_cp_control          4[0:2]   0      50ua=0, 100ua=1, 200ua=2, 400ua=3, 800ua=7
########################################################################
## address 5 and 9
########################################################################
idac_fine_gain_adjust   5[0:7]   0
qdac_fine_gain_adjust   9[0:7]   0
########################################################################
## address 6 and A
########################################################################
idac_coarse_gain_adjust 6[0:3]   0
qdac_coarse_gain_adjust 0xA[0:3] 0
########################################################################
## address 7, 8 and B, C
########################################################################
idac_offset_adjust_msb  7[0:7]   0
idac_offset_adjust_lsb  8[0:1]   0
~idac_offset_adjust     idac_offset_adjust_lsb, idac_offset_adjust_msb
idac_ioffset_direction  8[7]     0     out_a, out_b
qdac_offset_adjust_msb  0xB[0:7] 0
qdac_offset_adjust_lsb  0xC[0:1] 0
~qdac_offset_adjust     qdac_offset_adjust_lsb, qdac_offset_adjust_msb
qdac_ioffset_direction  0xC[7]   0     out_a, out_b
"""

########################################################################
# Template for methods in the body of the struct
########################################################################
BODY_TMPL="""\
uint8_t get_reg(uint8_t addr){
    uint8_t reg = 0;
    switch(addr){
    % for addr in sorted(set(map(lambda r: r.get_addr(), regs))):
    case ${addr}:
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
        reg |= (uint8_t(${reg.get_name()}) & ${reg.get_mask()}) << ${reg.get_shift()};
        % endfor
        break;
    % endfor
    }
    return reg;
}

uint16_t get_write_reg(uint8_t addr){
    return (uint16_t(addr) << 8) | get_reg(addr);
}

uint16_t get_read_reg(uint8_t addr){
    return (uint16_t(addr) << 8) | (1 << 7);
}
"""

if __name__ == '__main__':
    import common; common.generate(
        name='ad9777_regs',
        regs_tmpl=REGS_TMPL,
        body_tmpl=BODY_TMPL,
        file=__file__,
    )
