# Copyright 2022 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Register map for the X4xx RFDC space.

The ground truth for this register map is mostly encoded in the x4xx_ps_rfdc_bd
block diagram.
"""

########################################################################
# Template for raw text data describing registers
# name addr[bit range inclusive] default optional enums
########################################################################
REGS_TMPL = """\
########################################################################
## RFDC Control
########################################################################
## MMCM control (DRP registers)
MMCM_POWER                  0x00300[0:15]           0xFFFF
CLKOUT0_LO_TIME             0x00304[0:5]            5
CLKOUT0_HI_TIME             0x00304[6:11]           5
CLKOUT0_RSVRD1              0x00304[12]             1
CLKOUT0_PHASE_MUX           0x00304[13:15]          0
CLKOUT0_DELAY_TIME          0x00308[0:5]            0
CLKOUT0_NO_COUNT            0x00308[6]              1
CLKOUT0_EDGE                0x00308[7]              1
CLKOUT0_MX                  0x00308[8:9]            0
CLKOUT0_FRAC_WF_R           0x00308[10]             0
CLKOUT0_FRAC_EN             0x00308[11]             0
CLKOUT0_FRAC                0x00308[12:14]          0
## CLKOUT0 has some fractional settings on CLKOUT5's second register
CLKOUT0_FRAC_WF_F           0x00330[12]             0
CLKOUT0_PHASE_MUX_F         0x00330[13:15]          0
CLKOUT1_LO_TIME             0x0030C[0:5]            5
CLKOUT1_HI_TIME             0x0030C[6:11]           5
CLKOUT1_RSVRD1              0x0030C[12]             1
CLKOUT1_PHASE_MUX           0x0030C[13:15]          0
CLKOUT1_DELAY_TIME          0x00310[0:5]            0
CLKOUT1_NO_COUNT            0x00310[6]              1
CLKOUT1_EDGE                0x00310[7]              1
CLKOUT1_MX                  0x00310[8:9]            0
CLKOUT1_RSVRD               0x00310[11:12]          0
CLKOUT1_CLKOUTPHYMODE       0x00310[13:14]          0
CLKOUT2_LO_TIME             0x00314[0:5]            5
CLKOUT2_HI_TIME             0x00314[6:11]           5
CLKOUT2_RSVRD1              0x00314[12]             1
CLKOUT2_PHASE_MUX           0x00314[13:15]          0
CLKOUT2_DELAY_TIME          0x00318[0:5]            0
CLKOUT2_NO_COUNT            0x00318[6]              1
CLKOUT2_EDGE                0x00318[7]              1
CLKOUT2_MX                  0x00318[8:9]            0
CLKOUT2_RSVRD               0x00318[11:15]          0
CLKOUT3_LO_TIME             0x0031C[0:5]            5
CLKOUT3_HI_TIME             0x0031C[6:11]           5
CLKOUT3_RSVRD1              0x0031C[12]             1
CLKOUT3_PHASE_MUX           0x0031C[13:15]          0
CLKOUT3_DELAY_TIME          0x00320[0:5]            0
CLKOUT3_NO_COUNT            0x00320[6]              1
CLKOUT3_EDGE                0x00320[7]              1
CLKOUT3_MX                  0x00320[8:9]            0
CLKOUT3_RSVRD               0x00320[11:15]          0
CLKOUT4_LO_TIME             0x00324[0:5]            5
CLKOUT4_HI_TIME             0x00324[6:11]           5
CLKOUT4_RSVRD1              0x00324[12]             1
CLKOUT4_PHASE_MUX           0x00324[13:15]          0
CLKOUT4_DELAY_TIME          0x00328[0:5]            0
CLKOUT4_NO_COUNT            0x00328[6]              1
CLKOUT4_EDGE                0x00328[7]              1
CLKOUT4_MX                  0x00328[8:9]            0
CLKOUT4_RSVRD               0x00328[11:15]          0
CLKOUT5_LO_TIME             0x0032C[0:5]            5
CLKOUT5_HI_TIME             0x0032C[6:11]           5
CLKOUT5_RSVRD1              0x0032C[12]             1
CLKOUT5_PHASE_MUX           0x0032C[13:15]          0
CLKOUT5_DELAY_TIME          0x00330[0:5]            0
CLKOUT5_NO_COUNT            0x00330[6]              1
CLKOUT5_EDGE                0x00330[7]              1
CLKOUT5_MX                  0x00330[8:9]            0
CLKOUT5_RSVRD               0x00330[11]             0
CLKOUT6_LO_TIME             0x00334[0:5]            5
CLKOUT6_HI_TIME             0x00334[6:11]           5
CLKOUT6_RSVRD1              0x00334[12]             1
CLKOUT6_PHASE_MUX           0x00334[13:15]          0
CLKOUT6_DELAY_TIME          0x00338[0:5]            0
CLKOUT6_NO_COUNT            0x00338[6]              1
CLKOUT6_EDGE                0x00338[7]              1
CLKOUT6_MX                  0x00338[8:9]            0
CLKOUT6_RSVRD               0x00338[11]             0
MMCM_DIV_LO_TIME            0x0033C[0:5]            1
MMCM_DIV_HI_TIME            0x0033C[6:11]           1
MMCM_DIV_NO_COUNT           0x0033C[12]             1
MMCM_DIV_EDGE               0x0033C[13]             0
MMCM_DIV_RSVRD              0x00338[14:15]          0
CLKFBOUT_LO_TIME            0x00340[0:5]            5
CLKFBOUT_HI_TIME            0x00340[6:11]           5
CLKFBOUT_RSVRD1             0x00340[12]             1
CLKFBOUT_PHASE_MUX          0x00340[13:15]          0
CLKFBOUT_DELAY_TIME         0x00344[0:5]            0
CLKFBOUT_NO_COUNT           0x00344[6]              1
CLKFBOUT_EDGE               0x00344[7]              1
CLKFBOUT_MX                 0x00344[8:9]            0
CLKFBOUT_FRAC_WF_R          0x00344[10]             0
CLKFBOUT_FRAC_EN            0x00344[11]             0
CLKFBOUT_FRAC               0x00344[12:14]          0
## CLKFBOUT has some fractional settings on CLKOUT6's second register
CLKFBOUT_FRAC_WF_F          0x00338[12]             0
CLKFBOUT_PHASE_MUX_F        0x00338[13:15]          0
MMCM_LOCKREG1               0x348[0:15]             0x03E8
MMCM_LOCKREG2               0x34C[0:15]             0x7001
MMCM_LOCKREG3               0x350[0:15]             0x73E9
MMCM_FILTREG1               0x354[0:15]             0x0800
MMCM_FILTREG2               0x358[0:15]             0x9190
MMCM_LOAD_SEN               0x0035C[0]              0
MMCM_SADDR                  0x0035C[1]              0
IQ_SWAP_ADC_DB0_CHAN0       0x10000[ 0]             0           disable, enable
IQ_SWAP_ADC_DB0_CHAN1       0x10000[ 1]             0           disable, enable
IQ_SWAP_ADC_DB0_CHAN2       0x10000[ 2]             0           disable, enable
IQ_SWAP_ADC_DB0_CHAN3       0x10000[ 3]             0           disable, enable
IQ_SWAP_ADC_DB1_CHAN0       0x10800[ 0]             0           disable, enable
IQ_SWAP_ADC_DB1_CHAN1       0x10800[ 1]             0           disable, enable
IQ_SWAP_ADC_DB1_CHAN2       0x10800[ 2]             0           disable, enable
IQ_SWAP_ADC_DB1_CHAN3       0x10800[ 3]             0           disable, enable
IQ_SWAP_DAC_DB0_CHAN0       0x10000[ 8]             0           disable, enable
IQ_SWAP_DAC_DB0_CHAN1       0x10000[ 9]             0           disable, enable
IQ_SWAP_DAC_DB0_CHAN2       0x10000[10]             0           disable, enable
IQ_SWAP_DAC_DB0_CHAN3       0x10000[11]             0           disable, enable
IQ_SWAP_DAC_DB1_CHAN0       0x10800[ 8]             0           disable, enable
IQ_SWAP_DAC_DB1_CHAN1       0x10800[ 9]             0           disable, enable
IQ_SWAP_DAC_DB1_CHAN2       0x10800[10]             0           disable, enable
IQ_SWAP_DAC_DB1_CHAN3       0x10800[11]             0           disable, enable
## MMCM reset is active low
MMCM_RESET                  0x11000[0]              0           enable, disable
RFDC_DB0_ADC_RESET          0x12000[4]              0           disable, enable
RFDC_DB0_DAC_RESET          0x12000[8]              0           disable, enable
RFDC_DB0_ADC_RESET_DONE     0x12008[7]              ro
RFDC_DB0_DAC_RESET_DONE     0x12008[11]             ro
RFDC_DB1_ADC_RESET          0x12800[4]              0           disable, enable
RFDC_DB1_DAC_RESET          0x12800[8]              0           disable, enable
RFDC_DB1_ADC_RESET_DONE     0x12808[7]              ro
RFDC_DB1_DAC_RESET_DONE     0x12808[11]             ro
## RF Status: Debugging register
STATUS_RFDC_DB0_DAC_TREADY     0x13000[0:1]         ro
STATUS_RFDC_DB0_DAC_TVALID     0x13000[2:3]         ro
STATUS_RFDC_DB0_ADC_I_TREADY   0x13000[6:7]         ro
STATUS_RFDC_DB0_ADC_I_TVALID   0x13000[10:11]       ro
STATUS_RFDC_DB0_ADC_Q_TREADY   0x13000[4:5]         ro
STATUS_RFDC_DB0_ADC_Q_TVALID   0x13000[8:9]         ro
STATUS_USER_DB0_ADC_OUT_TVALID 0x13000[12:13]       ro
STATUS_USER_DB0_ADC_OUT_TREADY 0x13000[14:15]       ro
STATUS_RFDC_DB1_DAC_TREADY     0x13000[16:17]       ro
STATUS_RFDC_DB1_DAC_TVALID     0x13000[18:19]       ro
STATUS_RFDC_DB1_ADC_I_TREADY   0x13000[22:23]       ro
STATUS_RFDC_DB1_ADC_I_TVALID   0x13000[26:27]       ro
STATUS_RFDC_DB1_ADC_Q_TREADY   0x13000[20:21]       ro
STATUS_RFDC_DB1_ADC_Q_TVALID   0x13000[24:25]       ro
STATUS_USER_DB1_ADC_OUT_TVALID 0x13000[28:29]       ro
STATUS_USER_DB1_ADC_OUT_TREADY 0x13000[30:31]       ro
FABRIC_DSP_INFO_DB0_RX_CNT     0x13008[0:3]         ro
FABRIC_DSP_INFO_DB0_TX_CNT     0x13008[4:7]         ro
FABRIC_DSP_INFO_DB1_RX_CNT     0x13008[10:13]       ro
FABRIC_DSP_INFO_DB1_TX_CNT     0x13008[14:17]       ro
FABRIC_DSP_INFO_BW             0x13008[20:31]       ro
CAL_DATA_I                     0x14000[0:15]        0
CAL_DATA_Q                     0x14000[16:31]       0
CAL_ENABLE_DB0_CHAN0           0x14008[0]           0
CAL_ENABLE_DB0_CHAN1           0x14008[1]           0
CAL_ENABLE_DB0_CHAN2           0x14008[2]           0
CAL_ENABLE_DB0_CHAN3           0x14008[3]           0
CAL_ENABLE_DB1_CHAN0           0x14008[4]           0
CAL_ENABLE_DB1_CHAN1           0x14008[5]           0
CAL_ENABLE_DB1_CHAN2           0x14008[6]           0
CAL_ENABLE_DB1_CHAN3           0x14008[7]           0
THRESHOLD_ADC0_BLOCK0_IDX0     0x15000[0]           ro
THRESHOLD_ADC0_BLOCK0_IDX1     0x15000[1]           ro
THRESHOLD_ADC0_BLOCK1_IDX0     0x15000[2]           ro
THRESHOLD_ADC0_BLOCK1_IDX1     0x15000[3]           ro
THRESHOLD_ADC1_BLOCK0_IDX0     0x15000[4]           ro
THRESHOLD_ADC1_BLOCK0_IDX1     0x15000[5]           ro
THRESHOLD_ADC1_BLOCK1_IDX0     0x15000[6]           ro
THRESHOLD_ADC1_BLOCK1_IDX1     0x15000[7]           ro
THRESHOLD_ADC2_BLOCK0_IDX0     0x15000[8]           ro
THRESHOLD_ADC2_BLOCK0_IDX1     0x15000[9]           ro
THRESHOLD_ADC2_BLOCK1_IDX0     0x15000[10]          ro
THRESHOLD_ADC2_BLOCK1_IDX1     0x15000[11]          ro
THRESHOLD_ADC3_BLOCK0_IDX0     0x15000[12]          ro
THRESHOLD_ADC3_BLOCK0_IDX1     0x15000[13]          ro
THRESHOLD_ADC3_BLOCK1_IDX0     0x15000[14]          ro
THRESHOLD_ADC3_BLOCK1_IDX1     0x15000[15]          ro
RF_PLL_ENABLE_DATA_CLK         0x16000[0]           0           enable, disable
RF_PLL_ENABLE_DATA_CLK_x2      0x16000[4]           0           enable, disable
RF_PLL_ENABLE_RF0_CLK          0x16000[8]           0           enable, disable
RF_PLL_ENABLE_RF0_CLK_x2       0x16000[12]          0           enable, disable
RF_PLL_ENABLE_RF1_CLK          0x16000[20]          0           enable, disable
RF_PLL_ENABLE_RF1_CLK_x2       0x16000[24]          0           enable, disable
CLEAR_DATA_CLK_UNLOCKED        0x16000[16]          0           enable, disable
MMCM_LOCKED                    0x16008[20]          ro
ADC_TILEMAP_DB0_CHAN0_TILE     0x17000[0:1]         ro
ADC_TILEMAP_DB0_CHAN0_BLOCK    0x17000[2:3]         ro
ADC_TILEMAP_DB0_CHAN1_TILE     0x17000[4:5]         ro
ADC_TILEMAP_DB0_CHAN1_BLOCK    0x17000[6:7]         ro
ADC_TILEMAP_DB0_CHAN2_TILE     0x17000[8:9]         ro
ADC_TILEMAP_DB0_CHAN2_BLOCK    0x17000[10:11]       ro
ADC_TILEMAP_DB0_CHAN3_TILE     0x17000[12:13]       ro
ADC_TILEMAP_DB0_CHAN3_BLOCK    0x17000[14:15]       ro
ADC_TILEMAP_DB1_CHAN0_TILE     0x17000[16:17]       ro
ADC_TILEMAP_DB1_CHAN0_BLOCK    0x17000[18:19]       ro
ADC_TILEMAP_DB1_CHAN1_TILE     0x17000[20:21]       ro
ADC_TILEMAP_DB1_CHAN1_BLOCK    0x17000[22:23]       ro
ADC_TILEMAP_DB1_CHAN2_TILE     0x17000[24:25]       ro
ADC_TILEMAP_DB1_CHAN2_BLOCK    0x17000[26:27]       ro
ADC_TILEMAP_DB1_CHAN3_TILE     0x17000[28:29]       ro
ADC_TILEMAP_DB1_CHAN3_BLOCK    0x17000[30:31]       ro
DAC_TILEMAP_DB0_CHAN0_TILE     0x17008[0:1]         ro
DAC_TILEMAP_DB0_CHAN0_BLOCK    0x17008[2:3]         ro
DAC_TILEMAP_DB0_CHAN1_TILE     0x17008[4:5]         ro
DAC_TILEMAP_DB0_CHAN1_BLOCK    0x17008[6:7]         ro
DAC_TILEMAP_DB0_CHAN2_TILE     0x17008[8:9]         ro
DAC_TILEMAP_DB0_CHAN2_BLOCK    0x17008[10:11]       ro
DAC_TILEMAP_DB0_CHAN3_TILE     0x17008[12:13]       ro
DAC_TILEMAP_DB0_CHAN3_BLOCK    0x17008[14:15]       ro
DAC_TILEMAP_DB1_CHAN0_TILE     0x17008[16:17]       ro
DAC_TILEMAP_DB1_CHAN0_BLOCK    0x17008[18:19]       ro
DAC_TILEMAP_DB1_CHAN1_TILE     0x17008[20:21]       ro
DAC_TILEMAP_DB1_CHAN1_BLOCK    0x17008[22:23]       ro
DAC_TILEMAP_DB1_CHAN2_TILE     0x17008[24:25]       ro
DAC_TILEMAP_DB1_CHAN2_BLOCK    0x17008[26:27]       ro
DAC_TILEMAP_DB1_CHAN3_TILE     0x17008[28:29]       ro
DAC_TILEMAP_DB1_CHAN3_BLOCK    0x17008[30:31]       ro
RFDC_INFO_DB0_XTRA_RESAMP      0x18000[0:3]         ro
RFDC_INFO_DB0_SPC_RX           0x18000[4:6]         ro
RFDC_INFO_DB0_SPC_TX           0x18000[7:9]         ro
RFDC_INFO_DB1_XTRA_RESAMP      0x18000[16:19]       ro
RFDC_INFO_DB1_SPC_RX           0x18000[20:22]       ro
RFDC_INFO_DB1_SPC_TX           0x18000[23:25]       ro
"""

########################################################################
# Template for python methods in the body of the struct
########################################################################

PY_BODY_TMPL = """\
def get_reg(self, addr):
    '''Return the value of register at address addr'''
    reg = 0
    % for addr in sorted(set(map(lambda r: r.get_addr(), regs))):
    <% if_state = 'if' if loop.index == 0 else 'elif' %>
    ${if_state} addr == ${addr}:
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
        % if reg.get_enums():
        reg |= (self.${reg.get_name()}.value & ${reg.get_mask()}) << ${reg.get_shift()}
        % else:
        reg |= (self.${reg.get_name()} & ${reg.get_mask()}) << ${reg.get_shift()}
        % endif
        % endfor
    % endfor
    return reg

def get_addr(self, reg_name):
    '''returns the address of a register with the given name'''
    return {
        % for reg in regs:
        '${reg.get_name()}': ${reg.get_addr()},
        % endfor
    }[reg_name]

def set_reg(self, addr, reg):
    % for addr in sorted(set(map(lambda r: r.get_addr(), regs))):
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

if __name__ == "__main__":
    import common

    common.generate(
        name="x4xx_rfdc_regmap",
        regs_tmpl=REGS_TMPL,
        body_tmpl="",
        py_body_tmpl=PY_BODY_TMPL,
        file=__file__,
    )
