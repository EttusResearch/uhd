
#ifndef INCLUDED_XGE_PHY_H
#define INCLUDED_XGE_PHY_H

#define HAVE_AEL2005_PHY 1

//
// IEEE 802.3ae Clause 45 managable device types (DEVAD)
//
#define XGE_MDIO_DEVICE_PMA 1
#define XGE_MDIO_DEVICE_WIS 2
#define XGE_MDIO_DEVICE_PCS 3
#define XGE_MDIO_DEVICE_PHY_XS 4
#define XGE_MDIO_DEVICE_DTE_XS 5
#define XGE_MDIO_DEVICE_TC 6

//
// IEEE 802.3ae Clause 45 register set for MDIO
//
#define XGE_MDIO_CONTROL1 0
#define XGE_MDIO_STATUS1 1
#define XGE_MDIO_DEVID1 2
#define XGE_MDIO_DEVID2 3
#define XGE_MDIO_SPEED 4
#define XGE_MDIO_DEVICES1 5
#define XGE_MDIO_DEVICES2 6
#define XGE_MDIO_CONTROL2 7
#define XGE_MDIO_STATUS2 8
#define XGE_MDIO_LANESTATUS 24
#define XGE_MDIO_TESTCTRL 25
#define XILINX_CORE_VERSION 65535

//
// QR2 AEL2005 Phy address on MDIO (PORT ADDR)
//
#define XGE_MDIO_ADDR_PHY_A 0

//
// QR2 MDIO address of FPGA XAUI (DTE XS) (PORT ADDR)
//
#define XGE_MDIO_ADDR_XAUI_A 2

//
// ID's for all XGE interfaces
#define XGE_A 0

// PHY module types 
enum {
  SFFP_TYPE_NONE,
  SFFP_TYPE_SR,
  SFFP_TYPE_LR,
  SFFP_TYPE_LRM,
  SFFP_TYPE_TWINAX,
  SFFP_TYPE_TWINAX_LONG,
  SFFP_TYPE_UNKNOWN,
  SFFP_TYPE_1000BASE_SX,
  SFFP_TYPE_1000BASE_LX,
  SFFP_TYPE_1000BASE_T
};

// PHY module I2C device address 
// (I2C device driver shifts "7bit" address left 1 bit)
// SFF-8431 specifys the I2C address as 8 bits with lSB as X
enum {
  MODULE_DEV_ADDR = (0xa0 >> 1),
  SFF_DEV_ADDR    = (0xa2 >>1),
};

// SFPP module status
enum {
  SFFP_NO_CHANGE,
  SFFP_REMOVED,
  SFFP_INSERTED
};

#endif


