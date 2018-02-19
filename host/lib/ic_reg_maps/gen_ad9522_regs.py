#!/usr/bin/env python
#
# Copyright 2010-2011,2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# Template for raw text data describing registers
# name addr[bit range inclusive] default optional enums
########################################################################
REGS_TMPL="""\
sdo_active                  0x000[7]                 0           sdio, sdo_sdio
lsb_first_addr_incr         0x000[6]                 0           msb, lsb
soft_reset                  0x000[5]                 0
mirror                      0x000[3:0]               0
readback_active_registers   0x004[0]                 0           buffer, active
pfd_polarity                0x010[7]                 0           pos, neg
cp_current                  0x010[6:4]               7           0_6ma, 1_2ma, 1_8ma, 2_4ma, 3_0ma, 3_6ma, 4_2ma, 4_8ma
cp_mode                     0x010[3:2]               3           high_imp, force_source, force_sink, normal
pll_power_down              0x010[1:0]               1           normal=0, async=1, sync=3
r_counter_lsb               0x011[7:0]               1
r_counter_msb               0x012[5:0]               0
~r_counter                  r_counter_lsb, r_counter_msb
a_counter                   0x013[5:0]               0
b_counter_lsb               0x014[7:0]               3
b_counter_msb               0x015[4:0]               0
~b_counter                  b_counter_lsb, b_counter_msb
set_cp_pin_to_vcp_2         0x016[7]                 0           normal, vcp_2
reset_r_counter             0x016[6]                 0
reset_a_and_b_counters      0x016[5]                 0
reset_all_counters          0x016[4]                 0
b_counter_bypass            0x016[3]                 0           normal, div1
prescaler_p                 0x016[2:0]               6           div1, div2, div2_3, div4_5, div8_9, div16_17, div32_33, div3
status_pin_control          0x017[7:2]               0
antibacklash_pulse_width    0x017[1:0]               0           2_9ns, 1_3ns, 6_0ns
enb_cmos_ref_input_dc_off   0x018[7]                 0
lock_detect_counter         0x018[6:5]               0           5cyc, 16cyc, 64cyc, 255cyc
digital_lock_detect_window  0x018[4]                 0           high_range, low_range
disable_digital_lock_detect 0x018[3]                 0           normal, disabled
vco_calibration_divider     0x018[2:1]               3           div2, div4, div8, div16
vco_calibration_now         0x018[0]                 0
r_a_b_counters_sync_pin_rst 0x019[7:6]               0           nothing, async, sync
r_path_delay                0x019[5:3]               0
n_path_delay                0x019[2:0]               0
enable_status_pin_divider   0x01A[7]                 0
ref_freq_monitor_threshold  0x01A[6]                 0           1_02mhz, 6khz
ld_pin_control              0x01A[5:0]               0
enable_vco_freq_monitor     0x01B[7]                 0
enable_ref2_freq_monitor    0x01B[6]                 0
enable_ref1_freq_monitor    0x01B[5]                 0
refmon_pin_control          0x01B[4:0]               0
disable_switchover_deglitch 0x01C[7]                 0
select_ref                  0x01C[6]                 0           ref1, ref2
use_ref_sel_pin             0x01C[5]                 0           register, ref_sel
enb_auto_ref_switchover     0x01C[4]                 0           manual, auto
stay_on_ref2                0x01C[3]                 0           return_ref1, stay_ref2
enable_ref2                 0x01C[2]                 0
enable_ref1                 0x01C[1]                 0
enable_differential_ref     0x01C[0]                 0
enb_stat_eeprom_at_stat_pin 0x01D[7]                 1
enable_xtal_osc             0x01D[6]                 0
enable_clock_doubler        0x01D[5]                 0
disable_pll_status_reg      0x01D[4]                 0
enable_ld_pin_comparator    0x01D[3]                 0
enable_external_holdover    0x01D[1]                 0
enable_holdover             0x01D[0]                 0
external_zero_delay_fcds    0x01E[4:3]               0
enable_external_zero_delay  0x01E[2]                 0
enable_zero_delay           0x01E[1]                 0
########################################################################
vco_calibration_finished    0x01F[6]                 0
holdover_active             0x01F[5]                 0
ref2_selected               0x01F[4]                 0
vco_freq_gt_thresh          0x01F[3]                 0
ref2_freq_gt_thresh         0x01F[2]                 0
ref1_freq_gt_thresh         0x01F[1]                 0
digital_lock_detect         0x01F[0]                 0
########################################################################
% for i in range(12):
<% addr = (i + 0x0F0) %>\
out${i}_format              ${addr}[7]             0             lvds, cmos
out${i}_cmos_configuration  ${addr}[6:5]           3             off, a_on, b_on, ab_on
out${i}_polarity            ${addr}[4:3]           0             lvds_a_non_b_inv=0, lvds_a_inv_b_non=1, cmos_ab_non=0, cmos_ab_inv=1, cmos_a_non_b_inv=2, cmos_a_inv_b_non=3
out${i}_lvds_diff_voltage   ${addr}[2:1]           1             1_75ma, 3_5ma, 5_25ma, 7_0ma
out${i}_lvds_power_down     ${addr}[0]             0
% endfor
########################################################################
% for i in reversed(range(8)):
csdld_en_out_${i}           0x0FC[${i}]                0           ignore, async
% endfor
########################################################################
% for i in reversed(range(4)):
csdld_en_out_${8 + i}      0x0FD[${i}]                0           ignore, async
% endfor
########################################################################
% for i in range(4):
<% default_val = int(0x7 / (2**i)) %>\
<% addr0 = hex(i*3 + 0x190) %>\
<% addr1 = hex(i*3 + 0x191) %>\
<% addr2 = hex(i*3 + 0x192) %>\
divider${i}_low_cycles      ${addr0}[7:4]         ${default_val}
divider${i}_high_cycles     ${addr0}[3:0]         ${default_val}
divider${i}_bypass          ${addr1}[7]           0
divider${i}_ignore_sync     ${addr1}[6]           0
divider${i}_force_high      ${addr1}[5]           0
divider${i}_start_high      ${addr1}[4]           0
divider${i}_phase_offset    ${addr1}[3:0]         0
channel${i}_power_down      ${addr2}[2]           0
disable_divider${i}_ddc     ${addr2}[0]           0
% endfor
########################################################################
vco_divider                  0x1E0[2:0]              2             div2, div3, div4, div5, div6, static, div1
power_down_clock_input_sel   0x1E1[4]                0
power_down_vco_clock_ifc     0x1E1[3]                0
power_down_vco_and_clock     0x1E1[2]                0
select_vco_or_clock          0x1E1[1]                0             external, vco
bypass_vco_divider           0x1E1[0]                0
disable_power_on_sync        0x230[3]                0
power_down_sync              0x230[2]                0
power_down_dist_ref          0x230[1]                0
soft_sync                    0x230[0]                0
io_update                    0x232[0]                0
soft_eeprom                  0xB02[1]                0
enable_eeprom_write          0xB02[0]                0
reg2eeprom                   0xB03[0]                0
"""

########################################################################
# Template for methods in the body of the struct
########################################################################
BODY_TMPL="""\
uint32_t get_reg(uint16_t addr){
    uint32_t reg = 0;
    switch(addr){
    % for addr in sorted(set(map(lambda r: r.get_addr(), regs))):
    case ${addr}:
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
        reg |= (uint8_t(${reg.get_name()}) & ${reg.get_mask()}) << ${reg.get_shift()};
        % endfor
        break;
    % endfor
    }
    if (addr == 0){ //mirror 4 bits in register 0
        reg |= ((reg >> 7) & 0x1) << 0;
        reg |= ((reg >> 6) & 0x1) << 1;
        reg |= ((reg >> 5) & 0x1) << 2;
        reg |= ((reg >> 4) & 0x1) << 3;
    }
    return reg;
}

void set_reg(uint16_t addr, uint32_t reg){
    switch(addr){
    % for addr in sorted(set(map(lambda r: r.get_addr(), regs))):
    case ${addr}:
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
        ${reg.get_name()} = ${reg.get_type()}((reg >> ${reg.get_shift()}) & ${reg.get_mask()});
        % endfor
        break;
    % endfor
    }
}

uint32_t get_write_reg(uint16_t addr){
    return (uint32_t(addr) << 8) | get_reg(addr);
}

uint32_t get_read_reg(uint16_t addr){
    return (uint32_t(addr) << 8) | (1 << 23);
}

"""

if __name__ == '__main__':
    import common; common.generate(
        name='ad9522_regs',
        regs_tmpl=REGS_TMPL,
        body_tmpl=BODY_TMPL,
        file=__file__,
    )
