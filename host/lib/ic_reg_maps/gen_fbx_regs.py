# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Register map for the FBX. Note that the ground truth for this register map
is in the fpga/usrp3/top/x400/dboards/fbx subdirectory, and can be looked
up in fpga/usrp3/top/x400/doc/x440/X440_FPGA.htm
"""

########################################################################
# Template for raw text data describing registers
# name addr[bit range inclusive] default optional enums
########################################################################
REGS_TMPL = """\
########################################################################
## GLOBAL REGISTERS
########################################################################
SCRATCH                     0x000C[0:31]            0
DEVICE_ID                   0x0010[15:0]            ro   ro
########################################################################
## RF0 ATR STATE
########################################################################
RF0_TDDS[256]               0x0000[2]               0   RF2, RF1
RF0_RX_RFS[256]             0x0000[1]               0   RF2, RF1
RF0_TX_RX_RFS[256]          0x0000[0]               0   RF2, RF1
########################################################################
## RF1 ATR STATE
########################################################################
RF1_TDDS[256]               0x0400[2]               0   RF2, RF1
RF1_RX_RFS[256]             0x0400[1]               0   RF2, RF1
RF1_TX_RX_RFS[256]          0x0400[0]               0   RF2, RF1
########################################################################
## RF2 ATR STATE
########################################################################
RF2_TDDS[256]               0x0800[2]               0   RF2, RF1
RF2_RX_RFS[256]             0x0800[1]               0   RF2, RF1
RF2_TX_RX_RFS[256]          0x0800[0]               0   RF2, RF1
########################################################################
## RF3 ATR STATE
########################################################################
RF3_TDDS[256]               0x0C00[2]               0   RF2, RF1
RF3_RX_RFS[256]             0x0C00[1]               0   RF2, RF1
RF3_TX_RX_RFS[256]          0x0C00[0]               0   RF2, RF1
########################################################################
## ATR OPTIONS
########################################################################
RF0_ATR_OPTION              0x1000[0]               1   DB, CLASSIC
RF1_ATR_OPTION              0x1000[1]               1   DB, CLASSIC
RF2_ATR_OPTION              0x1000[2]               1   DB, CLASSIC
RF3_ATR_OPTION              0x1000[3]               1   DB, CLASSIC
LED0_ATR_OPTION             0x5000[0]               1   DB, CLASSIC
LED1_ATR_OPTION             0x5000[1]               1   DB, CLASSIC
LED2_ATR_OPTION             0x5000[2]               1   DB, CLASSIC
LED3_ATR_OPTION             0x5000[3]               1   DB, CLASSIC
########################################################################
## Sync Switches
########################################################################
SYNC0_CTRL                  0x2000[2:0]             0   NONE, RF1, RF2, RF3, RF4, OFF
SYNC1_CTRL                  0x2004[2:0]             0   NONE, RF1, RF2, RF3, RF4, OFF
SYNC2_CTRL                  0x2008[2:0]             0   NONE, RF1, RF2, RF3, RF4, OFF
SYNC3_CTRL                  0x200C[2:0]             0   NONE, RF1, RF2, RF3, RF4, OFF
SYNC4_CTRL                  0x2010[2:0]             0   NONE, RF1, RF2, RF3, RF4, OFF
IO_EXP_SETUP                0x2018[0]               1   NONE, INIT
IO_EXP_CONFIG               0x201c[0]               0   NONE, CONFIG
########################################################################
## LED CONTROL
########################################################################
CH3_TRX1_LED_GR[256]        0x4C00[2]               0   disable, enable
CH3_TRX1_LED_RED[256]       0x4C00[1]               0   disable, enable
CH3_RX2_LED[256]            0x4C00[0]               0   disable, enable
CH2_TRX1_LED_GR[256]        0x4800[2]               0   disable, enable
CH2_TRX1_LED_RED[256]       0x4800[1]               0   disable, enable
CH2_RX2_LED[256]            0x4800[0]               0   disable, enable
CH1_TRX1_LED_GR[256]        0x4400[2]               0   disable, enable
CH1_TRX1_LED_RED[256]       0x4400[1]               0   disable, enable
CH1_RX2_LED[256]            0x4400[0]               0   disable, enable
CH0_TRX1_LED_GR[256]        0x4000[2]               0   disable, enable
CH0_TRX1_LED_RED[256]       0x4000[1]               0   disable, enable
CH0_RX2_LED[256]            0x4000[0]               0   disable, enable
########################################################################
## Internal Sync Signal
########################################################################
SYNC_CLK                    0x6000[0]               0   disable, enable
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
enum class fbx_reg_field_t {
    % for addr in sorted(set(map(lambda r: r.get_addr(), regs))):
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
    ${reg.get_name()},
        % endfor
    % endfor
};

fbx_reg_field_t get_field_type(const std::string& field_name) {
    % for addr in sorted(set(map(lambda r: r.get_addr(), regs))):
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
    if (field_name == "${reg.get_name()}") {
        return fbx_reg_field_t::${reg.get_name()};
    } else
        % endfor
    % endfor
    {
        UHD_ASSERT_THROW(false);
    }
}

uint32_t get_array_size(fbx_reg_field_t field) {
    switch(field) {
    % for addr in sorted(set([r.get_addr() for r in regs if r.is_array])):
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
    case fbx_reg_field_t::${reg.get_name()}:
        return uint32_t(${reg.get_name()}.size());
        % endfor
    % endfor
    default:
        UHD_ASSERT_THROW(false);
        return 0;
    }
}

uint32_t get_field(fbx_reg_field_t field) {
    switch(field) {
    % for addr in sorted(set([r.get_addr() for r in regs if not r.is_array])):
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
    case fbx_reg_field_t::${reg.get_name()}:
        return uint32_t(${reg.get_name()});
        % endfor
    % endfor
    default:
        UHD_ASSERT_THROW(false);
        return 0;
    }
}

uint32_t get_field(fbx_reg_field_t field, const size_t idx) {
    switch(field) {
    % for addr in sorted(set([r.get_addr() for r in regs if r.is_array])):
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
    case fbx_reg_field_t::${reg.get_name()}:
        return uint32_t(${reg.get_name()}[idx]);
        % endfor
    % endfor
    default:
        UHD_ASSERT_THROW(false);
        return 0;
    }
}

void set_field(fbx_reg_field_t field, uint32_t value) {
    switch(field) {
    % for addr in sorted(set([r.get_addr() for r in regs if not r.is_array])):
        % for reg in filter(lambda r: r.get_addr() == addr and not r.is_readonly(), regs):
    case fbx_reg_field_t::${reg.get_name()}:
        ${reg.get_name()} = static_cast<${reg.get_type()}>(value);
        break;
        % endfor
    % endfor
    default:
        UHD_ASSERT_THROW(false);
    }
}

void set_field(fbx_reg_field_t field, uint32_t value, const size_t idx)
{
    switch(field) {
    % for addr in sorted(set([r.get_addr() for r in regs if r.is_array])):
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
    case fbx_reg_field_t::${reg.get_name()}:
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
        <% assert reg.get_array_len() == 256, f"Arrays must be length 256! {reg.get_name()}" %>
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

uint16_t get_addr(fbx_reg_field_t field) {
    switch(field) {
    % for addr in sorted(set([r.get_addr() for r in regs if r.is_array])):
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
    case fbx_reg_field_t::${reg.get_name()}:
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
        all_addr_filter = lambda reg: "mpm" not in reg.options.get("scope", "")
    else:
        all_addr_filter = lambda reg: True
    common.generate(
        name="fbx_regs",
        regs_tmpl=REGS_TMPL,
        body_tmpl=BODY_TMPL,
        py_body_tmpl=PY_BODY_TMPL,
        file=__file__,
        all_addr_filter=all_addr_filter,
    )
