# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Register map for the ZBX CPLD. Note that the ground truth for this register map
is in the fpga/usrp3/top/x400/dboards/zbx/cpld subdirectory, and can be looked
up in fpga/usrp3/top/x400/dboards/zbx/cpld/doc/ZBX_CPLD.htm

This CPLD regmap works with RevB daughterboards only.
"""

########################################################################
# Template for raw text data describing registers
# name addr[bit range inclusive] default optional enums
########################################################################
REGS_TMPL = """\
########################################################################
## SLAVE SETUP
########################################################################
BOARD_ID                    0x0000[0:15]            ro
REVISION                    0x0004[0:31]            ro
OLDEST_COMPAT_REVISION      0x0008[0:31]            ro
SCRATCH                     0x000C[0:31]            0
GIT_HASH                    0x0010[0:31]            ro
ENABLE_TX_POS_7V0           0x0040[0]               scope=mpm   disable, enable
ENABLE_RX_POS_7V0           0x0040[1]               scope=mpm   disable, enable
ENABLE_POS_3V3              0x0040[2]               scope=mpm   disable, enable
P7V_B_STATUS                0x0044[0]               scope=mpm,ro
P7V_A_STATUS                0x0044[1]               scope=mpm,ro
PLL_REF_CLOCK_ENABLE        0x0048[0]               scope=mpm   disable, enable
########################################################################
## ATR
########################################################################
CURRENT_RF0_CONFIG          0x1000[0:7]             ro
CURRENT_RF1_CONFIG          0x1000[8:15]            ro
CURRENT_RF0_DSA_CONFIG      0x1000[23:16]           ro
CURRENT_RF1_DSA_CONFIG      0x1000[31:24]           ro
RF0_OPTION                  0x1004[0:1]             0   sw_defined, classic_atr, fpga_state
RF1_OPTION                  0x1004[8:9]             0   sw_defined, classic_atr, fpga_state
RF0_DSA_OPTION              0x1004[17:16]           0   sw_defined, classic_atr, fpga_state
RF1_DSA_OPTION              0x1004[25:24]           0   sw_defined, classic_atr, fpga_state
SW_RF0_CONFIG               0x1008[0:7]             0
SW_RF1_CONFIG               0x1008[8:15]            0
SW_RF0_DSA_CONFIG           0x1008[23:16]           0
SW_RF1_DSA_CONFIG           0x1008[31:24]           0
########################################################################
## LO SPI
########################################################################
DATA                        0x1020[0:15]            0
ADDRESS                     0x1020[16:22]           0
READ_FLAG                   0x1020[23]              1   write, read
LO_SELECT                   0x1020[24:26]           0   TX0_LO1, TX0_LO2, TX1_LO1, TX1_LO2, RX0_LO1, RX0_LO2, RX1_LO1, RX1_LO2
START_TRANSACTION           0x1020[28]              0   disable, enable
SPI_READY                   0x1020[30]              ro
DATA_VALID                  0x1020[31]              ro
########################################################################
## LO SYNC
########################################################################
PULSE_TX0_LO1_SYNC          0x1024[0]               0
PULSE_TX0_LO2_SYNC          0x1024[1]               0
PULSE_TX1_LO1_SYNC          0x1024[2]               0
PULSE_TX1_LO2_SYNC          0x1024[3]               0
PULSE_RX0_LO1_SYNC          0x1024[4]               0
PULSE_RX0_LO2_SYNC          0x1024[5]               0
PULSE_RX1_LO1_SYNC          0x1024[6]               0
PULSE_RX1_LO2_SYNC          0x1024[7]               0
BYPASS_SYNC_REGISTER        0x1024[8]               0   disable, enable
########################################################################
## LED CONTROL
########################################################################
RX0_RX_LED[256]             0x1400[0]               0   disable, enable
RX0_TRX_LED[256]            0x1400[1]               0   disable, enable
TX0_TRX_LED[256]            0x1400[2]               0   disable, enable
RX1_RX_LED[256]             0x1400[16]              0   disable, enable
RX1_TRX_LED[256]            0x1400[17]              0   disable, enable
TX1_TRX_LED[256]            0x1400[18]              0   disable, enable
########################################################################
## PATH CONTROL - TX 0
########################################################################
TX0_IF2_1_2[256]            0x2000[0]               0   filter_2, filter_1
TX0_IF1_3[256]              0x2000[2:3]             0   filter_0_3, filter_4, filter_6, filter_5
TX0_IF1_4[256]              0x2000[4:5]             0   termination, filter_1, filter_2, filter_3
TX0_IF1_5[256]              0x2000[6:7]             0   filter_3, filter_2, filter_1, termination
TX0_IF1_6[256]              0x2000[8:9]             0   filter_5, filter_6, filter_4, filter_0_3
TX0_7[256]                  0x2000[10:11]           0   termination, no_connect, highband, lowband
TX0_RF_8[256]               0x2000[12:14]           0   invalid_0, rf_3, rf_1, invalid_1, rf_2
TX0_RF_9[256]               0x2000[16:17]           0   rf_3, rf_1, rf_2, highband
TX0_ANT_10[256]             0x2000[18:19]           0   bypass_amp, cal_loopback, lowband_amp, highband_amp
TX0_ANT_11[256]             0x2000[20:21]           0   tx_rx, highband_amp, lowband_amp, bypass_amp
TX0_LO_13[256]              0x2000[24]              0   internal, external
TX0_LO_14[256]              0x2000[26]              0   external, internal
########################################################################
## PATH CONTROL - TX 1
########################################################################
TX1_IF2_1_2[256]            0x2400[0]               0   filter_1, filter_2
TX1_IF1_3[256]              0x2400[2:3]             0   filter_5, filter_6, filter_4, filter_0_3
TX1_IF1_4[256]              0x2400[4:5]             0   filter_3, filter_2, filter_1, termination
TX1_IF1_5[256]              0x2400[6:7]             0   termination, filter_1, filter_2, filter_3
TX1_IF1_6[256]              0x2400[8:9]             0   filter_0_3, filter_4, filter_6, filter_5
TX1_7[256]                  0x2400[10:11]           0   lowband, highband, no_connect, termination
TX1_RF_8[256]               0x2400[12:14]           0   invalid_0, rf_2, rf_1, invalid_1, rf_3
TX1_RF_9[256]               0x2400[16:17]           0   highband, rf_2, rf_1, rf_3
TX1_ANT_10[256]             0x2400[18:19]           0   highband_amp, lowband_amp, bypass_amp, cal_loopback
TX1_ANT_11[256]             0x2400[20:21]           0   tx_rx, bypass_amp, lowband_amp, highband_amp
TX1_LO_13[256]              0x2400[24]              0   external, internal
TX1_LO_14[256]              0x2400[26]              0   internal, external
########################################################################
## PATH CONTROL - RX 0
########################################################################
RX0_ANT_1[256]              0x2800[0:1]             0   cal_loopback, termination, tx_rx, rx2
RX0_2[256]                  0x2800[2]               0   highband, lowband
RX0_RF_3[256]               0x2800[4:6]             0   invalid_0, rf_2, rf_1, invalid_1, rf_3
RX0_4[256]                  0x2800[8]               0   lowband, highband
RX0_IF1_5[256]              0x2800[10:11]           0   filter_4, filter_3, filter_2, filter_1
RX0_IF1_6[256]              0x2800[12:13]           0   filter_1, filter_2, filter_3, filter_4
RX0_IF2_7_8[256]            0x2800[14]              0   filter_2, filter_1
RX0_LO_9[256]               0x2800[16]              0   internal, external
RX0_LO_10[256]              0x2800[18]              0   internal, external
RX0_RF_11[256]              0x2800[20:22]           0   invalid_0, rf_3, rf_1, invalid_1, rf_2, invalid_2, invalid_3, invalid_4
########################################################################
## PATH CONTROL - RX 1
########################################################################
RX1_ANT_1[256]              0x2C00[0:1]             0   cal_loopback, tx_rx, rx2, termination
RX1_2[256]                  0x2C00[2]               0   lowband, highband
RX1_RF_3[256]               0x2C00[4:6]             0   invalid_0, rf_3, rf_1, invalid_1, rf_2
RX1_4[256]                  0x2C00[8]               0   highband, lowband
RX1_IF1_5[256]              0x2C00[10:11]           0   filter_1, filter_2, filter_3, filter_4
RX1_IF1_6[256]              0x2C00[12:13]           0   filter_4, filter_3, filter_2, filter_1
RX1_IF2_7_8[256]            0x2C00[14]              0   filter_1, filter_2
RX1_LO_9[256]               0x2C00[16]              0   external, internal
RX1_LO_10[256]              0x2C00[18]              0   external, internal
RX1_RF_11[256]              0x2C00[20:22]           0   invalid_0, rf_2, rf_1, invalid_1, rf_3, invalid_2, invalid_3, invalid_4
########################################################################
## DSA CONTROL
########################################################################
TX0_DSA1[256]               0x3000[0:4]             31
TX0_DSA2[256]               0x3000[8:12]            31
TX1_DSA1[256]               0x3400[0:4]             31
TX1_DSA2[256]               0x3400[8:12]            31
RX0_DSA1[256]               0x3800[0:3]             15
RX0_DSA2[256]               0x3800[4:7]             15
RX0_DSA3_A[256]             0x3800[8:11]            15
RX0_DSA3_B[256]             0x3800[12:15]           15
RX1_DSA1[256]               0x3C00[0:3]             15
RX1_DSA2[256]               0x3C00[4:7]             15
RX1_DSA3_A[256]             0x3C00[8:11]            15
RX1_DSA3_B[256]             0x3C00[12:15]           15
TX0_TABLE_SELECT[256]       0x4000[0:7]             0
TX1_TABLE_SELECT[256]       0x4400[0:7]             0
RX0_TABLE_SELECT[256]       0x4800[0:7]             0
RX1_TABLE_SELECT[256]       0x4C00[0:7]             0
TX0_TABLE_DSA1[256]         0x5000[0:4]             31
TX0_TABLE_DSA2[256]         0x5000[8:12]            31
TX1_TABLE_DSA1[256]         0x5400[0:4]             31
TX1_TABLE_DSA2[256]         0x5400[8:12]            31
RX0_TABLE_DSA1[256]         0x5800[0:3]             15
RX0_TABLE_DSA2[256]         0x5800[4:7]             15
RX0_TABLE_DSA3_A[256]       0x5800[8:11]            15
RX0_TABLE_DSA3_B[256]       0x5800[12:15]           15
RX1_TABLE_DSA1[256]         0x5C00[0:3]             15
RX1_TABLE_DSA2[256]         0x5C00[4:7]             15
RX1_TABLE_DSA3_A[256]       0x5C00[8:11]            15
RX1_TABLE_DSA3_B[256]       0x5C00[12:15]           15
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
        assert reg.get_array_len() == 256, "Arrays must be length 256!"
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
    all_addrs = set()
    for reg in regs:
        for index in range(reg.get_array_len() if reg.is_array else 1):
            all_addrs.add(reg.get_addr() + index * reg.get_addr_step_size())
%>
def get_all_regs(self):
    addrs = {
    % for addr in sorted(all_addrs):
        ${addr},
    % endfor
    }
    return addrs

def get_addr(self, reg_name):
    "returns the address of a register with the given name"
    return {
        % for reg in regs:
        '${reg.get_name()}': ${reg.get_addr()},
        % endfor
    }[reg_name]
"""


########################################################################
# Template for C++ methods in the body of the struct
########################################################################
BODY_TMPL = """\
enum class zbx_cpld_field_t {
    % for addr in sorted(set(map(lambda r: r.get_addr(), regs))):
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
    ${reg.get_name()},
        % endfor
    % endfor
};

zbx_cpld_field_t get_field_type(const std::string& field_name) {
    % for addr in sorted(set(map(lambda r: r.get_addr(), regs))):
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
    if (field_name == "${reg.get_name()}") {
        return zbx_cpld_field_t::${reg.get_name()};
    } else
        % endfor
    % endfor
    {
        UHD_ASSERT_THROW(false);
    }
}

uint32_t get_array_size(zbx_cpld_field_t field) {
    switch(field) {
    % for addr in sorted(set([r.get_addr() for r in regs if r.is_array])):
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
    case zbx_cpld_field_t::${reg.get_name()}:
        return uint32_t(${reg.get_name()}.size());
        % endfor
    % endfor
    default:
        UHD_ASSERT_THROW(false);
        return 0;
    }
}

uint32_t get_field(zbx_cpld_field_t field) {
    switch(field) {
    % for addr in sorted(set([r.get_addr() for r in regs if not r.is_array])):
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
    case zbx_cpld_field_t::${reg.get_name()}:
        return uint32_t(${reg.get_name()});
        % endfor
    % endfor
    default:
        UHD_ASSERT_THROW(false);
        return 0;
    }
}

uint32_t get_field(zbx_cpld_field_t field, const size_t idx) {
    switch(field) {
    % for addr in sorted(set([r.get_addr() for r in regs if r.is_array])):
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
    case zbx_cpld_field_t::${reg.get_name()}:
        return uint32_t(${reg.get_name()}[idx]);
        % endfor
    % endfor
    default:
        UHD_ASSERT_THROW(false);
        return 0;
    }
}

void set_field(zbx_cpld_field_t field, uint32_t value) {
    switch(field) {
    % for addr in sorted(set([r.get_addr() for r in regs if not r.is_array])):
        % for reg in filter(lambda r: r.get_addr() == addr and not r.is_readonly(), regs):
    case zbx_cpld_field_t::${reg.get_name()}:
        ${reg.get_name()} = static_cast<${reg.get_type()}>(value);
        break;
        % endfor
    % endfor
    default:
        UHD_ASSERT_THROW(false);
    }
}

void set_field(zbx_cpld_field_t field, uint32_t value, const size_t idx)
{
    switch(field) {
    % for addr in sorted(set([r.get_addr() for r in regs if r.is_array])):
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
    case zbx_cpld_field_t::${reg.get_name()}:
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
        <% assert reg.get_array_len() == 256, "Arrays must be length 256!" %>
        <% assert reg.get_addr() % 256 == 0, "Arrays must start at a multiple of 0x100!" %>
        reg |= (uint32_t(${reg.get_name()}[index]) & ${reg.get_mask()}) << ${reg.get_shift()};
        % endfor
        break;
    % endfor
    default:
        break;
    }
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
}

<%
    ro_regs = set()
    for reg in filter(lambda reg: reg.is_readonly(), filter(all_addr_filter, regs)):
        for index in range(reg.get_array_len() if reg.is_array else 1):
            ro_regs.add(reg.get_addr() + index * reg.get_addr_step_size())

    rw_regs = set()
    for reg in filter(lambda reg: not reg.is_readonly(), filter(all_addr_filter, regs)):
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

uint16_t get_addr(zbx_cpld_field_t field) {
    switch(field) {
    % for addr in sorted(set([r.get_addr() for r in regs if r.is_array])):
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
    case zbx_cpld_field_t::${reg.get_name()}:
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

if __name__ == '__main__':
    import sys
    import common
    outfile=sys.argv[1]
    if outfile.endswith(".hpp"):
        all_addr_filter = lambda reg: "mpm" not in reg.options.get("scope", "")
    else:
        all_addr_filter = lambda reg: True
    common.generate(
        name='zbx_cpld_regs',
        regs_tmpl=REGS_TMPL,
        body_tmpl=BODY_TMPL,
        py_body_tmpl=PY_BODY_TMPL,
        file=__file__,
        all_addr_filter=all_addr_filter
    )
