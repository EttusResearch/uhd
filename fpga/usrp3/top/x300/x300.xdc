#
# Copyright 2014 Ettus Research LLC
#

#*******************************************************************************
# X3x0 Pin Mapping
#*******************************************************************************

#*******************************************************************************
## SFP Lanes

# SFP clock pins now come from their own ucf files.  See _10ge.ucf, _1ge.ucf, and _cpri.ucf
# NOTE: In the schematic SFP0 signals are prefixed SFP1 and SFP1 signals are prefixed SFP2
set_property PACKAGE_PIN   AA3      [get_ports SFP0_RX_n]
set_property PACKAGE_PIN   AA4      [get_ports SFP0_RX_p]
# set_property IOSTANDARD    LVDS     [get_ports {SFP0_RX_*}]

set_property PACKAGE_PIN   Y1       [get_ports SFP0_TX_n]
set_property PACKAGE_PIN   Y2       [get_ports SFP0_TX_p]
# set_property IOSTANDARD    LVDS     [get_ports {SFP0_TX_*}]

set_property PACKAGE_PIN   T5       [get_ports SFP1_RX_n]
set_property PACKAGE_PIN   T6       [get_ports SFP1_RX_p]
# set_property IOSTANDARD    LVDS     [get_ports {SFP1_RX_*}]

set_property PACKAGE_PIN   P1       [get_ports SFP1_TX_n]
set_property PACKAGE_PIN   P2       [get_ports SFP1_TX_p]
# set_property IOSTANDARD    LVDS     [get_ports {SFP1_TX_*}]

#*******************************************************************************
## ADC 0

set_property PACKAGE_PIN   L27      [get_ports DB0_ADC_DA0_N]
set_property PACKAGE_PIN   L26      [get_ports DB0_ADC_DA0_P]
set_property PACKAGE_PIN   K29      [get_ports DB0_ADC_DA1_N]
set_property PACKAGE_PIN   K28      [get_ports DB0_ADC_DA1_P]
set_property PACKAGE_PIN   L28      [get_ports DB0_ADC_DA2_N]
set_property PACKAGE_PIN   M28      [get_ports DB0_ADC_DA2_P]
set_property PACKAGE_PIN   C30      [get_ports DB0_ADC_DA3_N]   ;# In 3.3V bank
set_property PACKAGE_PIN   D29      [get_ports DB0_ADC_DA3_P]   ;# In 3.3V bank
set_property PACKAGE_PIN   J24      [get_ports DB0_ADC_DA4_N]
set_property PACKAGE_PIN   J23      [get_ports DB0_ADC_DA4_P]
set_property PACKAGE_PIN   L23      [get_ports DB0_ADC_DA5_N]
set_property PACKAGE_PIN   L22      [get_ports DB0_ADC_DA5_P]
set_property PACKAGE_PIN   K24      [get_ports DB0_ADC_DA6_N]
set_property PACKAGE_PIN   K23      [get_ports DB0_ADC_DA6_P]
set_property IOSTANDARD    LVDS_25  [get_ports {DB0_ADC_DA*}]

set_property DIFF_TERM     TRUE     [get_ports {DB0_ADC_DA*}]
# Bit 3 is in the 3.3V bank and does no support diff termination
set_property DIFF_TERM     FALSE    [get_ports DB0_ADC_DA3_*]

set_property PACKAGE_PIN   K21      [get_ports DB0_ADC_DB0_N]
set_property PACKAGE_PIN   L21      [get_ports DB0_ADC_DB0_P]
set_property PACKAGE_PIN   J22      [get_ports DB0_ADC_DB1_N]
set_property PACKAGE_PIN   J21      [get_ports DB0_ADC_DB1_P]
set_property PACKAGE_PIN   L20      [get_ports DB0_ADC_DB2_N]
set_property PACKAGE_PIN   M20      [get_ports DB0_ADC_DB2_P]
set_property PACKAGE_PIN   H29      [get_ports DB0_ADC_DB3_N]
set_property PACKAGE_PIN   J29      [get_ports DB0_ADC_DB3_P]
set_property PACKAGE_PIN   J28      [get_ports DB0_ADC_DB4_N]
set_property PACKAGE_PIN   J27      [get_ports DB0_ADC_DB4_P]
set_property PACKAGE_PIN   K30      [get_ports DB0_ADC_DB5_N]
set_property PACKAGE_PIN   L30      [get_ports DB0_ADC_DB5_P]
set_property PACKAGE_PIN   J26      [get_ports DB0_ADC_DB6_N]
set_property PACKAGE_PIN   K26      [get_ports DB0_ADC_DB6_P]
set_property IOSTANDARD    LVDS_25  [get_ports {DB0_ADC_DB*}]

set_property DIFF_TERM     TRUE     [get_ports {DB0_ADC_DB*}]

set_property PACKAGE_PIN   K25      [get_ports DB0_ADC_DCLK_N]
set_property PACKAGE_PIN   L25      [get_ports DB0_ADC_DCLK_P]
set_property IOSTANDARD    LVDS_25  [get_ports {DB0_ADC_DCLK_*}]

#*******************************************************************************
## ADC 1

set_property PACKAGE_PIN   D18      [get_ports DB1_ADC_DA0_N]
set_property PACKAGE_PIN   D17      [get_ports DB1_ADC_DA0_P]
set_property PACKAGE_PIN   D19      [get_ports DB1_ADC_DA1_N]
set_property PACKAGE_PIN   E19      [get_ports DB1_ADC_DA1_P]
set_property PACKAGE_PIN   L18      [get_ports DB1_ADC_DA2_N]
set_property PACKAGE_PIN   L17      [get_ports DB1_ADC_DA2_P]
set_property PACKAGE_PIN   J13      [get_ports DB1_ADC_DA3_N]   ;# In 3.3V bank
set_property PACKAGE_PIN   K13      [get_ports DB1_ADC_DA3_P]   ;# In 3.3V bank
set_property PACKAGE_PIN   H17      [get_ports DB1_ADC_DA4_N]
set_property PACKAGE_PIN   J17      [get_ports DB1_ADC_DA4_P]
set_property PACKAGE_PIN   F18      [get_ports DB1_ADC_DA5_N]
set_property PACKAGE_PIN   G18      [get_ports DB1_ADC_DA5_P]
set_property PACKAGE_PIN   H19      [get_ports DB1_ADC_DA6_N]
set_property PACKAGE_PIN   J19      [get_ports DB1_ADC_DA6_P]
set_property IOSTANDARD    LVDS_25  [get_ports {DB1_ADC_DA*}]

set_property DIFF_TERM     TRUE     [get_ports {DB1_ADC_DA*}]
# Bit 3 is in the 3.3V bank and does no support diff termination
set_property DIFF_TERM     FALSE    [get_ports DB1_ADC_DA3_*]

set_property PACKAGE_PIN   J18      [get_ports DB1_ADC_DB0_N]
set_property PACKAGE_PIN   K18      [get_ports DB1_ADC_DB0_P]
set_property PACKAGE_PIN   C21      [get_ports DB1_ADC_DB1_N]
set_property PACKAGE_PIN   D21      [get_ports DB1_ADC_DB1_P]
set_property PACKAGE_PIN   K20      [get_ports DB1_ADC_DB2_N]
set_property PACKAGE_PIN   K19      [get_ports DB1_ADC_DB2_P]
set_property PACKAGE_PIN   F22      [get_ports DB1_ADC_DB3_N]
set_property PACKAGE_PIN   G22      [get_ports DB1_ADC_DB3_P]
set_property PACKAGE_PIN   G20      [get_ports DB1_ADC_DB4_N]
set_property PACKAGE_PIN   H20      [get_ports DB1_ADC_DB4_P]
set_property PACKAGE_PIN   C22      [get_ports DB1_ADC_DB5_N]
set_property PACKAGE_PIN   D22      [get_ports DB1_ADC_DB5_P]
set_property PACKAGE_PIN   H22      [get_ports DB1_ADC_DB6_N]
set_property PACKAGE_PIN   H21      [get_ports DB1_ADC_DB6_P]
set_property IOSTANDARD    LVDS_25  [get_ports {DB1_ADC_DB*}]

set_property DIFF_TERM     TRUE     [get_ports {DB1_ADC_DB*}]

set_property PACKAGE_PIN   E20      [get_ports DB1_ADC_DCLK_N]
set_property PACKAGE_PIN   F20      [get_ports DB1_ADC_DCLK_P]
set_property IOSTANDARD    LVDS_25  [get_ports {DB1_ADC_DCLK_*}]

#*******************************************************************************
## DAC 0

set_property PACKAGE_PIN   M30      [get_ports DB0_DAC_D0_N]
set_property PACKAGE_PIN   M29      [get_ports DB0_DAC_D0_P]
set_property PACKAGE_PIN   M27      [get_ports DB0_DAC_D1_N]
set_property PACKAGE_PIN   N27      [get_ports DB0_DAC_D1_P]
set_property PACKAGE_PIN   N30      [get_ports DB0_DAC_D2_N]
set_property PACKAGE_PIN   N29      [get_ports DB0_DAC_D2_P]
set_property PACKAGE_PIN   N26      [get_ports DB0_DAC_D3_N]
set_property PACKAGE_PIN   N25      [get_ports DB0_DAC_D3_P]
set_property PACKAGE_PIN   N20      [get_ports DB0_DAC_D4_N]
set_property PACKAGE_PIN   N19      [get_ports DB0_DAC_D4_P]
set_property PACKAGE_PIN   N22      [get_ports DB0_DAC_D5_N]
set_property PACKAGE_PIN   N21      [get_ports DB0_DAC_D5_P]
set_property PACKAGE_PIN   N24      [get_ports DB0_DAC_D6_N]
set_property PACKAGE_PIN   P23      [get_ports DB0_DAC_D6_P]
set_property PACKAGE_PIN   P22      [get_ports DB0_DAC_D7_N]
set_property PACKAGE_PIN   P21      [get_ports DB0_DAC_D7_P]
set_property IOSTANDARD    LVDS_25  [get_ports {DB0_DAC_D*}]

set_property PACKAGE_PIN   M23      [get_ports DB0_DAC_DCI_N]
set_property PACKAGE_PIN   M22      [get_ports DB0_DAC_DCI_P]
set_property IOSTANDARD    LVDS_25  [get_ports {DB0_DAC_DCI_*}]

set_property PACKAGE_PIN   M25      [get_ports DB0_DAC_FRAME_N]
set_property PACKAGE_PIN   M24      [get_ports DB0_DAC_FRAME_P]
set_property IOSTANDARD    LVDS_25  [get_ports {DB0_DAC_FRAME_*}]

#*******************************************************************************
## DAC 1

set_property PACKAGE_PIN   B17      [get_ports DB1_DAC_D0_N]
set_property PACKAGE_PIN   C17      [get_ports DB1_DAC_D0_P]
set_property PACKAGE_PIN   F17      [get_ports DB1_DAC_D1_N]
set_property PACKAGE_PIN   G17      [get_ports DB1_DAC_D1_P]
set_property PACKAGE_PIN   A17      [get_ports DB1_DAC_D2_N]
set_property PACKAGE_PIN   A16      [get_ports DB1_DAC_D2_P]
set_property PACKAGE_PIN   A18      [get_ports DB1_DAC_D3_N]
set_property PACKAGE_PIN   B18      [get_ports DB1_DAC_D3_P]
set_property PACKAGE_PIN   A21      [get_ports DB1_DAC_D4_N]
set_property PACKAGE_PIN   A20      [get_ports DB1_DAC_D4_P]
set_property PACKAGE_PIN   B20      [get_ports DB1_DAC_D5_N]
set_property PACKAGE_PIN   C20      [get_ports DB1_DAC_D5_P]
set_property PACKAGE_PIN   A22      [get_ports DB1_DAC_D6_N]
set_property PACKAGE_PIN   B22      [get_ports DB1_DAC_D6_P]
set_property PACKAGE_PIN   B19      [get_ports DB1_DAC_D7_N]
set_property PACKAGE_PIN   C19      [get_ports DB1_DAC_D7_P]
set_property IOSTANDARD    LVDS_25  [get_ports {DB1_DAC_D*}]

set_property PACKAGE_PIN   E21      [get_ports DB1_DAC_DCI_N]
set_property PACKAGE_PIN   F21      [get_ports DB1_DAC_DCI_P]
set_property IOSTANDARD    LVDS_25  [get_ports {DB1_DAC_DCI_*}]

set_property PACKAGE_PIN   C16      [get_ports DB1_DAC_FRAME_N]
set_property PACKAGE_PIN   D16      [get_ports DB1_DAC_FRAME_P]
set_property IOSTANDARD    LVDS_25  [get_ports {DB1_DAC_FRAME_*}]

#*******************************************************************************
## DB0 GPIO

set_property PACKAGE_PIN   G25      [get_ports {DB0_RX_IO[0]}]
set_property PACKAGE_PIN   G30      [get_ports {DB0_RX_IO[1]}]
set_property PACKAGE_PIN   H30      [get_ports {DB0_RX_IO[2]}]
set_property PACKAGE_PIN   H27      [get_ports {DB0_RX_IO[3]}]
set_property PACKAGE_PIN   H26      [get_ports {DB0_RX_IO[4]}]
set_property PACKAGE_PIN   F30      [get_ports {DB0_RX_IO[5]}]
set_property PACKAGE_PIN   G29      [get_ports {DB0_RX_IO[6]}]
set_property PACKAGE_PIN   F27      [get_ports {DB0_RX_IO[7]}]
set_property PACKAGE_PIN   G27      [get_ports {DB0_RX_IO[8]}]
set_property PACKAGE_PIN   F28      [get_ports {DB0_RX_IO[9]}]
set_property PACKAGE_PIN   G28      [get_ports {DB0_RX_IO[10]}]
set_property PACKAGE_PIN   H25      [get_ports {DB0_RX_IO[11]}]
set_property PACKAGE_PIN   H24      [get_ports {DB0_RX_IO[12]}]
set_property PACKAGE_PIN   E30      [get_ports {DB0_RX_IO[13]}]
set_property PACKAGE_PIN   E29      [get_ports {DB0_RX_IO[14]}]
set_property PACKAGE_PIN   A30      [get_ports {DB0_RX_IO[15]}]
set_property IOSTANDARD    LVCMOS33 [get_ports {DB0_RX_IO*}]

set_property PACKAGE_PIN   B25      [get_ports {DB0_TX_IO[0]}]
set_property PACKAGE_PIN   C25      [get_ports {DB0_TX_IO[1]}]
set_property PACKAGE_PIN   C26      [get_ports {DB0_TX_IO[2]}]
set_property PACKAGE_PIN   D26      [get_ports {DB0_TX_IO[3]}]
set_property PACKAGE_PIN   A26      [get_ports {DB0_TX_IO[4]}]
set_property PACKAGE_PIN   A25      [get_ports {DB0_TX_IO[5]}]
set_property PACKAGE_PIN   A28      [get_ports {DB0_TX_IO[6]}]
set_property PACKAGE_PIN   B28      [get_ports {DB0_TX_IO[7]}]
set_property PACKAGE_PIN   B24      [get_ports {DB0_TX_IO[8]}]
set_property PACKAGE_PIN   C24      [get_ports {DB0_TX_IO[9]}]
set_property PACKAGE_PIN   A27      [get_ports {DB0_TX_IO[10]}]
set_property PACKAGE_PIN   B27      [get_ports {DB0_TX_IO[11]}]
set_property PACKAGE_PIN   G24      [get_ports {DB0_TX_IO[12]}]
set_property PACKAGE_PIN   G23      [get_ports {DB0_TX_IO[13]}]
set_property PACKAGE_PIN   E26      [get_ports {DB0_TX_IO[14]}]
set_property PACKAGE_PIN   F26      [get_ports {DB0_TX_IO[15]}]
set_property IOSTANDARD    LVCMOS33 [get_ports {DB0_TX_IO*}]

#*******************************************************************************
## DB1 GPIO

set_property PACKAGE_PIN   F16      [get_ports {DB1_RX_IO[0]}]
set_property PACKAGE_PIN   A15      [get_ports {DB1_RX_IO[1]}]
set_property PACKAGE_PIN   B14      [get_ports {DB1_RX_IO[2]}]
set_property PACKAGE_PIN   B15      [get_ports {DB1_RX_IO[3]}]
set_property PACKAGE_PIN   C15      [get_ports {DB1_RX_IO[4]}]
set_property PACKAGE_PIN   A13      [get_ports {DB1_RX_IO[5]}]
set_property PACKAGE_PIN   B13      [get_ports {DB1_RX_IO[6]}]
set_property PACKAGE_PIN   C14      [get_ports {DB1_RX_IO[7]}]
set_property PACKAGE_PIN   D14      [get_ports {DB1_RX_IO[8]}]
set_property PACKAGE_PIN   E15      [get_ports {DB1_RX_IO[9]}]
set_property PACKAGE_PIN   E14      [get_ports {DB1_RX_IO[10]}]
set_property PACKAGE_PIN   E16      [get_ports {DB1_RX_IO[11]}]
set_property PACKAGE_PIN   F15      [get_ports {DB1_RX_IO[12]}]
set_property PACKAGE_PIN   C11      [get_ports {DB1_RX_IO[13]}]
set_property PACKAGE_PIN   D11      [get_ports {DB1_RX_IO[14]}]
set_property PACKAGE_PIN   A12      [get_ports {DB1_RX_IO[15]}]
set_property IOSTANDARD    LVCMOS33 [get_ports {DB1_RX_IO*}]

set_property PACKAGE_PIN   F13      [get_ports {DB1_TX_IO[0]}]
set_property PACKAGE_PIN   G13      [get_ports {DB1_TX_IO[1]}]
set_property PACKAGE_PIN   G14      [get_ports {DB1_TX_IO[2]}]
set_property PACKAGE_PIN   H14      [get_ports {DB1_TX_IO[3]}]
set_property PACKAGE_PIN   H12      [get_ports {DB1_TX_IO[4]}]
set_property PACKAGE_PIN   H11      [get_ports {DB1_TX_IO[5]}]
set_property PACKAGE_PIN   H16      [get_ports {DB1_TX_IO[6]}]
set_property PACKAGE_PIN   J16      [get_ports {DB1_TX_IO[7]}]
set_property PACKAGE_PIN   J12      [get_ports {DB1_TX_IO[8]}]
set_property PACKAGE_PIN   J11      [get_ports {DB1_TX_IO[9]}]
set_property PACKAGE_PIN   G15      [get_ports {DB1_TX_IO[10]}]
set_property PACKAGE_PIN   H15      [get_ports {DB1_TX_IO[11]}]
set_property PACKAGE_PIN   K11      [get_ports {DB1_TX_IO[12]}]
set_property PACKAGE_PIN   L11      [get_ports {DB1_TX_IO[13]}]
set_property PACKAGE_PIN   J14      [get_ports {DB1_TX_IO[14]}]
set_property PACKAGE_PIN   K14      [get_ports {DB1_TX_IO[15]}]
set_property IOSTANDARD    LVCMOS33 [get_ports {DB1_TX_IO*}]

#*******************************************************************************
## DB0 SPI

set_property PACKAGE_PIN   AE14     [get_ports DB0_DAC_SEN]
set_property IOSTANDARD    LVCMOS18 [get_ports DB0_DAC_SEN]

set_property PACKAGE_PIN   B29      [get_ports DB0_ADC_SEN]
set_property IOSTANDARD    LVCMOS33 [get_ports DB0_ADC_SEN]

set_property PACKAGE_PIN   E24      [get_ports DB0_MOSI]
set_property IOSTANDARD    LVCMOS33 [get_ports DB0_MOSI]

set_property PACKAGE_PIN   C27      [get_ports DB0_RX_LSADC_MISO]
set_property IOSTANDARD    LVCMOS33 [get_ports DB0_RX_LSADC_MISO]

set_property PACKAGE_PIN   E28      [get_ports DB0_RX_LSADC_SEN]
set_property IOSTANDARD    LVCMOS33 [get_ports DB0_RX_LSADC_SEN]

set_property PACKAGE_PIN   D27      [get_ports DB0_RX_LSDAC_SEN]
set_property IOSTANDARD    LVCMOS33 [get_ports DB0_RX_LSDAC_SEN]

set_property PACKAGE_PIN   C29      [get_ports DB0_RX_MISO]
set_property IOSTANDARD    LVCMOS33 [get_ports DB0_RX_MISO]

set_property PACKAGE_PIN   D28      [get_ports DB0_RX_SEN]
set_property IOSTANDARD    LVCMOS33 [get_ports DB0_RX_SEN]

set_property PACKAGE_PIN   D24      [get_ports DB0_SCLK]
set_property IOSTANDARD    LVCMOS33 [get_ports DB0_SCLK]

set_property PACKAGE_PIN   B23      [get_ports DB0_TX_LSADC_MISO]
set_property IOSTANDARD    LVCMOS33 [get_ports DB0_TX_LSADC_MISO]

set_property PACKAGE_PIN   A23      [get_ports DB0_TX_LSADC_SEN]
set_property IOSTANDARD    LVCMOS33 [get_ports DB0_TX_LSADC_SEN]

set_property PACKAGE_PIN   F23      [get_ports DB0_TX_LSDAC_SEN]
set_property IOSTANDARD    LVCMOS33 [get_ports DB0_TX_LSDAC_SEN]

set_property PACKAGE_PIN   E23      [get_ports DB0_TX_MISO]
set_property IOSTANDARD    LVCMOS33 [get_ports DB0_TX_MISO]

set_property PACKAGE_PIN   D23      [get_ports DB0_TX_SEN]
set_property IOSTANDARD    LVCMOS33 [get_ports DB0_TX_SEN]

#*******************************************************************************
# DB1 SPI

set_property PACKAGE_PIN   AE15     [get_ports DB1_DAC_SEN]
set_property IOSTANDARD    LVCMOS18 [get_ports DB1_DAC_SEN]

set_property PACKAGE_PIN   B12      [get_ports DB1_ADC_SEN]
set_property IOSTANDARD    LVCMOS33 [get_ports DB1_ADC_SEN]

set_property PACKAGE_PIN   L12      [get_ports DB1_MOSI]
set_property IOSTANDARD    LVCMOS33 [get_ports DB1_MOSI]

set_property PACKAGE_PIN   D13      [get_ports DB1_RX_LSADC_MISO]
set_property IOSTANDARD    LVCMOS33 [get_ports DB1_RX_LSADC_MISO]

set_property PACKAGE_PIN   F12      [get_ports DB1_RX_LSADC_SEN]
set_property IOSTANDARD    LVCMOS33 [get_ports DB1_RX_LSADC_SEN]

set_property PACKAGE_PIN   D12      [get_ports DB1_RX_LSDAC_SEN]
set_property IOSTANDARD    LVCMOS33 [get_ports DB1_RX_LSDAC_SEN]

set_property PACKAGE_PIN   C12      [get_ports DB1_RX_MISO]
set_property IOSTANDARD    LVCMOS33 [get_ports DB1_RX_MISO]

set_property PACKAGE_PIN   E13      [get_ports DB1_RX_SEN]
set_property IOSTANDARD    LVCMOS33 [get_ports DB1_RX_SEN]

set_property PACKAGE_PIN   L13      [get_ports DB1_SCLK]
set_property IOSTANDARD    LVCMOS33 [get_ports DB1_SCLK]

set_property PACKAGE_PIN   L16      [get_ports DB1_TX_LSADC_MISO]
set_property IOSTANDARD    LVCMOS33 [get_ports DB1_TX_LSADC_MISO]

set_property PACKAGE_PIN   K16      [get_ports DB1_TX_LSADC_SEN]
set_property IOSTANDARD    LVCMOS33 [get_ports DB1_TX_LSADC_SEN]

set_property PACKAGE_PIN   G12      [get_ports DB1_TX_LSDAC_SEN]
set_property IOSTANDARD    LVCMOS33 [get_ports DB1_TX_LSDAC_SEN]

set_property PACKAGE_PIN   L15      [get_ports DB1_TX_MISO]
set_property IOSTANDARD    LVCMOS33 [get_ports DB1_TX_MISO]

set_property PACKAGE_PIN   K15      [get_ports DB1_TX_SEN]
set_property IOSTANDARD    LVCMOS33 [get_ports DB1_TX_SEN]

#*******************************************************************************
## DB Misc

set_property PACKAGE_PIN   AB15     [get_ports DB_DAC_MOSI]
set_property IOSTANDARD    LVCMOS18 [get_ports DB_DAC_MOSI]

set_property PACKAGE_PIN   AA15     [get_ports DB_DAC_SCLK]
set_property IOSTANDARD    LVCMOS18 [get_ports DB_DAC_SCLK]

set_property PACKAGE_PIN   F25      [get_ports DB_SCL]
set_property IOSTANDARD    LVCMOS33 [get_ports DB_SCL]

set_property PACKAGE_PIN   E25      [get_ports DB_SDA]
set_property IOSTANDARD    LVCMOS33 [get_ports DB_SDA]

set_property PACKAGE_PIN   B30      [get_ports DB_ADC_RESET]
set_property IOSTANDARD    LVCMOS33 [get_ports DB_ADC_RESET]

set_property PACKAGE_PIN   AC16     [get_ports DB_DAC_RESET]
set_property IOSTANDARD    LVCMOS18 [get_ports DB_DAC_RESET]

set_property PACKAGE_PIN   AC14     [get_ports DB0_DAC_ENABLE]
set_property IOSTANDARD    LVCMOS18 [get_ports DB0_DAC_ENABLE]

set_property PACKAGE_PIN   AC15     [get_ports DB1_DAC_ENABLE]
set_property IOSTANDARD    LVCMOS18 [get_ports DB1_DAC_ENABLE]

#*******************************************************************************
## STC3 Pin Mapping

set_property PACKAGE_PIN AC27  [get_ports aIoResetIn_n]
set_property IOSTANDARD  LVTTL [get_ports aIoResetIn_n]

set_property PACKAGE_PIN AH29  [get_ports aIoReadyIn]
set_property IOSTANDARD  LVTTL [get_ports aIoReadyIn]

set_property PACKAGE_PIN AE28  [get_ports aIoReadyOut]
set_property IOSTANDARD  LVTTL [get_ports aIoReadyOut]

set_property PACKAGE_PIN AD18  [get_ports IoRxClock]
set_property PACKAGE_PIN AE18  [get_ports IoRxClock_n]
set_property IOSTANDARD  LVDS  [get_ports {IoRxClock*}]
set_property DIFF_TERM   TRUE  [get_ports {IoRxClock*}]

set_property PACKAGE_PIN AK18  [get_ports {irIoRxData_n[0]}]
set_property PACKAGE_PIN AJ17  [get_ports {irIoRxData_n[1]}]
set_property PACKAGE_PIN AK19  [get_ports {irIoRxData_n[2]}]
set_property PACKAGE_PIN AG14  [get_ports {irIoRxData_n[3]}]
set_property PACKAGE_PIN AH15  [get_ports {irIoRxData_n[4]}]
set_property PACKAGE_PIN Y18   [get_ports {irIoRxData_n[5]}]
set_property PACKAGE_PIN AG17  [get_ports {irIoRxData_n[6]}]
set_property PACKAGE_PIN AE19  [get_ports {irIoRxData_n[7]}]
set_property PACKAGE_PIN AC17  [get_ports {irIoRxData_n[8]}]
set_property PACKAGE_PIN AD16  [get_ports {irIoRxData_n[9]}]
set_property PACKAGE_PIN AJ16  [get_ports {irIoRxData_n[10]}]
set_property PACKAGE_PIN AC19  [get_ports {irIoRxData_n[11]}]
set_property PACKAGE_PIN AB18  [get_ports {irIoRxData_n[12]}]
set_property PACKAGE_PIN AF16  [get_ports {irIoRxData_n[13]}]
set_property PACKAGE_PIN AH19  [get_ports {irIoRxData_n[14]}]
set_property PACKAGE_PIN AK15  [get_ports {irIoRxData_n[15]}]

set_property PACKAGE_PIN AJ18  [get_ports {irIoRxData[0]}]
set_property PACKAGE_PIN AH17  [get_ports {irIoRxData[1]}]
set_property PACKAGE_PIN AJ19  [get_ports {irIoRxData[2]}]
set_property PACKAGE_PIN AF15  [get_ports {irIoRxData[3]}]
set_property PACKAGE_PIN AG15  [get_ports {irIoRxData[4]}]
set_property PACKAGE_PIN Y19   [get_ports {irIoRxData[5]}]
set_property PACKAGE_PIN AF17  [get_ports {irIoRxData[6]}]
set_property PACKAGE_PIN AD19  [get_ports {irIoRxData[7]}]
set_property PACKAGE_PIN AB17  [get_ports {irIoRxData[8]}]
set_property PACKAGE_PIN AD17  [get_ports {irIoRxData[9]}]
set_property PACKAGE_PIN AH16  [get_ports {irIoRxData[10]}]
set_property PACKAGE_PIN AB19  [get_ports {irIoRxData[11]}]
set_property PACKAGE_PIN AA18  [get_ports {irIoRxData[12]}]
set_property PACKAGE_PIN AE16  [get_ports {irIoRxData[13]}]
set_property PACKAGE_PIN AG19  [get_ports {irIoRxData[14]}]
set_property PACKAGE_PIN AK16  [get_ports {irIoRxData[15]}]

set_property IOSTANDARD  LVDS  [get_ports {irIoRxData*}]
set_property DIFF_TERM   TRUE  [get_ports {irIoRxData*}]

set_property IOSTANDARD  LVDS  [get_ports {irIoRxHeader*}]
set_property DIFF_TERM   TRUE  [get_ports {irIoRxHeader*}]
set_property PACKAGE_PIN AF18  [get_ports irIoRxHeader]
set_property PACKAGE_PIN AG18  [get_ports irIoRxHeader_n]

set_property IOSTANDARD  LVDS_25 [get_ports {IoTxClock*}]
set_property PACKAGE_PIN AE24    [get_ports IoTxClock_n]
set_property PACKAGE_PIN AD23    [get_ports IoTxClock]

set_property IOSTANDARD  LVDS_25 [get_ports {itIoTxData*}]
set_property PACKAGE_PIN AF25    [get_ports {itIoTxData_n[0]}]
set_property PACKAGE_PIN AC25    [get_ports {itIoTxData_n[1]}]
set_property PACKAGE_PIN AH22    [get_ports {itIoTxData_n[2]}]
set_property PACKAGE_PIN AF21    [get_ports {itIoTxData_n[3]}]
set_property PACKAGE_PIN AD24    [get_ports {itIoTxData_n[4]}]
set_property PACKAGE_PIN AB20    [get_ports {itIoTxData_n[5]}]
set_property PACKAGE_PIN AA21    [get_ports {itIoTxData_n[6]}]
set_property PACKAGE_PIN AH25    [get_ports {itIoTxData_n[7]}]
set_property PACKAGE_PIN AB23    [get_ports {itIoTxData_n[8]}]
set_property PACKAGE_PIN AK25    [get_ports {itIoTxData_n[9]}]
set_property PACKAGE_PIN AK24    [get_ports {itIoTxData_n[10]}]
set_property PACKAGE_PIN AD22    [get_ports {itIoTxData_n[11]}]
set_property PACKAGE_PIN AF23    [get_ports {itIoTxData_n[12]}]
set_property PACKAGE_PIN AE21    [get_ports {itIoTxData_n[13]}]
set_property PACKAGE_PIN AC21    [get_ports {itIoTxData_n[14]}]
set_property PACKAGE_PIN AA23    [get_ports {itIoTxData_n[15]}]
set_property PACKAGE_PIN AE25    [get_ports {itIoTxData[0]}]
set_property PACKAGE_PIN AB24    [get_ports {itIoTxData[1]}]
set_property PACKAGE_PIN AG22    [get_ports {itIoTxData[2]}]
set_property PACKAGE_PIN AF20    [get_ports {itIoTxData[3]}]
set_property PACKAGE_PIN AC24    [get_ports {itIoTxData[4]}]
set_property PACKAGE_PIN AA20    [get_ports {itIoTxData[5]}]
set_property PACKAGE_PIN Y21     [get_ports {itIoTxData[6]}]
set_property PACKAGE_PIN AG25    [get_ports {itIoTxData[7]}]
set_property PACKAGE_PIN AB22    [get_ports {itIoTxData[8]}]
set_property PACKAGE_PIN AJ24    [get_ports {itIoTxData[9]}]
set_property PACKAGE_PIN AK23    [get_ports {itIoTxData[10]}]
set_property PACKAGE_PIN AC22    [get_ports {itIoTxData[11]}]
set_property PACKAGE_PIN AE23    [get_ports {itIoTxData[12]}]
set_property PACKAGE_PIN AD21    [get_ports {itIoTxData[13]}]
set_property PACKAGE_PIN AC20    [get_ports {itIoTxData[14]}]
set_property PACKAGE_PIN AA22    [get_ports {itIoTxData[15]}]

set_property IOSTANDARD  LVDS_25 [get_ports {itIoTxHeader*}]
set_property PACKAGE_PIN Y24     [get_ports itIoTxHeader_n]
set_property PACKAGE_PIN Y23     [get_ports itIoTxHeader]

set_property IOSTANDARD  LVTTL   [get_ports aIrq]
set_property PACKAGE_PIN AF28    [get_ports aIrq]

set_property IOSTANDARD  LVCMOS33 [get_ports aIoPort2Restart]
set_property PULLUP      TRUE     [get_ports aIoPort2Restart]
set_property PACKAGE_PIN AE30     [get_ports aIoPort2Restart]

set_property IOSTANDARD  LVCMOS33 [get_ports aStc3Gpio7]
set_property PACKAGE_PIN AF30     [get_ports aStc3Gpio7]

#*******************************************************************************
## Front Panel LEDs

set_property PACKAGE_PIN   AJ26     [get_ports LED_ACT1]
set_property PACKAGE_PIN   AE26     [get_ports LED_ACT2]
set_property PACKAGE_PIN   AF27     [get_ports LED_LINK1]
set_property PACKAGE_PIN   AK26     [get_ports LED_LINK2]
set_property PACKAGE_PIN   AF26     [get_ports LED_PPS]
set_property PACKAGE_PIN   AH27     [get_ports LED_REFLOCK]
set_property PACKAGE_PIN   U30      [get_ports LED_GPSLOCK]
set_property PACKAGE_PIN   V27      [get_ports LED_LINKSTAT]
set_property PACKAGE_PIN   V29      [get_ports LED_LINKACT]
set_property PACKAGE_PIN   AD29     [get_ports LED_RX1_RX]
set_property PACKAGE_PIN   AB30     [get_ports LED_RX2_RX]
set_property PACKAGE_PIN   AA30     [get_ports LED_TXRX1_RX]
set_property PACKAGE_PIN   Y30      [get_ports LED_TXRX1_TX]
set_property PACKAGE_PIN   AB29     [get_ports LED_TXRX2_RX]
set_property PACKAGE_PIN   AE29     [get_ports LED_TXRX2_TX]
set_property IOSTANDARD    LVCMOS33 [get_ports {LED_*}]

#*******************************************************************************
## Front panel GPIO on DB15

set_property PACKAGE_PIN   Y25      [get_ports {FrontPanelGpio[0]}]
set_property PACKAGE_PIN   AD27     [get_ports {FrontPanelGpio[1]}]
set_property PACKAGE_PIN   AD28     [get_ports {FrontPanelGpio[2]}]
set_property PACKAGE_PIN   AG30     [get_ports {FrontPanelGpio[3]}]
set_property PACKAGE_PIN   AH30     [get_ports {FrontPanelGpio[4]}]
set_property PACKAGE_PIN   AC26     [get_ports {FrontPanelGpio[5]}]
set_property PACKAGE_PIN   AD26     [get_ports {FrontPanelGpio[6]}]
set_property PACKAGE_PIN   AJ27     [get_ports {FrontPanelGpio[7]}]
set_property PACKAGE_PIN   AK28     [get_ports {FrontPanelGpio[8]}]
set_property PACKAGE_PIN   AG27     [get_ports {FrontPanelGpio[9]}]
set_property PACKAGE_PIN   AG28     [get_ports {FrontPanelGpio[10]}]
set_property PACKAGE_PIN   AH26     [get_ports {FrontPanelGpio[11]}]
set_property IOSTANDARD    LVCMOS33 [get_ports {FrontPanelGpio*}]
set_property PULLDOWN      TRUE     [get_ports {FrontPanelGpio*}]

#*******************************************************************************
## LMK04816 Clock Control

set_property PACKAGE_PIN   Y26      [get_ports {LMK_Status[0]}]
set_property PACKAGE_PIN   AA26     [get_ports {LMK_Status[1]}]
set_property PACKAGE_PIN   W27      [get_ports LMK_Holdover]
set_property PACKAGE_PIN   W28      [get_ports LMK_Lock]
set_property PACKAGE_PIN   T27      [get_ports LMK_Sync]
set_property PACKAGE_PIN   U19      [get_ports LMK_SEN]
set_property PACKAGE_PIN   Y28      [get_ports LMK_MOSI]
set_property PACKAGE_PIN   AA28     [get_ports LMK_SCLK]
set_property IOSTANDARD    LVCMOS33 [get_ports {LMK_*}]

#*******************************************************************************
# Micrel chip control

set_property PACKAGE_PIN   W29      [get_ports {ClockRefSelect[0]}]
set_property PACKAGE_PIN   Y29      [get_ports {ClockRefSelect[1]}]
set_property IOSTANDARD    LVCMOS33 [get_ports {ClockRefSelect*}]

#*******************************************************************************
## PPS, GPS and Timing

set_property PACKAGE_PIN   AB28     [get_ports GPS_LOCK_OK]
# set_property PACKAGE_PIN  AA25     [get_ports GPS_NMEA_TX]
set_property PACKAGE_PIN   AB25     [get_ports GPS_SER_IN]
set_property PACKAGE_PIN   AC29     [get_ports GPS_SER_OUT]
set_property PACKAGE_PIN   AA27     [get_ports GPS_PPS_OUT]
set_property IOSTANDARD    LVCMOS33 [get_ports {GPS_*}]

set_property PACKAGE_PIN   AC30     [get_ports EXT_PPS_IN]
set_property PACKAGE_PIN   T25      [get_ports EXT_PPS_OUT]
set_property IOSTANDARD    LVCMOS33 [get_ports {EXT_PPS_*}]

#*******************************************************************************
## Clocks

set_property PACKAGE_PIN   AB27     [get_ports FPGA_125MHz_CLK]
set_property IOSTANDARD    LVCMOS33 [get_ports FPGA_125MHz_CLK]

set_property PACKAGE_PIN   AG23     [get_ports FPGA_CLK_n]
set_property IOSTANDARD    LVDS_25  [get_ports FPGA_CLK_n]
set_property DIFF_TERM     TRUE     [get_ports FPGA_CLK_n]

set_property PACKAGE_PIN   AF22     [get_ports FPGA_CLK_p]
set_property IOSTANDARD    LVDS_25  [get_ports FPGA_CLK_p]
set_property DIFF_TERM     TRUE     [get_ports FPGA_CLK_p]

set_property PACKAGE_PIN   E11      [get_ports GPSDO_PWR_ENA]
set_property IOSTANDARD    LVCMOS33 [get_ports GPSDO_PWR_ENA]

set_property PACKAGE_PIN   A11      [get_ports TCXO_ENA]
set_property IOSTANDARD    LVCMOS33 [get_ports TCXO_ENA]

set_property PACKAGE_PIN   AG24     [get_ports FPGA_REFCLK_10MHz_p]
set_property IOSTANDARD    LVDS_25  [get_ports FPGA_REFCLK_10MHz_p]

set_property PACKAGE_PIN   AH24     [get_ports FPGA_REFCLK_10MHz_n]
set_property IOSTANDARD    LVDS_25  [get_ports FPGA_REFCLK_10MHz_n]

set_property PACKAGE_PIN   AA17     [get_ports CPRI_CLK_OUT_P]
set_property IOSTANDARD    LVDS     [get_ports CPRI_CLK_OUT_P]

set_property PACKAGE_PIN   AA16     [get_ports CPRI_CLK_OUT_N]
set_property IOSTANDARD    LVDS     [get_ports CPRI_CLK_OUT_N]

#*******************************************************************************
## SFP low-speed IO

set_property PACKAGE_PIN   U24      [get_ports SFPP0_SCL]
set_property IOSTANDARD    LVCMOS33 [get_ports SFPP0_SCL]

set_property PACKAGE_PIN   V22      [get_ports SFPP0_SDA]
set_property IOSTANDARD    LVCMOS33 [get_ports SFPP0_SDA]

set_property PACKAGE_PIN   W22      [get_ports SFPP0_RS0]
set_property IOSTANDARD    LVCMOS33 [get_ports SFPP0_RS0]

set_property PACKAGE_PIN   W19      [get_ports SFPP0_RS1]
set_property IOSTANDARD    LVCMOS33 [get_ports SFPP0_RS1]

set_property PACKAGE_PIN   V21      [get_ports SFPP0_TxDisable]   ;# Open drain output
set_property IOSTANDARD    LVCMOS33 [get_ports SFPP0_TxDisable]

set_property PACKAGE_PIN   V24      [get_ports SFPP0_ModAbs]      ;# (IJB) Should pullup on pcb
set_property IOSTANDARD    LVCMOS33 [get_ports SFPP0_ModAbs]

set_property PACKAGE_PIN   W21      [get_ports SFPP0_RxLOS]       ;# Input
set_property IOSTANDARD    LVCMOS33 [get_ports SFPP0_RxLOS]

set_property PACKAGE_PIN   U23      [get_ports SFPP0_TxFault]     ;# Input
set_property IOSTANDARD    LVCMOS33 [get_ports SFPP0_TxFault]

set_property PACKAGE_PIN   V19      [get_ports SFPP1_SCL]
set_property IOSTANDARD    LVCMOS33 [get_ports SFPP1_SCL]

set_property PACKAGE_PIN   W26      [get_ports SFPP1_SDA]
set_property IOSTANDARD    LVCMOS33 [get_ports SFPP1_SDA]

set_property PACKAGE_PIN   W24      [get_ports SFPP1_RS0]
set_property IOSTANDARD    LVCMOS33 [get_ports SFPP1_RS0]

set_property PACKAGE_PIN   U22      [get_ports SFPP1_RS1]
set_property IOSTANDARD    LVCMOS33 [get_ports SFPP1_RS1]

set_property PACKAGE_PIN   V25      [get_ports SFPP1_TxDisable]   ;# Open drain output
set_property IOSTANDARD    LVCMOS33 [get_ports SFPP1_TxDisable]

set_property PACKAGE_PIN   V20      [get_ports SFPP1_ModAbs]      ;# (IJB) Should pullup on pcb
set_property IOSTANDARD    LVCMOS33 [get_ports SFPP1_ModAbs]

set_property PACKAGE_PIN   W23      [get_ports SFPP1_RxLOS]       ;# Input
set_property IOSTANDARD    LVCMOS33 [get_ports SFPP1_RxLOS]

set_property PACKAGE_PIN   V30      [get_ports SFPP1_TxFault]     ;# Input
set_property IOSTANDARD    LVCMOS33 [get_ports SFPP1_TxFault]

#*******************************************************************************
## Config Flash Interface

set_property PROHIBIT      TRUE     [get_sites {P24}]
set_property PROHIBIT      TRUE     [get_sites {R25}]
set_property PROHIBIT      TRUE     [get_sites {R20}]
set_property PROHIBIT      TRUE     [get_sites {R21}]
set_property PROHIBIT      TRUE     [get_sites {T20}]
set_property PROHIBIT      TRUE     [get_sites {T21}]
set_property PROHIBIT      TRUE     [get_sites {T22}]
set_property PROHIBIT      TRUE     [get_sites {T23}]
set_property PROHIBIT      TRUE     [get_sites {U20}]
set_property PROHIBIT      TRUE     [get_sites {P29}]
set_property PROHIBIT      TRUE     [get_sites {R29}]
set_property PROHIBIT      TRUE     [get_sites {P27}]
set_property PROHIBIT      TRUE     [get_sites {P28}]
set_property PROHIBIT      TRUE     [get_sites {T30}]
set_property PROHIBIT      TRUE     [get_sites {P26}]
set_property PROHIBIT      TRUE     [get_sites {R26}]

#*******************************************************************************
# Miscellaneous

# Expose this pin to work around a silicon bug in Series 7 FPGA where
# race condition with the reading of PUDC during the erase of the FPGA
# image cause glitches on output IO pins
set_property PACKAGE_PIN   R23      [get_ports FPGA_PUDC_B]
set_property IOSTANDARD    LVCMOS33 [get_ports FPGA_PUDC_B]
set_property PULLUP        TRUE     [get_ports FPGA_PUDC_B]


#*******************************************************************************
# UNUSED or DEPOPULATED)

# Security Chip
# set_property PACKAGE_PIN   U27      [get_ports AUTH_SDA]
# set_property IOSTANDARD    LVCMOS33 [get_ports AUTH_SDA]

# set_property PACKAGE_PIN   U28      [get_ports FPGA_RESET_N]
# set_property IOSTANDARD    LVCMOS33 [get_ports FPGA_RESET_N]

# UART, new on Rev B
#NET   uart_tx                IOSTANDARD = LVCMOS33   |   LOC  =  R28;
#NET   uart_rx                IOSTANDARD = LVCMOS33   |   LOC  =  T28;
#NET   uart_wat               IOSTANDARD = LVCMOS33   |   LOC  =  T26;

#NET   aIO_Interrupt_0        IOSTANDARD = LVCMOS33   |   LOC  =  AF28;
#NET   aIO_Interrupt_1        IOSTANDARD = LVCMOS33   |   LOC  =  AK29;
#NET   aStcIO_Reset_n         IOSTANDARD = LVCMOS33   |   LOC  =  AC27;
#NET   0V75_VTT_REF           IOSTANDARD = DDR15      |   LOC  =  AD7;
#NET   0V75_VTT_REF           IOSTANDARD = DDR15      |   LOC  =  AG8;
#NET   XSIG030149             IOSTANDARD = DDR15      |   LOC  =  Y13;
#NET   XSIG030150             IOSTANDARD = DDR15      |   LOC  =  AD13;
#NET   XSIG030154             IOSTANDARD = DDR15      |   LOC  =  AB7;
#NET   XSIG030186             IOSTANDARD = DDR15      |   LOC  =  AC6;
#NET   XSIG051113             IOSTANDARD = LVCMOS25   |   LOC  =  E18;
#NET   XSIG051117             IOSTANDARD = LVCMOS25   |   LOC  =  M19;
#NET   XSIG051118             IOSTANDARD = LVCMOS25   |   LOC  =  P19;
#NET   XSIG051203             IOSTANDARD = LVCMOS25   |   LOC  =  G19;
#NET   XSIG051347             IOSTANDARD = LVCMOS33   |   LOC  =  F11;
#NET   XSIG080308             IOSTANDARD = LVCMOS33   |   LOC  =  R19;
#NET   XSIG080310             IOSTANDARD = LVCMOS33   |   LOC  =  R24;
#NET   XSIG130242             IOSTANDARD = LVCMOS18   |   LOC  =  Y14;
#NET   XSIG130243             IOSTANDARD = LVCMOS18   |   LOC  =  AB14;
#NET   XSIG130258             IOSTANDARD = LVCMOS18   |   LOC  =  AD14;
#NET   XSIG130261             IOSTANDARD = LVCMOS18   |   LOC  =  Y16;
#NET   XSIG130262             IOSTANDARD = LVCMOS18   |   LOC  =  Y15;
#NET   XSIG130263             IOSTANDARD = LVCMOS25   |   LOC  =  AH20;
#NET   XSIG130265             IOSTANDARD = LVCMOS25   |   LOC  =  AG20;

#NET   CPRI_CLK_p            IOSTANDARD = LVDS_25 | LOC  =  U8;
#NET   CPRI_CLK_n            IOSTANDARD = LVDS_25 | LOC  =  U7;
