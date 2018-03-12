#!/usr/bin/env python
#
# Copyright 2014 Ettus Research LLC
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
## Address 0x00
## Divider control
## Write-only, default = 0x007D0000
########################################################################
int_n_mode              0x00[31]        0       frac_n, int_n
## Integer divider: 16-65535 in int-N mode, 19-4091 in frac-N mode.
int_16_bit              0x00[15:30]     0x007D
## Frac divider: 0-4095
frac_12_bit             0x00[3:14]      0
########################################################################
## Address 0x01
## Charge pump control
## Write-only, default = 0x2000FFF9
########################################################################
res1                    0x01[31]        0
cpl                     0x01[29:30]     1       disabled, enabled, res1, res2
cpt                     0x01[27:28]     0       normal, reserved, force_source, force_sink
## sets phase shift
phase_12_bit            0x01[15:26]     1
## VCO frac modulus
mod_12_bit              0x01[3:14]      0xFFF
########################################################################
## Address 0x02
## Misc. control
## Write-only, default = 0x00004042
########################################################################
lds                     0x02[31]        0       slow, fast
low_noise_and_spur      0x02[29:30]     3       low_noise, reserved, low_spur_1, low_spur_2
muxout                  0x02[26:28]     0x6     tri_state, high, low, rdiv, ndiv, ald, dld, sync, res8, res9, res10, res11, spi, res13, res14, res15
reference_doubler       0x02[25]        0       disabled, enabled
reference_divide_by_2   0x02[24]        0       disabled, enabled
## R divider value, 1-1023
r_counter_10_bit        0x02[14:23]     1
double_buffer           0x02[13]        0       disabled, enabled
<% current_setting_enums = ', '.join(map(lambda x: '_'.join(("%0.2fma"%(1.631/5.1 * (1.+x))).split('.')), range(0,16))) %>\
charge_pump_current     0x02[9:12]      7       ${current_setting_enums}
ldf                     0x02[8]         0       frac_n, int_n
ldp                     0x02[7]         0       10ns, 6ns
pd_polarity             0x02[6]         1       negative, positive
power_down              0x02[5]         0       normal, shutdown
cp_three_state          0x02[4]         0       disabled, enabled
counter_reset           0x02[3]         0       normal, reset
########################################################################
## Address 0x03
## VCO control
## Write-only, default = 0x0000000B
########################################################################
## VCO subband selection, used when VAS disabledd
vco                     0x03[26:31]     0
## VCO autoselect
shutdown_vas            0x03[25]        0       enabled, disabled
## should actually be called vas_temp ...
retune                  0x03[24]        1       disabled, enabled
res3                    0x3[19:23]      0
csm                     0x3[18]         0       disabled, enabled
mutedel                 0x3[17]         0       disabled, enabled
clock_div_mode          0x03[15:16]     0       clock_divider_off, fast_lock, phase, reserved
## clock divider, 1-4095
clock_divider_12_bit    0x03[3:14]      1
########################################################################
## Address 0x04
## RF output control
## Write-only, default = 0x6180B23C
########################################################################
res4                    0x04[29:31]     0x3
shutdown_ldo            0x04[28]        0       enabled, disabled
shutdown_div            0x04[27]        0       enabled, disabled
shutdown_ref            0x04[26]        0       enabled, disabled
## Band select MSBs
bs_msb                  0x04[24:25]     0
feedback_select         0x04[23]        1       divided, fundamental
rf_divider_select       0x04[20:22]     0       div1, div2, div4, div8, div16, div32, div64, div128
band_select_clock_div   0x04[12:19]     0
shutdown_vco            0x04[11]        0       enabled, disabled
mute_lock_detect        0x04[10]        0       enabled, disabled
aux_output_select       0x04[9]         1       divided, fundamental
aux_output_enable       0x04[8]         0       disabled, enabled
aux_output_power        0x04[6:7]       0       m4dBm, m1dBm, 2dBm, 5dBm
rf_output_enable        0x04[5]         1       disabled, enabled
output_power            0x04[3:4]       3       m4dBm, m1dBm, 2dBm, 5dBm
########################################################################
## Address 0x05
## Misc
## Write only, default = 0x18400005
########################################################################
res5_31                 0x05[31]        0
vas_dly                 0x05[29:30]     3       disabled, res0, res1, enabled
res5_26_28              0x05[26:28]     0
shutdown_pll            0x05[25]        0       enabled, disabled
f01                     0x05[24]        1       frac_n, auto
ld_pin_mode             0x05[22:23]     1       low, dld, ald, high
mux_sdo                 0x05[18]        0       normal, sdo
res5_7_17               0x05[7:17]      0
adc_start               0x05[6]         0       normal, start_conversion
adc_mode                0x05[2:0]       0       disabled, temp_sensor, res2, res3, tune_pin, res5, res6, res7
"""

########################################################################
# Template for methods in the body of the struct
########################################################################
BODY_TMPL="""\
enum addr_t{
    ADDR_R0 = 0,
    ADDR_R1 = 1,
    ADDR_R2 = 2,
    ADDR_R3 = 3,
    ADDR_R4 = 4,
    ADDR_R5 = 5
};

uint32_t get_reg(uint8_t addr){
    uint32_t reg = addr & 0x7;
    switch(addr){
    % for addr in range(5+1):
    case ${addr}:
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
        reg |= (uint32_t(${reg.get_name()}) & ${reg.get_mask()}) << ${reg.get_shift()};
        % endfor
        break;
    % endfor
    }
    return reg;
}
"""

if __name__ == '__main__':
    import common; common.generate(
        name='max2871_regs',
        regs_tmpl=REGS_TMPL,
        body_tmpl=BODY_TMPL,
        file=__file__,
    )

