"""Register map for LTC5594."""

#!/usr/bin/env python3
#
# Copyright 2026 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# Template for raw text data describing registers
# name addr[bit range inclusive] default optional enums
########################################################################
REGS_TMPL = """\
########################################################################
## LTC5594 Register Map
########################################################################
IM3QY                   0x00[0:7]           0x80
IM3QX                   0x01[0:7]           0x80
IM3IY                   0x02[0:7]           0x80
IM3IX                   0x03[0:7]           0x80
IM2QX                   0x04[0:7]           0x80
IM2IX                   0x05[0:7]           0x80
HD3QY                   0x06[0:7]           0x80
HD3QX                   0x07[0:7]           0x80
HD3IY                   0x08[0:7]           0x80
HD3IX                   0x09[0:7]           0x80
HD2QY                   0x0A[0:7]           0x80
HD2QX                   0x0B[0:7]           0x80
HD2IY                   0x0C[0:7]           0x80
HD2IX                   0x0D[0:7]           0x80
DCOI                    0x0E[0:7]           0x80
DCOQ                    0x0F[0:7]           0x80
IP3IC                   0x10[0:2]           0x04
GERR                    0x11[2:7]           0x20
IP3CC                   0x11[1:0]           0x02
LVCM                    0x12[5:7]           0x02
CF1                     0x12[0:4]           0x08
BAND                    0x13[7]             0x01
LF1                     0x13[5:6]           0x03
CF2                     0x13[0:4]           0x03
PHA_H                   0x14[0:7]           0x80
PHA_L                   0x15[0]             0x00
AMPG                    0x15[4:6]           0x06
AMPCC                   0x15[2:3]           0x02
AMPIC                   0x15[0:1]           0x02
EDEM                    0x16[7]             0x01
EDC                     0x16[6]             0x01
EADJ                    0x16[5]             0x01
EAMP                    0x16[4]             0x01
SRST                    0x16[3]             0x00
SDO_MODE                0x16[2]             0x00
CHIPID                  0x17[6:7]           0x00
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
        name="ltc5594_regs",
        regs_tmpl=REGS_TMPL,
        body_tmpl=BODY_TMPL,
        py_body_tmpl=PY_BODY_TMPL,
        file=__file__,
    )
