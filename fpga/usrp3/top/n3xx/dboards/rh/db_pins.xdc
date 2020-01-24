## TDC : ################################################################################
## Bank 10, 2.5V (DB A)
#########################################################################################

set_property PACKAGE_PIN    AB15        [get_ports {UNUSED_PIN_TDCA_0}]
set_property PACKAGE_PIN    AB14        [get_ports {UNUSED_PIN_TDCA_1}]
set_property PACKAGE_PIN    AB16        [get_ports {UNUSED_PIN_TDCA_2}]
set_property PACKAGE_PIN    AB17        [get_ports {UNUSED_PIN_TDCA_3}]
set_property IOSTANDARD     LVCMOS25    [get_ports {UNUSED_PIN_TDCA_*}]
set_property IOB            TRUE        [get_ports {UNUSED_PIN_TDCA_*}]


## TDC : ################################################################################
## Bank 11, 2.5V (DB B)
#########################################################################################

set_property PACKAGE_PIN    W21         [get_ports {UNUSED_PIN_TDCB_0}]
set_property PACKAGE_PIN    Y21         [get_ports {UNUSED_PIN_TDCB_1}]
set_property PACKAGE_PIN    Y22         [get_ports {UNUSED_PIN_TDCB_2}]
set_property PACKAGE_PIN    Y23         [get_ports {UNUSED_PIN_TDCB_3}]
set_property IOSTANDARD     LVCMOS25    [get_ports {UNUSED_PIN_TDCB_*}]
set_property IOB            TRUE        [get_ports {UNUSED_PIN_TDCB_*}]


## USRP IO A : ##########################################################################
# DBA
#########################################################################################

set_property PACKAGE_PIN    G1          [get_ports DBA_MODULE_PWR_ENABLE]
set_property PACKAGE_PIN    E5          [get_ports DBA_RF_PWR_ENABLE]

set_property PACKAGE_PIN    AF14        [get_ports DBA_FPGA_CLK_P]
set_property PACKAGE_PIN    AG14        [get_ports DBA_FPGA_CLK_N]

set_property PACKAGE_PIN    N8          [get_ports DBA_MGTCLK_P]
set_property PACKAGE_PIN    N7          [get_ports DBA_MGTCLK_N]

set_property PACKAGE_PIN    AG17        [get_ports DBA_FPGA_SYSREF_P]
set_property PACKAGE_PIN    AG16        [get_ports DBA_FPGA_SYSREF_N]
set_property IOB            TRUE        [get_ports DBA_FPGA_SYSREF_*]


# set_property PACKAGE_PIN    AD10        [get_ports NET2CLK_P]
# set_property PACKAGE_PIN    AD9         [get_ports NET2CLK_N]

set_property PACKAGE_PIN    C2          [get_ports DBA_CPLD_PL_SPI_SCLK]
set_property PACKAGE_PIN    B1          [get_ports DBA_TXLO_SPI_CS_B];    # DBA_CPLD_PL_SPI_LE
set_property PACKAGE_PIN    B2          [get_ports DBA_CPLD_PL_SPI_CS_B]; # DBA_CPLD_PL_SPI_ADDR[0]
set_property PACKAGE_PIN    F4          [get_ports DBA_RXLO_SPI_CS_B];    # DBA_CPLD_PL_SPI_ADDR[1]
set_property PACKAGE_PIN    F3          [get_ports DBA_LODIS_SPI_CS_B];   # DBA_CPLD_PL_SPI_ADDR[2]
set_property PACKAGE_PIN    C1          [get_ports DBA_CPLD_PL_SPI_MISO]
set_property PACKAGE_PIN    A3          [get_ports DBA_CPLD_PL_SPI_MOSI]

set_property PACKAGE_PIN    AC16        [get_ports DBA_CPLD_PS_SPI_SCLK]; # DBA_CPLD_PS_SPI_ADDR[0]
set_property PACKAGE_PIN    AE15        [get_ports DBA_CPLD_PS_SPI_CS_B]; # DBA_CPLD_PS_SPI_ADDR[1]
set_property PACKAGE_PIN    AE16        [get_ports DBA_PHDAC_SPI_CS_B];   # DBA_CPLD_PS_SPI_LE
set_property PACKAGE_PIN    AF17        [get_ports DBA_CLKDIS_SPI_CS_B]
set_property PACKAGE_PIN    AK16        [get_ports DBA_ADC_SPI_CS_B];     # DBA_CPLD_UNUSED[12]
set_property PACKAGE_PIN    AJ16        [get_ports DBA_DAC_SPI_CS_B];     # DBA_CPLD_UNUSED[13]
set_property PACKAGE_PIN    AC17        [get_ports DBA_CPLD_PS_SPI_MISO]
set_property PACKAGE_PIN    AF18        [get_ports DBA_CPLD_PS_SPI_MOSI]

set_property PACKAGE_PIN    AH13        [get_ports DBA_CPLD_JTAG_TDI]
set_property PACKAGE_PIN    AH14        [get_ports DBA_CPLD_JTAG_TDO]
set_property PACKAGE_PIN    AE13        [get_ports DBA_CPLD_JTAG_TMS]
set_property PACKAGE_PIN    AF13        [get_ports DBA_CPLD_JTAG_TCK]

set_property PACKAGE_PIN    E1          [get_ports DBA_CLKDIST_SYNC]

set_property PACKAGE_PIN    A2          [get_ports DBA_ATR_TX]
set_property PACKAGE_PIN    E3          [get_ports DBA_ATR_RX]

set_property PACKAGE_PIN    E2          [get_ports DBA_TXRX_SW_CTRL_1]
set_property PACKAGE_PIN    F5          [get_ports DBA_TXRX_SW_CTRL_2]

set_property PACKAGE_PIN    AE12        [get_ports DBA_ADC_SYNCB_P]
set_property PACKAGE_PIN    AF12        [get_ports DBA_ADC_SYNCB_N]
set_property PACKAGE_PIN    AD14        [get_ports DBA_DAC_SYNCB_P]; # Layout swapped, RTL is negated.
set_property PACKAGE_PIN    AD13        [get_ports DBA_DAC_SYNCB_N]; # Layout swapped, RTL is negated.

# This mapping uses the TX pins as the "master" and mimics RX off of them so Vivado
# places the transceivers in the correct places. The mixup in lanes is accounted for
# in the AD9695 and the DAC37J82 crossbar settings.
set_property PACKAGE_PIN    V6          [get_ports DBA_RX_P[0]]
set_property PACKAGE_PIN    V5          [get_ports DBA_RX_N[0]]
set_property PACKAGE_PIN    U4          [get_ports DBA_RX_P[1]]
set_property PACKAGE_PIN    U3          [get_ports DBA_RX_N[1]]
set_property PACKAGE_PIN    T6          [get_ports DBA_RX_P[2]]
set_property PACKAGE_PIN    T5          [get_ports DBA_RX_N[2]]
set_property PACKAGE_PIN    P6          [get_ports DBA_RX_P[3]]
set_property PACKAGE_PIN    P5          [get_ports DBA_RX_N[3]]

set_property PACKAGE_PIN    T2          [get_ports DBA_TX_P[0]]
set_property PACKAGE_PIN    T1          [get_ports DBA_TX_N[0]]
set_property PACKAGE_PIN    R4          [get_ports DBA_TX_P[1]]
set_property PACKAGE_PIN    R3          [get_ports DBA_TX_N[1]]
set_property PACKAGE_PIN    P2          [get_ports DBA_TX_P[2]]
set_property PACKAGE_PIN    P1          [get_ports DBA_TX_N[2]]
set_property PACKAGE_PIN    N4          [get_ports DBA_TX_P[3]]
set_property PACKAGE_PIN    N3          [get_ports DBA_TX_N[3]]

set_property PACKAGE_PIN    AG12        [get_ports DBA_LED_RX]
set_property PACKAGE_PIN    AH12        [get_ports DBA_LED_RX2]
set_property PACKAGE_PIN    AJ13        [get_ports DBA_LED_TX]

# Possibly need to be used. Connected to CPLD.
# set_property PACKAGE_PIN    C4          [get_ports DBA_CPLD_UNUSED[0]]
# set_property PACKAGE_PIN    C3          [get_ports DBA_CPLD_UNUSED[1]]
# set_property PACKAGE_PIN    K1          [get_ports DBA_CPLD_UNUSED[2]]
# set_property PACKAGE_PIN    L1          [get_ports DBA_CPLD_UNUSED[3]]
# set_property PACKAGE_PIN    D1          [get_ports DBA_CPLD_UNUSED[4]]
# set_property PACKAGE_PIN    AE17        [get_ports DBA_CPLD_UNUSED[5]]
# set_property PACKAGE_PIN    AE18        [get_ports DBA_CPLD_UNUSED[6]]
# set_property PACKAGE_PIN    AB12        [get_ports DBA_CPLD_UNUSED[7]]
# set_property PACKAGE_PIN    AC12        [get_ports DBA_CPLD_UNUSED[8]]
# set_property PACKAGE_PIN    AG17        [get_ports DBA_CPLD_UNUSED[9]]
# set_property PACKAGE_PIN    AK12        [get_ports DBA_CPLD_UNUSED[10]]
# set_property PACKAGE_PIN    AK13        [get_ports DBA_CPLD_UNUSED[11]]

set UsrpIoAHpPinsSe    [get_ports {DBA_MODULE_PWR_ENABLE \
                                   DBA_RF_PWR_ENABLE \
                                   DBA_CPLD_PL_SPI_* \
                                   DBA_TXLO_SPI_CS_B \
                                   DBA_RXLO_SPI_CS_B \
                                   DBA_LODIS_SPI_CS_B \
                                   DBA_CLKDIST_SYNC \
                                   DBA_TXRX_SW_CTRL_* \
                                   DBA_ATR_*}]
set_property IOSTANDARD     LVCMOS18    $UsrpIoAHpPinsSe
set_property DRIVE          6           $UsrpIoAHpPinsSe
set_property SLEW           SLOW        $UsrpIoAHpPinsSe

set UsrpIoAHrPinsSeDr4 [get_ports {DBA_LED_* \
                                   DBA_CPLD_JTAG_*}]
set_property IOSTANDARD     LVCMOS25    $UsrpIoAHrPinsSeDr4
set_property DRIVE          4           $UsrpIoAHrPinsSeDr4
set_property SLEW           SLOW        $UsrpIoAHrPinsSeDr4

set UsrpIoAHrPinsSeDr8 [get_ports {DBA_CPLD_PS_SPI_* \
                                   DBA_PHDAC_SPI_CS_B \
                                   DBA_CLKDIS_SPI_CS_B \
                                   DBA_ADC_SPI_CS_B \
                                   DBA_DAC_SPI_CS_B}]
set_property IOSTANDARD     LVCMOS25    $UsrpIoAHrPinsSeDr8
set_property DRIVE          8           $UsrpIoAHrPinsSeDr8
set_property SLEW           SLOW        $UsrpIoAHrPinsSeDr8

set UsrpIoAHrPinsDiff  [get_ports {DBA_ADC_SYNCB_* \
                                   DBA_DAC_SYNCB_* \
                                   DBA_FPGA_CLK_* \
                                   DBA_FPGA_SYSREF_*}]
set_property IOSTANDARD     LVDS_25     $UsrpIoAHrPinsDiff
set_property DIFF_TERM      TRUE        $UsrpIoAHrPinsDiff


## USRP IO B : ##########################################################################
# DBB
#########################################################################################

set_property PACKAGE_PIN    J4          [get_ports DBB_MODULE_PWR_ENABLE]
set_property PACKAGE_PIN    G4          [get_ports DBB_RF_PWR_ENABLE]

set_property PACKAGE_PIN    AG21        [get_ports DBB_FPGA_CLK_P]
set_property PACKAGE_PIN    AH21        [get_ports DBB_FPGA_CLK_N]

set_property PACKAGE_PIN    W8          [get_ports DBB_MGTCLK_P]
set_property PACKAGE_PIN    W7          [get_ports DBB_MGTCLK_N]

set_property PACKAGE_PIN    AE22        [get_ports DBB_FPGA_SYSREF_P]
set_property PACKAGE_PIN    AF22        [get_ports DBB_FPGA_SYSREF_N]
set_property IOB            TRUE        [get_ports DBB_FPGA_SYSREF_*]

set_property PACKAGE_PIN    K6          [get_ports DBB_CPLD_PL_SPI_SCLK]
set_property PACKAGE_PIN    F2          [get_ports DBB_TXLO_SPI_CS_B];    # DBB_CPLD_PL_SPI_LE
set_property PACKAGE_PIN    G2          [get_ports DBB_CPLD_PL_SPI_CS_B]; # DBB_CPLD_PL_SPI_ADDR[0]
set_property PACKAGE_PIN    H4          [get_ports DBB_RXLO_SPI_CS_B];    # DBB_CPLD_PL_SPI_ADDR[1]
set_property PACKAGE_PIN    H3          [get_ports DBB_LODIS_SPI_CS_B];   # DBB_CPLD_PL_SPI_ADDR[2]
set_property PACKAGE_PIN    J6          [get_ports DBB_CPLD_PL_SPI_MISO]
set_property PACKAGE_PIN    D5          [get_ports DBB_CPLD_PL_SPI_MOSI]

set_property PACKAGE_PIN    AG22        [get_ports DBB_CPLD_PS_SPI_SCLK]
set_property PACKAGE_PIN    AD23        [get_ports DBB_CPLD_PS_SPI_CS_B]; # DBB_CPLD_PS_SPI_ADDR[0]
set_property PACKAGE_PIN    AE23        [get_ports DBB_PHDAC_SPI_CS_B];   # DBB_CPLD_PS_SPI_ADDR[1]
set_property PACKAGE_PIN    AB24        [get_ports DBB_CLKDIS_SPI_CS_B];  # DBB_CPLD_PS_SPI_LE
set_property PACKAGE_PIN    AJ23        [get_ports DBB_ADC_SPI_CS_B];     # DBB_CPLD_UNUSED[12]]
set_property PACKAGE_PIN    AJ24        [get_ports DBB_DAC_SPI_CS_B];     # DBB_CPLD_UNUSED[13]]
set_property PACKAGE_PIN    AH22        [get_ports DBB_CPLD_PS_SPI_MISO]
set_property PACKAGE_PIN    AA24        [get_ports DBB_CPLD_PS_SPI_MOSI]

set_property PACKAGE_PIN    AH19        [get_ports DBB_CPLD_JTAG_TDI]
set_property PACKAGE_PIN    AJ19        [get_ports DBB_CPLD_JTAG_TDO]
set_property PACKAGE_PIN    AB21        [get_ports DBB_CPLD_JTAG_TMS]
set_property PACKAGE_PIN    AB22        [get_ports DBB_CPLD_JTAG_TCK]

set_property PACKAGE_PIN    D3          [get_ports DBB_CLKDIST_SYNC]

set_property PACKAGE_PIN    E6          [get_ports DBB_ATR_TX]
set_property PACKAGE_PIN    J5          [get_ports DBB_ATR_RX]

set_property PACKAGE_PIN    K5          [get_ports DBB_TXRX_SW_CTRL_1]
set_property PACKAGE_PIN    G5          [get_ports DBB_TXRX_SW_CTRL_2]

set_property PACKAGE_PIN    AF23        [get_ports DBB_ADC_SYNCB_P]
set_property PACKAGE_PIN    AF24        [get_ports DBB_ADC_SYNCB_N]
set_property PACKAGE_PIN    AD21        [get_ports DBB_DAC_SYNCB_P]
set_property PACKAGE_PIN    AE21        [get_ports DBB_DAC_SYNCB_N]

# This mapping uses the TX pins as the "master" and mimics RX off of them so Vivado
# places the transceivers in the correct places. The mixup in lanes is accounted for
# in the AD9695 and the DAC37J82 crossbar settings.
set_property PACKAGE_PIN    AC4         [get_ports DBB_RX_P[0]]
set_property PACKAGE_PIN    AC3         [get_ports DBB_RX_N[0]]
set_property PACKAGE_PIN    AB6         [get_ports DBB_RX_P[1]]
set_property PACKAGE_PIN    AB5         [get_ports DBB_RX_N[1]]
set_property PACKAGE_PIN    Y6          [get_ports DBB_RX_P[2]]
set_property PACKAGE_PIN    Y5          [get_ports DBB_RX_N[2]]
set_property PACKAGE_PIN    AA4         [get_ports DBB_RX_P[3]]
set_property PACKAGE_PIN    AA3         [get_ports DBB_RX_N[3]]

set_property PACKAGE_PIN    AB2         [get_ports DBB_TX_P[0]]
set_property PACKAGE_PIN    AB1         [get_ports DBB_TX_N[0]]
set_property PACKAGE_PIN    Y2          [get_ports DBB_TX_P[1]]
set_property PACKAGE_PIN    Y1          [get_ports DBB_TX_N[1]]
set_property PACKAGE_PIN    W4          [get_ports DBB_TX_P[2]]
set_property PACKAGE_PIN    W3          [get_ports DBB_TX_N[2]]
set_property PACKAGE_PIN    V2          [get_ports DBB_TX_P[3]]
set_property PACKAGE_PIN    V1          [get_ports DBB_TX_N[3]]

set_property PACKAGE_PIN    AK17        [get_ports DBB_LED_RX]
set_property PACKAGE_PIN    AK18        [get_ports DBB_LED_RX2]
set_property PACKAGE_PIN    AK21        [get_ports DBB_LED_TX]

# Possibly need to be used. Connected to CPLD.
# set_property PACKAGE_PIN    G6          [get_ports DBB_CPLD_UNUSED[0]]
# set_property PACKAGE_PIN    H6          [get_ports DBB_CPLD_UNUSED[1]]
# set_property PACKAGE_PIN    L3          [get_ports DBB_CPLD_UNUSED[2]]
# set_property PACKAGE_PIN    L2          [get_ports DBB_CPLD_UNUSED[3]]
# set_property PACKAGE_PIN    D4          [get_ports DBB_CPLD_UNUSED[4]]
# set_property PACKAGE_PIN    AC22        [get_ports DBB_CPLD_UNUSED[5]]
# set_property PACKAGE_PIN    AC23        [get_ports DBB_CPLD_UNUSED[6]]
# set_property PACKAGE_PIN    AC24        [get_ports DBB_CPLD_UNUSED[7]]
# set_property PACKAGE_PIN    AD24        [get_ports DBB_CPLD_UNUSED[8]]
# set_property PACKAGE_PIN    AE22        [get_ports DBB_CPLD_UNUSED[9]]
# set_property PACKAGE_PIN    AK20        [get_ports DBB_CPLD_UNUSED[10]]
# set_property PACKAGE_PIN    AJ20        [get_ports DBB_CPLD_UNUSED[11]]

set UsrpIoBHpPinsSe    [get_ports {DBB_MODULE_PWR_ENABLE \
                                   DBB_RF_PWR_ENABLE \
                                   DBB_CPLD_PL_SPI_* \
                                   DBB_TXLO_SPI_CS_B \
                                   DBB_RXLO_SPI_CS_B \
                                   DBB_LODIS_SPI_CS_B \
                                   DBB_CLKDIST_SYNC \
                                   DBB_TXRX_SW_CTRL_* \
                                   DBB_ATR_*}]
set_property IOSTANDARD     LVCMOS18    $UsrpIoBHpPinsSe
set_property DRIVE          6           $UsrpIoBHpPinsSe
set_property SLEW           SLOW        $UsrpIoBHpPinsSe

set UsrpIoBHrPinsSeDr4 [get_ports {DBB_LED_* \
                                   DBB_CPLD_JTAG_*}]
set_property IOSTANDARD     LVCMOS25    $UsrpIoBHrPinsSeDr4
set_property DRIVE          4           $UsrpIoBHrPinsSeDr4
set_property SLEW           SLOW        $UsrpIoBHrPinsSeDr4

set UsrpIoBHrPinsSeDr8 [get_ports {DBB_CPLD_PS_SPI_* \
                                   DBB_PHDAC_SPI_CS_B \
                                   DBB_CLKDIS_SPI_CS_B \
                                   DBB_ADC_SPI_CS_B \
                                   DBB_DAC_SPI_CS_B}]
set_property IOSTANDARD     LVCMOS25    $UsrpIoBHrPinsSeDr8
set_property DRIVE          8           $UsrpIoBHrPinsSeDr8
set_property SLEW           SLOW        $UsrpIoBHrPinsSeDr8

set UsrpIoBHrPinsDiff [get_ports {DBB_ADC_SYNCB_* \
                                  DBB_DAC_SYNCB_* \
                                  DBB_FPGA_CLK_* \
                                  DBB_FPGA_SYSREF_*}]
set_property IOSTANDARD     LVDS_25     $UsrpIoBHrPinsDiff
set_property DIFF_TERM      TRUE        $UsrpIoBHrPinsDiff
