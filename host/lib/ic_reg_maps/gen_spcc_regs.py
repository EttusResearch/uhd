#
# Copyright 2019 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# Template for raw text data describing registers
# name addr[bit range inclusive] default optional enums
########################################################################
REGS_TMPL="""\
########################################################################
## Core Common
########################################################################
<% addr_0 = 0x0 %>\
signature                   ${addr_0}[0:31]         0
version                     ${addr_0+4}[0:31]       0
instantiated                ${addr_0+8}[0:31]       0
########################################################################
## JTAG 0
########################################################################
## No designated registers for the JTAG master
########################################################################
## UART 0 & 1
########################################################################
% for i in range(2):
<% start_addr = addr_0 + 0x040 %>\
<% addr = ((i*32) + start_addr) %>\
clkdiv_uart${i}             ${addr}[0:31]           0
data_uart${i}               ${addr+4}[0:31]         0
txlvl_uart${i}              ${addr+8}[0:31]         0
rxlvl_uart${i}              ${addr+12}[0:31]        0
% endfor
########################################################################
## SPI 0, 1, 2, & 3
########################################################################
% for i in range(4):
<% start_addr =  addr_0 + 0x080 %>\
<% addr = ((i*32) + start_addr) %>\
rx_data_lo_spi${i}          ${addr}[0:31]           0
rx_data_hi_spi${i}          ${addr+4}[0:31]         0
~rx_data_spi${i}            rx_data_lo_spi${i}, rx_data_hi_spi${i}
tx_data_lo_spi${i}          ${addr+8}[0:31]         0
tx_data_hi_spi${i}          ${addr+12}[0:31]        0
~tx_data_spi${i}            tx_data_lo_spi${i}, tx_data_hi_spi${i}
ctrl_reg_spi${i}            ${addr+16}[0:31]        0
div_spi${i}                 ${addr+20}[0:31]        0
ss_spi${i}                  ${addr+24}[0:31]        0
% endfor
########################################################################
## GPIO ATR 0, 1, 2, & 3
########################################################################
% for i in range(4):
<% start_addr =  addr_0 + 0x100 %>\
<% addr = ((i*32) + start_addr) %>\
idled_gpio${i}              ${addr}[0:31]           0
rxd_gpio${i}                ${addr+4}[0:31]         0
txd_gpio${i}                ${addr+8}[0:31]         0
fdxd_gpio${i}               ${addr+12}[0:31]        0
ddr_gpio${i}                ${addr+16}[0:31]        0
atren_gpio${i}              ${addr+20}[0:31]        0
fabctrl_gpio${i}            ${addr+24}[0:31]        0
rb_gpio${i}                 ${addr+28}[0:31]        0
% endfor
########################################################################
## I2C 0, 1, 2, & 3
########################################################################
% for i in range(4):
<% start_addr =  addr_0 + 0x180 %>\
<% addr = ((i*32) + start_addr) %>\
prerlo_i2c${i}              ${addr}[0:31]           0
prerhi_i2c${i}              ${addr+4}[0:31]         0
~prer_i2c${i}               prerlo_i2c${i}, prerhi_i2c${i}
ctrl_i2c${i}                ${addr+8}[0:31]         0
tx_i2c${i}                  ${addr+12}[0:31]        0
rx_i2c${i}                  ${addr+16}[0:31]        0
cmd_i2c${i}                 ${addr+20}[0:31]        0
stat_i2c${i}                ${addr+24}[0:31]        0
% endfor
"""
########################################################################
# Template for methods in the body of the struct
########################################################################

PY_BODY_TMPL = """\
def get_reg(self, addr):
    reg = 0
    % for addr in sorted(set(map(lambda r: r.get_addr(), regs))):
    <% if_state = 'if' if loop.index == 0 else 'elif' %>${if_state} addr == ${addr}:
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
        % if reg.get_enums():
        reg |= (self.${reg.get_name()}.value & ${reg.get_mask()}) << ${reg.get_shift()}
        % else:
        reg |= (self.${reg.get_name()} & ${reg.get_mask()}) << ${reg.get_shift()}
        % endif
        % endfor
    % endfor
    return reg

def set_reg(self, addr, value):
    # writes value to a given address
    % for addr in sorted(set(map(lambda r: r.get_addr(), regs))):
    <% if_state = 'if' if loop.index == 0 else 'elif' %>${if_state} addr == ${addr}:
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
        self.${reg.get_name()} = (value >> ${reg.get_shift()}) & ${reg.get_mask()}
        % endfor
    % endfor
"""

if __name__ == '__main__':
    import common; common.generate(
        name='spcc_regs',
        regs_tmpl=REGS_TMPL,
        py_body_tmpl=PY_BODY_TMPL,
        file=__file__,
    )
