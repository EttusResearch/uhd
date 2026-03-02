# Copyright 2026 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

"""Register map for ADMV1320."""

########################################################################
# Template for raw text data describing registers
# name addr[bit range inclusive] default optional enums
########################################################################
REGS_TMPL = """\
########################################################################
## ADMV1320 Register Map
########################################################################
SOFT_RESET                  0x000[0]            0x00
LSB_FIRST                   0x000[1]            0x00
ADDR_ASCN                   0x000[2]            0x00
SDO_ACTIVE                  0x000[3]            0x00
SDO_ACTIVE_R                0x000[4]            0x00
ADDR_ASCN_R                 0x000[5]            0x00
LSB_FIRST_R                 0x000[6]            0x00
SOFT_RESET_R                0x000[7]            0x00
PRODUCT_ID_L                0x004[0:7]          ro
PRODUCT_ID_H                0x005[0:7]          ro
SCRATCH_PAD                 0x00A[0:7]          0x00
SDO_LEVEL                   0x013[0]            0x01
RF_ENV_TAP                  0x138[1]            0x00
ENV_DET_PD                  0x138[2]            0x01
ADC_BIAS_PD                 0x13B[0]            0x00
CHIP_PD                     0x13E[0]            0x00
RF_HS_PD                    0x13E[1]            0x00
FILTER_ADDRESS_STEP         0x200[0:7]          0x01
FILTER_TABLE_EN             0x202[0]            0x00
FILTER_MODE                 0x203[0:1]          0x00
FILTER_ADDRESS_RESET        0x204[0]            0x01
FILTER_TABLE_SELECT         0x207[0]            0x00
FILTER_LOAD_EN              0x208[0]            0x00
FILTER_ADDRESS_POINTER      0x209[0:4]          0x00
FILTER_PINS_READBACK        0x210[0:4]          ro
FILTER_READBACK_L1          0x220[0:7]          ro
FILTER_READBACK_L2          0x221[0:7]          ro
FILTER_READBACK_L3          0x222[0:7]          ro
FILTER_READBACK_L4          0x223[0:7]          ro
FILTER_READBACK_L5          0x224[0:7]          ro
FILTER_READBACK_L6          0x225[0:7]          ro
FILTER_READBACK_L7          0x226[0:7]          ro
FILTER_READBACK_L8          0x227[0:7]          ro
FILTER_READBACK_L9          0x228[0:7]          ro
FILTER_ADDRESS_READBACK     0x240[0:7]          ro
GAIN_LOAD_EN                0x281[0]            0x00
GAIN_ADDRESS_POINTER        0x282[0:6]          0x00
GAIN_ADDRESS_STEP           0x283[0:7]          0x01
GAIN_ADDRESS_OFFSET         0x284[0:7]          0x00
GAIN_TABLE_EN               0x285[0]            0x00
GAIN_TABLE_MODE             0x286[0:1]          0x00
GAIN_ADDRESS_RESET          0x287[0]            0x01
GAIN_TABLE_BYPASS_EN        0x28A[0]            0x00
DSA1_BYPASS_VALUE           0x28B[0:3]          0x00
DSA2_BYPASS_VALUE           0x28B[4:7]          0x00
LPF_VALUE                   0x2A0[0:3]          0x00
LPF_SELECT                  0x2A0[4]            0x00
HPF_VALUE                   0x2A1[0:5]          0x00
HPF_SELECT                  0x2A1[7]            0x00
LPF_READBACK                0x2A3[0:3]          ro
HPF_READBACK                0x2A4[0:5]          ro
ADC_MUX_SEL                 0x2A7[0:4]          0x00
ADC_CLK_EN                  0x300[0]            0x00
ADC_CLK_FREQ_SEL            0x300[1:2]          0x01
ADC_EN                      0x300[3]            0x00
ADC_SPI_PD                  0x300[4]            0x00
ADC_CLK_DIVIDER             0x300[5:7]          0x04
ADC_RESET                   0x302[0]            0x00
LON_AUTO_EN                 0x302[1]            0x00
LON_CURRENT                 0x304[0:3]          0x08
LON_I_SELECT                0x305[0]            0x00
LON_Q_SELECT                0x305[1]            0x00
LON_EN                      0x305[2]            0x00
LON_IQ_SEL                  0x305[3]            0x00
LON_POLARITY                0x306[0]            0x00
LON_START                   0x306[1]            0x00
ADC_CLK_FROM_SPI_EN         0x306[2]            0x00
ADC_OUT                     0x30D[0:7]          ro
RF_DSA1_GAIN                0x600[0:3]          0x00
RF_DSA2_GAIN                0x600[4:7]          0x00
GPO_G                       0x601[0:6]          0x00
BB_EN                       0x60A[0]            0x00
IF_EN                       0x60A[1]            0x01
IF_BAND_EN                  0x60A[2]            0x00
MIXER_GATE                  0x700[0:6]          0x00
MIXER_GATE_SELECT           0x700[7]            0x00
IF_VCM                      0x701[0:6]          0x00
BB_SW_NMOS                  0x702[0:6]          0x00
BB_SW_SELECT                0x702[7]            0x00
MIXER_BYPASS                0x703[0]            0x00
REG703_DEFAULT              0x703[3]            0x01
GPO_F_OE_L                  0x780[0:7]          0x00
GPO_F_OE_H                  0x781[0]            0x00
GPO_G_OE                    0x781[1:7]          0x00
DIRECT_LO_X3_FILTER         0x800[0:3]          0x00
DIRECT_LO_SIDEBAND          0x800[4]            0x00
DIRECT_RF_BAND_SELECT       0x800[5:6]          0x03
GPO_F_L                     0x800[7]            0x00
GPO_F_H                     0x801[0:7]          0x00
DIRECT_LO_PHASE_I           0x802[0:4]          0x00
DIRECT_LO_PHASE_Q           0x803[0:4]          0x00
DIRECT_LON_OFFSET_I         0x804[0:5]          0x00
DIRECT_LON_OFFSET_Q         0x805[0:5]          0x00
DIRECT_DSA1_OFFSET          0x806[0:3]          0x00
DIRECT_DSA2_OFFSET          0x806[4:7]          0x00
DIRECT_IF_GAIN_I            0x807[0:3]          0x00
DIRECT_IF_GAIN_Q            0x807[4:7]          0x00
DIRECT_LPF                  0x808[0:3]          0x00
DIRECT_HPF                  0x808[4:7]          0x00
DSA1_CAL_EN                 0x860[0]            0x01
DSA2_CAL_EN                 0x860[1]            0x00
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
    common.generate(
        name="admv1320_regs",
        regs_tmpl=REGS_TMPL,
        body_tmpl=BODY_TMPL,
        py_body_tmpl=PY_BODY_TMPL,
        file=__file__,
    )
