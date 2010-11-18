/* -*- c -*- */
/*
 * Copyright 2007 Free Software Foundation, Inc.
 * Copyright(c) 1999 - 2005 Intel Corporation. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* Much of this was extracted from the Linux e1000_hw.h file */

#ifndef INCLUDED_ETH_PHY_H
#define INCLUDED_ETH_PHY_H

/* PHY 1000 MII Register/Bit Definitions */
/* PHY Registers defined by IEEE */

#define PHY_CTRL         0x00 /* Control Register */
#define PHY_STATUS       0x01 /* Status Regiser */
#define PHY_ID1          0x02 /* Phy Id Reg (word 1) */
#define PHY_ID2          0x03 /* Phy Id Reg (word 2) */
#define PHY_AUTONEG_ADV  0x04 /* Autoneg Advertisement */
#define PHY_LP_ABILITY   0x05 /* Link Partner Ability (Base Page) */
#define PHY_AUTONEG_EXP  0x06 /* Autoneg Expansion Reg */
#define PHY_NEXT_PAGE_TX 0x07 /* Next Page TX */
#define PHY_LP_NEXT_PAGE 0x08 /* Link Partner Next Page */
#define PHY_1000T_CTRL   0x09 /* 1000Base-T Control Reg */
#define PHY_1000T_STATUS 0x0A /* 1000Base-T Status Reg */
#define PHY_EXT_STATUS   0x0F /* Extended Status Reg */

/* PHY 1000 MII Register additions in ET1011C */
#define PHY_INT_MASK     24
#define PHY_INT_STATUS   25
#define PHY_PHY_STATUS   26
#define PHY_LED2         28

/* Bit definitions for some of the registers above */

/* PHY Control Register (PHY_CTRL) */
#define MII_CR_SPEED_SELECT_MSB 0x0040  /* bits 6,13: 10=1000, 01=100, 00=10 */
#define MII_CR_COLL_TEST_ENABLE 0x0080  /* Collision test enable */
#define MII_CR_FULL_DUPLEX      0x0100  /* FDX =1, half duplex =0 */
#define MII_CR_RESTART_AUTO_NEG 0x0200  /* Restart auto negotiation */
#define MII_CR_ISOLATE          0x0400  /* Isolate PHY from MII */
#define MII_CR_POWER_DOWN       0x0800  /* Power down */
#define MII_CR_AUTO_NEG_EN      0x1000  /* Auto Neg Enable */
#define MII_CR_SPEED_SELECT_LSB 0x2000  /* bits 6,13: 10=1000, 01=100, 00=10 */
#define MII_CR_LOOPBACK         0x4000  /* 0 = normal, 1 = loopback */
#define MII_CR_RESET            0x8000  /* 0 = normal, 1 = PHY reset */

/* PHY Status Register (PHY_STATUS) */
#define MII_SR_EXTENDED_CAPS     0x0001 /* Extended register capabilities */
#define MII_SR_JABBER_DETECT     0x0002 /* Jabber Detected */
#define MII_SR_LINK_STATUS       0x0004 /* Link Status 1 = link */
#define MII_SR_AUTONEG_CAPS      0x0008 /* Auto Neg Capable */
#define MII_SR_REMOTE_FAULT      0x0010 /* Remote Fault Detect */
#define MII_SR_AUTONEG_COMPLETE  0x0020 /* Auto Neg Complete */
#define MII_SR_PREAMBLE_SUPPRESS 0x0040 /* Preamble may be suppressed */
#define MII_SR_EXTENDED_STATUS   0x0100 /* Ext. status info in Reg 0x0F */
#define MII_SR_100T2_HD_CAPS     0x0200 /* 100T2 Half Duplex Capable */
#define MII_SR_100T2_FD_CAPS     0x0400 /* 100T2 Full Duplex Capable */
#define MII_SR_10T_HD_CAPS       0x0800 /* 10T   Half Duplex Capable */
#define MII_SR_10T_FD_CAPS       0x1000 /* 10T   Full Duplex Capable */
#define MII_SR_100X_HD_CAPS      0x2000 /* 100X  Half Duplex Capable */
#define MII_SR_100X_FD_CAPS      0x4000 /* 100X  Full Duplex Capable */
#define MII_SR_100T4_CAPS        0x8000 /* 100T4 Capable */

/* Autoneg Advertisement Register (PHY_AUTONEG_ADV) */
#define NWAY_AR_SELECTOR_FIELD 0x0001   /* indicates IEEE 802.3 CSMA/CD */
#define NWAY_AR_10T_HD_CAPS    0x0020   /* 10T   Half Duplex Capable */
#define NWAY_AR_10T_FD_CAPS    0x0040   /* 10T   Full Duplex Capable */
#define NWAY_AR_100TX_HD_CAPS  0x0080   /* 100TX Half Duplex Capable */
#define NWAY_AR_100TX_FD_CAPS  0x0100   /* 100TX Full Duplex Capable */
#define NWAY_AR_100T4_CAPS     0x0200   /* 100T4 Capable */
#define NWAY_AR_PAUSE          0x0400   /* Pause operation desired */
#define NWAY_AR_ASM_DIR        0x0800   /* Asymmetric Pause Direction bit */
#define NWAY_AR_REMOTE_FAULT   0x2000   /* Remote Fault detected */
#define NWAY_AR_NEXT_PAGE      0x8000   /* Next Page ability supported */

/* Link Partner Ability Register (Base Page) (PHY_LP_ABILITY) */
#define NWAY_LPAR_SELECTOR_FIELD 0x0000 /* LP protocol selector field */
#define NWAY_LPAR_10T_HD_CAPS    0x0020 /* LP is 10T   Half Duplex Capable */
#define NWAY_LPAR_10T_FD_CAPS    0x0040 /* LP is 10T   Full Duplex Capable */
#define NWAY_LPAR_100TX_HD_CAPS  0x0080 /* LP is 100TX Half Duplex Capable */
#define NWAY_LPAR_100TX_FD_CAPS  0x0100 /* LP is 100TX Full Duplex Capable */
#define NWAY_LPAR_100T4_CAPS     0x0200 /* LP is 100T4 Capable */
#define NWAY_LPAR_PAUSE          0x0400 /* LP Pause operation desired */
#define NWAY_LPAR_ASM_DIR        0x0800 /* LP Asymmetric Pause Direction bit */
#define NWAY_LPAR_REMOTE_FAULT   0x2000 /* LP has detected Remote Fault */
#define NWAY_LPAR_ACKNOWLEDGE    0x4000 /* LP has rx'd link code word */
#define NWAY_LPAR_NEXT_PAGE      0x8000 /* Next Page ability supported */

/* Autoneg Expansion Register (PHY_AUTONEG_EXP) */
#define NWAY_ER_LP_NWAY_CAPS      0x0001 /* LP has Auto Neg Capability */
#define NWAY_ER_PAGE_RXD          0x0002 /* LP is 10T   Half Duplex Capable */
#define NWAY_ER_NEXT_PAGE_CAPS    0x0004 /* LP is 10T   Full Duplex Capable */
#define NWAY_ER_LP_NEXT_PAGE_CAPS 0x0008 /* LP is 100TX Half Duplex Capable */
#define NWAY_ER_PAR_DETECT_FAULT  0x0010 /* LP is 100TX Full Duplex Capable */

/* Next Page TX Register (PHY_NEXT_PAGE_TX) */
#define NPTX_MSG_CODE_FIELD 0x0001 /* NP msg code or unformatted data */
#define NPTX_TOGGLE         0x0800 /* Toggles between exchanges
                                    * of different NP
                                    */
#define NPTX_ACKNOWLDGE2    0x1000 /* 1 = will comply with msg
                                    * 0 = cannot comply with msg
                                    */
#define NPTX_MSG_PAGE       0x2000 /* formatted(1)/unformatted(0) pg */
#define NPTX_NEXT_PAGE      0x8000 /* 1 = addition NP will follow
                                    * 0 = sending last NP
                                    */

/* Link Partner Next Page Register (PHY_LP_NEXT_PAGE) */
#define LP_RNPR_MSG_CODE_FIELD 0x0001 /* NP msg code or unformatted data */
#define LP_RNPR_TOGGLE         0x0800 /* Toggles between exchanges
                                       * of different NP
                                       */
#define LP_RNPR_ACKNOWLDGE2    0x1000 /* 1 = will comply with msg
                                       * 0 = cannot comply with msg
                                       */
#define LP_RNPR_MSG_PAGE       0x2000  /* formatted(1)/unformatted(0) pg */
#define LP_RNPR_ACKNOWLDGE     0x4000  /* 1 = ACK / 0 = NO ACK */
#define LP_RNPR_NEXT_PAGE      0x8000  /* 1 = addition NP will follow
                                        * 0 = sending last NP
                                        */

/* 1000BASE-T Control Register (PHY_1000T_CTRL) */
#define CR_1000T_ASYM_PAUSE      0x0080 /* Advertise asymmetric pause bit */
#define CR_1000T_HD_CAPS         0x0100 /* Advertise 1000T HD capability */
#define CR_1000T_FD_CAPS         0x0200 /* Advertise 1000T FD capability  */
#define CR_1000T_REPEATER_DTE    0x0400 /* 1=Repeater/switch device port */
                                        /* 0=DTE device */
#define CR_1000T_MS_VALUE        0x0800 /* 1=Configure PHY as Master */
                                        /* 0=Configure PHY as Slave */
#define CR_1000T_MS_ENABLE       0x1000 /* 1=Master/Slave manual config value */
                                        /* 0=Automatic Master/Slave config */
#define CR_1000T_TEST_MODE_NORMAL 0x0000 /* Normal Operation */
#define CR_1000T_TEST_MODE_1     0x2000 /* Transmit Waveform test */
#define CR_1000T_TEST_MODE_2     0x4000 /* Master Transmit Jitter test */
#define CR_1000T_TEST_MODE_3     0x6000 /* Slave Transmit Jitter test */
#define CR_1000T_TEST_MODE_4     0x8000 /* Transmitter Distortion test */

/* 1000BASE-T Status Register (PHY_1000T_STATUS) */
#define SR_1000T_IDLE_ERROR_CNT   0x00FF /* Num idle errors since last read */
#define SR_1000T_ASYM_PAUSE_DIR   0x0100 /* LP asymmetric pause direction bit */
#define SR_1000T_LP_HD_CAPS       0x0400 /* LP is 1000T HD capable */
#define SR_1000T_LP_FD_CAPS       0x0800 /* LP is 1000T FD capable */
#define SR_1000T_REMOTE_RX_STATUS 0x1000 /* Remote receiver OK */
#define SR_1000T_LOCAL_RX_STATUS  0x2000 /* Local receiver OK */
#define SR_1000T_MS_CONFIG_RES    0x4000 /* 1=Local TX is Master, 0=Slave */
#define SR_1000T_MS_CONFIG_FAULT  0x8000 /* Master/Slave config fault */
#define SR_1000T_REMOTE_RX_STATUS_SHIFT          12
#define SR_1000T_LOCAL_RX_STATUS_SHIFT           13
#define SR_1000T_PHY_EXCESSIVE_IDLE_ERR_COUNT    5
#define FFE_IDLE_ERR_COUNT_TIMEOUT_20            20
#define FFE_IDLE_ERR_COUNT_TIMEOUT_100           100

/* Extended Status Register (PHY_EXT_STATUS) */
#define IEEE_ESR_1000T_HD_CAPS 0x1000 /* 1000T HD capable */
#define IEEE_ESR_1000T_FD_CAPS 0x2000 /* 1000T FD capable */
#define IEEE_ESR_1000X_HD_CAPS 0x4000 /* 1000X HD capable */
#define IEEE_ESR_1000X_FD_CAPS 0x8000 /* 1000X FD capable */

#define PHY_TX_POLARITY_MASK   0x0100 /* register 10h bit 8 (polarity bit) */
#define PHY_TX_NORMAL_POLARITY 0      /* register 10h bit 8 (normal polarity) */

#define AUTO_POLARITY_DISABLE  0x0010 /* register 11h bit 4 */
                                      /* (0=enable, 1=disable) */

/* PHY Status Register (PHY_PHY_STATUS) */
#define PHYSTAT_ASYMMETRIC   (1 << 0)
#define PHYSTAT_PAUSE        (1 << 1)
#define PHYSTAT_AUTONEG_EN   (1 << 2)
#define PHYSTAT_COLLISION    (1 << 3)
#define PHYSTAT_RXSTAT       (1 << 4)
#define PHYSTAT_TXSTAT       (1 << 5)
#define PHYSTAT_LINK         (1 << 6)
#define PHYSTAT_DUPLEX       (1 << 7)
#define PHYSTAT_SPEED_MASK   ((1 << 8) | (1 << 9))
#define PHYSTAT_SPEED_1000   (1 << 9)
#define PHYSTAT_SPEED_100    (1 << 8)
#define PHYSTAT_SPEED_10     0
#define PHYSTAT_POLARITY     (1 << 10)
#define PHYSTAT_MDIX         (1 << 11)
#define PHYSTAT_AUTONEG_STAT (1 << 12)
#define PHYSTAT_STANDBY      (1 << 13)

/* Interrupt status, mask and clear regs (PHY_INT_{STATUS,MASK,CLEAR}) */
#define PHY_INT_ENABLE         (1 << 0)
#define PHY_INT_DOWNSHIFT      (1 << 1)
#define PHY_INT_LINK_STATUS_CHANGE  (1 << 2)
#define PHY_INT_RX_STATUS_CHANGE (1 << 3)
#define PHY_INT_FIFO_ERROR     (1 << 4)
#define PHY_INT_ERR_CTR_FULL   (1 << 5)
#define PHY_INT_NEXT_PAGE_RX   (1 << 6)
#define PHY_INT_CRC_ERROR      (1 << 7)
#define PHY_INT_AUTONEG_STATUS_CHANGE (1 << 8)
#define PHY_INT_MDIO_SYNC_LOST (1 << 9)
#define PHY_INT_TDR_IP_PHONE   (1 << 10)

/* PHY LED status register 2 (used for controlling link LED for activity light) */
#define PHY_LED_TXRX_LSB           12
#define PHY_LED_LINK_LSB           8
#define PHY_LED_100_LSB            4
#define PHY_LED_1000_LSB           0

#define LED_1000                   0
#define LED_100_TX                 1
#define LED_10                     2
#define LED_1000_ON_100_BLINK      3
#define LED_LINK                   4
#define LED_TX                     5
#define LED_RX                     6
#define LED_ACTIVITY               7
#define LED_FULLDUPLEX             8
#define LED_COLLISION              9
#define LED_LINK_ON_ACTIVITY_BLINK 10
#define LED_LINK_ON_RX_BLINK       11
#define LED_FULL_DUPLEX_ON_COLLISION_BLINK 12
#define LED_BLINK                  13
#define LED_ON                     14
#define LED_OFF                    15


#endif /* INCLUDED_ETH_PHY_H */
