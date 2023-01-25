
// Copyright 2014 Ettus Research LLC

#ifndef INCLUDED_X300_DEFS_H
#define INCLUDED_X300_DEFS_H

#define CPU_CLOCK  93750000     // Half of X300_BUS_CLOCK_RATE (187.5 MHz)
#define MAIN_RAM_BASE 0x0000
#define PKT_RAM0_BASE 0x8000
#define SFP0_MAC_BASE 0xC000
#define SFP1_MAC_BASE 0xD000
#define BOOT_LDR_BASE 0xFA00
#define UART0_BASE 0xfd00
#define UART0_BAUD 115200
#define UART1_BASE 0xf900
#define UART1_BAUD 115200
#define I2C0_BASE 0xfe00
#define I2C1_BASE 0xff00
#define I2C2_BASE 0xfc00
#define SET0_BASE 0xa000
#define RB0_BASE 0xa000 //same as set

//eeprom map for mboard addrs
#define MBOARD_EEPROM_ADDR 0x50

// Setting Regs Memeory Map
static const int SR_LEDS       = 0; // see below for bit values
static const int SR_SW_RST     = 1;
//static const int SR_CLOCK_CTRL = 2;
//static const int SR_DEVICE_ID  = 3;
//static const int SR_SFPP_CTRL  = 4;
//static const int SR_SPI        = 32;
static const int SR_ETHINT0    = 40;
static const int SR_ETHINT1    = 56;
static const int SR_RB_ADDR    = 128;
// Transport adapter controls (see below for offsets)
static const int SR_SFP0_ADAPTER = 144;
static const int SR_SFP1_ADAPTER = 160;

//led shifts for SR_LEDS
static const int LED_LINKSTAT = (1 << 1); // green
static const int LED_LINKACT  = (1 << 0); // red

// Offsets for transport adapter controls
static const int TA_COMPAT_NUM    = 0; // 8 bits major, 8 bits minor
static const int TA_INFO          = 1;
static const int TA_NODE_INST     = 2; // read-only
static const int TA_KV_MAC_LO     = 3;
static const int TA_KV_MAC_HI     = 4;
static const int TA_KV_IPV4       = 5;
static const int TA_KV_UDP_PORT   = 6;
static const int TA_KV_CFG        = 7;
static const int TA_KV_IPV4_W_ARP = 8; // not a true SR!

// Readback Memory Map
static const int RB_COUNTER      = 0;
static const int RB_SPI_RDY      = 1;
static const int RB_SPI_DATA     = 2;
static const int RB_SFP0_TYPE    = 4;
static const int RB_SFP1_TYPE    = 5;
static const int RB_FPGA_COMPAT  = 6;
static const int RB_SFP0_STATUS  = 8;
static const int RB_SFP1_STATUS  = 9;

// Bootloader Memory Map
static const int BL_ADDRESS     = 0;
static const int BL_DATA        = 1;

#define SW_RST_PHY 0x01
#define SW_RST_RADIO 0x02
#define SW_RST_RADIO_PLL 0x04

// SFP type constants
#define RB_SFP_1G_ETH   0
#define RB_SFP_10G_ETH  1
#define RB_SFP_AURORA   2

#endif /* INCLUDED_X300_DEFS_H */
