# Copyright 2026 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""Register map for the HBX.

Note that the ground truth for this register map is in the
fpga/usrp3/top/x400/dboards/hbx subdirectory, and can be looked
up in fpga/usrp3/top/x400/doc/x420/X420_FPGA.htm
"""

########################################################################
# Template for raw text data describing registers
# name addr[bit range inclusive] default optional enums
########################################################################
REGS_TMPL = """\
########################################################################
## GLOBAL REGISTERS
########################################################################
## Register SLAVE_SIGNATURE
BOARD_ID                     0x0000[0:15]          ro
## Register SLAVE_REVISION
REVISION_REG                 0x0004[0:31]          ro
## Register SLAVE_OLDEST_REVISION
OLDEST_REVISION_REG          0x0008[0:31]          ro
## Register SLAVE_SCRATCH
SCRATCH_REG                  0x000C[0:31]          rw
## Register GIT_HASH_REGISTER
GIT_HASH                     0x0010[0:27]          ro
GIT_CLEAN                    0x0010[28:31]         ro
########################################################################
## Power Control
########################################################################
## Register RF_POWER_CONTROL
N1d7V_EN                     0x0020[0]             scope=mpm,rw
N2d5V_EN                     0x0020[1]             scope=mpm,rw
N3d2V_EN                     0x0020[2]             scope=mpm,rw
N3d3V_EN                     0x0020[3]             scope=mpm,rw
P2d5V_TX_BB_AMP_EN           0x0020[4]             scope=mpm,rw
P3d3V_RF_EN                  0x0020[5]             scope=mpm,rw
P3d3V_REF_CLK_EN             0x0020[6]             scope=mpm,rw
P3d3V_RX1_LO1_EN             0x0020[7]             scope=mpm,rw
P3d3V_TX1_LO1_EN             0x0020[8]             scope=mpm,rw
P3d3V_RX_BB_AMP2_EN          0x0020[9]             scope=mpm,rw
P3d3V_RX_BB_AMP1_EN          0x0020[10]            scope=mpm,rw
P3d3V_ADMV1420_EN            0x0020[11]            scope=mpm,rw
P3d3V_ADMV1320_EN            0x0020[12]            scope=mpm,rw
P4d0V_EN                     0x0020[13]            scope=mpm,rw
P5V_RX_RF_EN                 0x0020[14]            scope=mpm,rw
P5V_TX_RF_EN                 0x0020[15]            scope=mpm,rw
P5V_RX_LO_EN                 0x0020[16]            scope=mpm,rw
P5V_TX_LO_EN                 0x0020[17]            scope=mpm,rw
P5d6V_EN                     0x0020[18]            scope=mpm,rw
P8d0V_EN                     0x0020[19]            scope=mpm,rw
P8d7V_SMPS_EN                0x0020[20]            scope=mpm,rw
## Register RF_POWER_STATUS
N1d7V_PG                     0x0024[0]             scope=mpm,ro
N2d5V_PG                     0x0024[1]             scope=mpm,ro
N3d2V_PG                     0x0024[2]             scope=mpm,ro
N3d3V_PG_RX                  0x0024[3]             scope=mpm,ro
N3d3V_PG_TX                  0x0024[4]             scope=mpm,ro
P3d3V_ADMV1420_PG            0x0024[5]             scope=mpm,ro
P3d3V_ADMV1320_PG            0x0024[6]             scope=mpm,ro
P4d0V_PG                     0x0024[7]             scope=mpm,ro
P5d6V_PG                     0x0024[8]             scope=mpm,ro
P8d0V_PG                     0x0024[9]             scope=mpm,ro
P8d7V_SMPS_PG                0x0024[10]            scope=mpm,ro
## Register CPLD_INTERNAL_REG
PLL_REF_CLOCK_ENABLE         0x0028[0]             scope=mpm,rw
IO_ENABLE                    0x0028[1]             scope=mpm,rw
########################################
## TX LO SYNTH CONTROL
########################################
## Register LO_SPI_INFO
TX_LO_SPI_INFO               0x1000[0:15]          ro
########################################
## RX LO SYNTH CONTROL
########################################
## Register LO_SPI_INFO
RX_LO_SPI_INFO               0x1010[0:15]          ro
########################################
## TX ADMV CONTROL
########################################
## Register ADMV_SPI_INFO
TX_ADMV_SPI_INFO             0x1020[0:15]          ro
## Register ADMV_CONTROL
TX_ADMV_EN                   0x102C[0]             rw
TX_ADMV_RST                  0x102C[1]             rw
TX_ADMV_LOAD                 0x102C[2]             rw
########################################
## RX ADMV CONTROL
########################################
## Register ADMV_SPI_INFO
RX_ADMV_SPI_INFO             0x1030[0:15]          ro
## Register ADMV_CONTROL
RX_ADMV_EN                   0x103C[0]             rw
RX_ADMV_RST                  0x103C[1]             rw
RX_ADMV_LOAD                 0x103C[2]             rw
########################################
## IQ DEMOD CONTROL
########################################
## Register IQ_DEMOD_SPI_INFO
IQ_DEMOD_SPI_INFO            0x1040[0:15]          ro
########################################
## TX LO PD ADC CONTROL
########################################
## Register LO_PD_ADC_SPI_INFO
TX_LO_PD_ADC_SPI_INFO        0x1050[0:15]          ro
########################################
## RX LO PD ADC CONTROL
########################################
## Register LO_PD_ADC_SPI_INFO
RX_LO_PD_ADC_SPI_INFO        0x1060[0:15]          ro
########################################
## RX_RF_PD_REGMAP
########################################
## Register RX_RF_PD_THRESHOLD
RX_RF_PD_THRESHOLD           0x1070[0:15]          rw
## Register RX_RF_PD_CONTROL
RX_RF_PD_CONTROL_ENABLE      0x1074[0]             wo
RX_RF_PD_CONTROL_DISABLE     0x1074[1]             wo
RX_RF_PD_OVERLOAD_RESET      0x1074[2]             wo
## Register RX_RF_PD_STATUS
RX_RF_PD_ADC_VALUE           0x1078[0:15]          ro
RX_RF_PD_OVERLOAD            0x1078[30]            ro
RX_RF_PD_CONTROL_RUNNING     0x1078[31]            ro
########################################
## ATR CONTROLLER
########################################
## Register CURRENT_CONFIG_REG
CURRENT_RF0_CONFIG           0x10A0[0:7]           ro
## Register OPTION_REG
RF0_OPTION                   0x10A4[0]             rw  SW_DEFINED, CLASSIC_ATR
## Register SW_CONFIG_REG
SW_RF0_CONFIG                0x10A8[0:7]           rw
########################################
## RF_SWITCHES_REGMAP
########################################
## Register TX_SW_REG
TX_BB_SW1[256]               0x2000[0]             rw  band_1, band_2_3
TX_BB_SW2[256]               0x2000[1]             rw  band_2, band_3
TX_B1_SW1[256]               0x2000[4:6]           rw  filter_1, filter_4, filter_8, filter_2, filter_6, filter_7, filter_5, filter_3
TX_B1_SW2[256]               0x2000[8:10]          rw  filter_3, filter_5, filter_7, filter_6, filter_2, filter_8, filter_4, filter_1
TX_LF_SW1[256]               0x2000[12]            rw  band_0, band_1
TX_RF_SW_LF[256]             0x2000[13]            rw  band_0_1, band_2_3
TX_RF_SW_LOOP[256]           0x2000[14]            rw  loopback, antenna
TX_RF_SW_TRX[256]            0x2000[15]            rw  tx, rx
TX_LO_IN_SW[256]             0x2000[16]            rw  internal, external
TX_LO_SW1[256]               0x2000[20:21]         rw  band_1, filter_bank_2, filter_bank_1, power_detector
TX_LO_B23_SW1[256]           0x2000[24]            rw  filter_bank_2, filter_bank_1
TX_LO_B23_SW2[256]           0x2000[25]            rw  band_2, band_3
TX_LO_B1_FB[256]             0x2000[28:29]         rw  filter_4, filter_1, filter_2, filter_3
TX_LO_SBHM_FB[256]           0x2000[30:31]         rw  filter_3, filter_4, filter_5, filter_6
## Register RX_SW_REG
RX_BB_SW1[256]               0x2400[0]             rw  band_1, band_2
RX_B1_SW1[256]               0x2400[1]             rw  rf_path, lf_path
RX_B1_SW2[256]               0x2400[2]             rw  fb1_f1_to_f8, fb1_f9
RX_B1_SW3[256]               0x2400[4:6]           rw  filter_3, filter_8, filter_6, filter_7, filter_2, filter_5, filter_1, filter_4
RX_B1_SW4[256]               0x2400[7:9]           rw  filter_4, filter_1, filter_5, filter_2, filter_7, filter_6, filter_8, filter_3
RX_B1_SW5[256]               0x2400[10]            rw  fb1_f9, fb1_f1_to_f8
RX_DIRECT_SW1[256]           0x2400[11]            rw  rf, sync_inj
RX_LF_SW1[256]               0x2400[12]            rw  ltc, adc
RX_SW_RF_LF[256]             0x2400[13]            rw  band_0, admv
RX_SW_TRX[256]               0x2400[14:15]         rw  loopback, trx, rx, termination
RX_LO_IN_SW[256]             0x2400[16]            rw  internal, external
RX_LO_SW1[256]               0x2400[20:21]         rw  band_1, b12_pre_db_f2, b12_pre_db_f1, power_detector
RX_LO_B12_SW[256]            0x2400[24]            rw  lf_path, rf_path
RX_LO_B23_SW1[256]           0x2400[25]            rw  pre_db_f2, pre_db_f1
RX_LO_B23_SW2[256]           0x2400[26]            rw  band_2, band_1
RX_LO_B1_FB[256]             0x2400[28:29]         rw  filter_4, filter_1, filter_2, filter_3
RX_LO_SBHM_FB[256]           0x2400[30:31]         rw  filter_3, filter_4, filter_5, filter_6
########################################
## DSA CONTROL
########################################
## Register TX_RF_DSA_REG
TX_RF_DSA[256]               0x3000[0:5]           46
## Register RX_RF_DSA_REG
RX_RF_DSA[256]               0x3400[0:5]           46
RX_LF_DSA1[256]              0x3400[8:12]          31
RX_LF_DSA2[256]              0x3400[16:20]         31
## Register TX_LO_DSA_REG
TX_LO_DSA[256]               0x3800[0:4]           31
## Register RX_LO_DSA_REG
RX_LO_DSA[256]               0x3C00[0:4]           31
########################################
## LED CONTROL
########################################
## Register HBX_RF_LEDS
TXRX_LED_G[256]              0x4000[0]             rw
TXRX_LED[256]                0x4000[1]             rw
RX1_LED_G[256]               0x4000[4]             rw
TX_LO_LED_G[256]             0x4000[8]             rw
TX_LO_LED[256]               0x4000[9]             rw
RX_LO_LED_G[256]             0x4000[12]            rw
RX_LO_LED[256]               0x4000[13]            rw
## Register HBX_DEBUG_LEDS
CPLD_DEBUG_LED               0x4400[0:3]           rw
########################################
## Internal Sync Signal (implemented in FPGA)
########################################
SYNC_CLK                     0x7000[0]             scope=uhd,rw  disable, enable
"""

########################################################################
# Template for python methods in the body of the struct
########################################################################

PY_BODY_TMPL = """\
def get_reg(self, addr):
    "Return the value of register at address addr"
    reg = 0
    # First the regular registers
    % for addr in sorted(set([r.get_addr() for r in regs if not r.is_array])):
    if addr == ${addr}:
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
            % if reg.get_enums():
        reg |= (self.${reg.get_name()}.value & ${reg.get_mask()}) << ${reg.get_shift()}
            % else:
        reg |= (self.${reg.get_name()} & ${reg.get_mask()}) << ${reg.get_shift()}
            % endif
        % endfor
    % endfor
    # Now the arrays
    # We can do this because all arrays have a base address that is a multiple
    # of 256 (0x100). In other words, this is a hack that only works if you
    # know exactly what you're doing, which is OK in this case, because its in
    # this same file as the register map.
    array_offset = addr % 256
    base_addr = addr - array_offset
    index = int(array_offset / 4)
    % for addr in sorted(set([r.get_addr() for r in regs if r.is_array])):
    if base_addr == ${addr}:
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
        <%
        assert reg.get_array_len() == 256, f"Arrays must be length 256! {reg.get_name()}"
        assert reg.get_addr() % 256 == 0, "Arrays must start at a multiple of 0x100!"
        %>
        % if reg.get_enums():
        reg |= (int(self.${reg.get_name()}[index].value) & ${reg.get_mask()}) << ${reg.get_shift()}
        % else:
        reg |= (int(self.${reg.get_name()}[index]) & ${reg.get_mask()}) << ${reg.get_shift()}
        % endif
        % endfor
    % endfor
    return reg

<%
    ro_regs = set()
    for reg in filter(lambda reg: reg.is_readonly(), regs):
        for index in range(reg.get_array_len() if reg.is_array else 1):
            ro_regs.add(reg.get_addr() + index * reg.get_addr_step_size())

    rw_regs = set()
    for reg in filter(lambda reg: not reg.is_readonly(), regs):
        for index in range(reg.get_array_len() if reg.is_array else 1):
            rw_regs.add(reg.get_addr() + index * reg.get_addr_step_size())
%>
def get_all_addrs(self, include_ro = False):
    addrs = []
    % if len(ro_regs) > 0:
    if (include_ro):
    % for addr in sorted(ro_regs):
        addrs.append(${addr})
    % endfor
    % endif
    % for addr in sorted(rw_regs):
    addrs.append(${addr})
    % endfor
    return addrs

def get_addr(self, reg_name):
    "returns the address of a register with the given name"
    return {
        % for reg in regs:
        '${reg.get_name()}': ${reg.get_addr()},
        % endfor
    }[reg_name]

def set_reg(self, addr, reg):
    % for addr in sorted(set([r.get_addr() for r in regs if not r.is_array])):
    <% if_state = 'if' if loop.index == 0 else 'elif' %>
    ${if_state} addr == ${addr}:
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
        % if reg.get_enums():
        self.${reg.get_name()} = self.${reg.get_name()}_t((reg >> ${reg.get_shift()}) & ${reg.get_mask()})
        % else:
        self.${reg.get_name()} = (reg >> ${reg.get_shift()}) & ${reg.get_mask()}
        % endif
        % endfor
    % endfor

    # Now the arrays
    % for addr in sorted(set([r.get_addr() for r in regs if r.is_array])):
    <% if_state = 'if' if loop.index == 0 else 'elif' %> \
    <% reglist = [r for r in regs if r.get_addr() == addr] %>
    ${if_state} ${addr} <= addr < ${addr} + ${reglist[0].get_array_len()} * 4:
        index = (addr - ${addr}) // 4
        % for reg in reglist:
        % if reg.get_enums():
        self.${reg.get_name()}[index] = self.${reg.get_type()}((reg & (${reg.get_mask()} << ${reg.get_shift()})) >> ${reg.get_shift()})
        % else:
        self.${reg.get_name()}[index] = (reg & (${reg.get_mask()} << ${reg.get_shift()})) >> ${reg.get_shift()}
        % endif
        % endfor
    % endfor
"""


########################################################################
# Template for C++ methods in the body of the struct
########################################################################
BODY_TMPL = """\
enum class hbx_reg_field_t {
    % for addr in sorted(set(map(lambda r: r.get_addr(), regs))):
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
    ${reg.get_name()},
        % endfor
    % endfor
};

hbx_reg_field_t get_field_type(const std::string& field_name) {
    % for addr in sorted(set(map(lambda r: r.get_addr(), regs))):
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
    if (field_name == "${reg.get_name()}") {
        return hbx_reg_field_t::${reg.get_name()};
    } else
        % endfor
    % endfor
    {
        UHD_ASSERT_THROW(false);
    }
}

uint32_t get_array_size(hbx_reg_field_t field) {
    switch(field) {
    % for addr in sorted(set([r.get_addr() for r in regs if r.is_array])):
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
    case hbx_reg_field_t::${reg.get_name()}:
        return uint32_t(${reg.get_name()}.size());
        % endfor
    % endfor
    default:
        UHD_ASSERT_THROW(false);
        return 0;
    }
}

uint32_t get_field(hbx_reg_field_t field) {
    switch(field) {
    % for addr in sorted(set([r.get_addr() for r in regs if not r.is_array])):
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
    case hbx_reg_field_t::${reg.get_name()}:
        return uint32_t(${reg.get_name()});
        % endfor
    % endfor
    default:
        UHD_ASSERT_THROW(false);
        return 0;
    }
}

uint32_t get_field(hbx_reg_field_t field, const size_t idx) {
    % if not any(map(lambda r: r.is_array, regs)):
        (void)field;
        (void)idx;
    % endif
    switch(field) {
    % for addr in sorted(set([r.get_addr() for r in regs if r.is_array])):
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
    case hbx_reg_field_t::${reg.get_name()}:
        return uint32_t(${reg.get_name()}[idx]);
        % endfor
    % endfor
    default:
        UHD_ASSERT_THROW(false);
        return 0;
    }
}

void set_field(hbx_reg_field_t field, uint32_t value) {
    switch(field) {
    % for addr in sorted(set([r.get_addr() for r in regs if not r.is_array])):
        % for reg in filter(lambda r: r.get_addr() == addr and not r.is_readonly(), regs):
    case hbx_reg_field_t::${reg.get_name()}:
        ${reg.get_name()} = static_cast<${reg.get_type()}>(value);
        break;
        % endfor
    % endfor
    default:
        UHD_ASSERT_THROW(false);
    }
}

void set_field(hbx_reg_field_t field, uint32_t value, const size_t idx)
{
    % if not any(map(lambda r: r.is_array, regs)):
        (void)field;
        (void)value;
        (void)idx;
    % endif
    switch(field) {
    % for addr in sorted(set([r.get_addr() for r in regs if r.is_array])):
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
    case hbx_reg_field_t::${reg.get_name()}:
        ${reg.get_name()}[idx] = static_cast<${reg.get_type()}>(value);
        break;
        % endfor
    % endfor
    default:
        UHD_ASSERT_THROW(false);
    }
}

uint32_t get_reg(uint16_t addr)
{
    uint32_t reg = 0;
    // First the regular registers
    switch(addr) {
    % for addr in sorted(set([r.get_addr() for r in regs if not r.is_array])):
    case ${addr}:
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
        reg |= (uint32_t(${reg.get_name()}) & ${reg.get_mask()}) << ${reg.get_shift()};
        % endfor
        break;
    % endfor
    default:
        break;
    }
    % if any(map(lambda r: r.is_array, regs)):
    // Now the arrays
    // We can do this because all arrays have a base address that is a multiple
    // of 256 (0x100). In other words, this is a hack that only works if you
    // know exactly what you're doing, which is OK in this case, because it's in
    // this same file as the register map.
    const uint16_t array_offset = addr % 256;
    const uint16_t base_addr = addr - array_offset;
    const size_t index = array_offset / 4;
    switch(base_addr) {
    % for addr in sorted(set([r.get_addr() for r in regs if r.is_array])):
    case ${addr}:
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
        <% assert reg.get_array_len() == 256, f"Arrays must be length 256! {reg.get_name()}" %>
        <% assert reg.get_addr() % 256 == 0, "Arrays must start at a multiple of 0x100!" %>
        reg |= (uint32_t(${reg.get_name()}[index]) & ${reg.get_mask()}) << ${reg.get_shift()};
        % endfor
        break;
    % endfor
    default:
        break;
    }
    % endif
    return reg;
}

void set_reg(uint16_t addr, uint32_t val)
{
    switch(addr) {
    % for addr in sorted(set([r.get_addr() for r in regs if not r.is_array])):
    case ${addr}:
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
            ${reg.get_name()} = static_cast<${reg.get_type()}>((val & (${reg.get_mask()} << ${reg.get_shift()})) >> ${reg.get_shift()});
        % endfor
        break;
    % endfor
    default:
        break;
    }
    % if any(map(lambda r: r.is_array, regs)):
    // Now the arrays
    // We can do this because all arrays have a base address that is a multiple
    // of 256 (0x100). In other words, this is a hack that only works if you
    // know exactly what you're doing, which is OK in this case, because it's in
    // this same file as the register map.
    const uint16_t array_offset = addr % 256;
    const uint16_t base_addr = addr - array_offset;
    const size_t index = array_offset / 4;
    switch(base_addr) {
    % for addr in sorted(set([r.get_addr() for r in regs if r.is_array])):
        case ${addr}:
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
            ${reg.get_name()}[index] = static_cast<${reg.get_type()}>((val & (${reg.get_mask()} << ${reg.get_shift()})) >> ${reg.get_shift()});
        % endfor
        break;
    % endfor
        default: break;
    }
    % endif
}

<%
    ro_regs = set()
    for reg in filter(lambda reg: reg.is_readonly(), regs):
        for index in range(reg.get_array_len() if reg.is_array else 1):
            ro_regs.add(reg.get_addr() + index * reg.get_addr_step_size())

    rw_regs = set()
    for reg in filter(lambda reg: not reg.is_readonly(), regs):
        for index in range(reg.get_array_len() if reg.is_array else 1):
            rw_regs.add(reg.get_addr() + index * reg.get_addr_step_size())
%>

template<typename T> std::set<T> get_all_addrs(bool include_ro = false)
{
    std::set<T> addrs;
    if (include_ro) {
    % for reg in sorted(ro_regs):
        addrs.insert(${reg});
    % endfor
    }
    % for reg in sorted(rw_regs):
    addrs.insert(${reg});
    % endfor
    return addrs;
}

uint16_t get_addr(hbx_reg_field_t field) {
    switch(field) {
    % for addr in sorted(set([r.get_addr() for r in regs if r.is_array])):
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
    case hbx_reg_field_t::${reg.get_name()}:
        return ${addr};
        % endfor
    % endfor
    default:
        UHD_ASSERT_THROW(false);
        return 0;
    }
}

uint16_t get_addr(const std::string& reg_name)
{
    % for reg in regs:
    if ("${reg.get_name()}" == reg_name) {
        return ${reg.get_addr()};
    }
    % endfor
    return 0;
}
"""

if __name__ == "__main__":
    import sys

    import common

    outfile = sys.argv[1]
    if outfile.endswith(".hpp"):
        REGS_TMPL = "\n".join(line for line in REGS_TMPL.splitlines() if "scope=mpm" not in line)
    if outfile.endswith(".py"):
        REGS_TMPL = "\n".join(line for line in REGS_TMPL.splitlines() if "scope=uhd" not in line)
    common.generate(
        name="hbx_cpld_regs",
        regs_tmpl=REGS_TMPL,
        body_tmpl=BODY_TMPL,
        py_body_tmpl=PY_BODY_TMPL,
        file=__file__,
    )
