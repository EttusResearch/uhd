// Copyright 2010-2011 Ettus Research LLC
/*
 * Copyright 2007,2008,2009 Free Software Foundation, Inc.
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

#ifndef INCLUDED_MEMORY_MAP_H
#define INCLUDED_MEMORY_MAP_H

#include <stdint.h>


#define MASTER_CLK_RATE        100000000		// 100 MHz


////////////////////////////////////////////////////////////////
//
//         Memory map for embedded wishbone bus
//
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
// Boot RAM, Slave 0

#define BOOTRAM_BASE 0x0000


////////////////////////////////////////////////////////////////
// Packet Router RAM, Slave 1
//
// The buffers themselves are located in Slave 1, Packet Router RAM.
// The status registers are in Slave 5, Packet Router Status.
// The control register is in Slave 7, Settings Bus.

#define ROUTER_RAM_BASE 0x4000

/////////////////////////////////////////////////////
// SPI Core, Slave 2.  See core docs for more info
#define SPI_BASE 0x6000   // Base address (16-bit) is base peripheral addr

typedef struct {
  volatile uint32_t	txrx0;
  volatile uint32_t	txrx1;
  volatile uint32_t	txrx2;
  volatile uint32_t	txrx3;
  volatile uint32_t	ctrl;
  volatile uint32_t	div;
  volatile uint32_t	ss;
} spi_regs_t;

#define spi_regs ((spi_regs_t *) SPI_BASE)


// Masks for controlling different peripherals
#define SPI_SS_AD9510    1
#define SPI_SS_AD9777    2
#define SPI_SS_RX_DAC    4
#define SPI_SS_RX_ADC    8
#define SPI_SS_RX_DB    16
#define SPI_SS_TX_DAC   32
#define SPI_SS_TX_ADC   64
#define SPI_SS_TX_DB   128
#define SPI_SS_ADS62P44 256

// Masks for different parts of CTRL reg
#define SPI_CTRL_ASS      (1<<13)
#define SPI_CTRL_IE       (1<<12)
#define SPI_CTRL_LSB      (1<<11)
#define SPI_CTRL_TXNEG    (1<<10)
#define SPI_CTRL_RXNEG    (1<< 9)
#define SPI_CTRL_GO_BSY   (1<< 8)
#define SPI_CTRL_CHAR_LEN_MASK 0x7F

////////////////////////////////////////////////
// I2C, Slave 3
// See Wishbone I2C-Master Core Specification.

#define I2C_BASE 0x6100

typedef struct {
  volatile uint32_t  prescaler_lo;	// r/w
  volatile uint32_t  prescaler_hi;	// r/w
  volatile uint32_t  ctrl;		// r/w
  volatile uint32_t  data;		// wr = transmit reg; rd = receive reg
  volatile uint32_t  cmd_status;	// wr = command reg;  rd = status reg
} i2c_regs_t;

#define i2c_regs ((i2c_regs_t *) I2C_BASE)

#define	I2C_CTRL_EN	(1 << 7)	// core enable
#define	I2C_CTRL_IE	(1 << 6)	// interrupt enable

//
// STA, STO, RD, WR, and IACK bits are cleared automatically
//
#define	I2C_CMD_START	(1 << 7)	// generate (repeated) start condition
#define I2C_CMD_STOP	(1 << 6)	// generate stop condition
#define	I2C_CMD_RD	(1 << 5)	// read from slave
#define I2C_CMD_WR	(1 << 4)	// write to slave
#define	I2C_CMD_NACK	(1 << 3)	// when a rcvr, send ACK (ACK=0) or NACK (ACK=1)
#define I2C_CMD_RSVD_2	(1 << 2)	// reserved
#define	I2C_CMD_RSVD_1	(1 << 1)	// reserved
#define I2C_CMD_IACK	(1 << 0)	// set to clear pending interrupt

#define I2C_ST_RXACK	(1 << 7)	// Received acknowledgement from slave (1 = NAK, 0 = ACK)
#define	I2C_ST_BUSY	(1 << 6)	// 1 after START signal detected; 0 after STOP signal detected
#define	I2C_ST_AL	(1 << 5)	// Arbitration lost.  1 when core lost arbitration
#define	I2C_ST_RSVD_4	(1 << 4)	// reserved
#define	I2C_ST_RSVD_3	(1 << 3)	// reserved
#define	I2C_ST_RSVD_2	(1 << 2)	// reserved
#define I2C_ST_TIP	(1 << 1)	// Transfer-in-progress
#define	I2C_ST_IP	(1 << 0)	// Interrupt pending


////////////////////////////////////////////////
// GPIO, Slave 4
//
// These go to the daughterboard i/o pins

#define GPIO_BASE 0x6200

typedef struct {
  volatile uint32_t	io;	  // tx data in high 16, rx in low 16
  volatile uint32_t     ddr;      // 32 bits, 1 means output. tx in high 16, rx in low 16
  volatile uint32_t	tx_sel;   // 16 2-bit fields select which source goes to TX DB
  volatile uint32_t	rx_sel;   // 16 2-bit fields select which source goes to RX DB
} gpio_regs_t;

// each 2-bit sel field is layed out this way
#define GPIO_SEL_SW	   0 // if pin is an output, set by software in the io reg
#define	GPIO_SEL_ATR	   1 // if pin is an output, set by ATR logic
#define	GPIO_SEL_DEBUG_0   2 // if pin is an output, debug lines from FPGA fabric
#define	GPIO_SEL_DEBUG_1   3 // if pin is an output, debug lines from FPGA fabric

#define gpio_base ((gpio_regs_t *) GPIO_BASE)

///////////////////////////////////////////////////
// Packet Router Status, Slave 5
//
// The buffers themselves are located in Slave 1, Packet Router RAM.
// The status registers are in Slave 5, Packet Router Status.
// The control register is in Slave 7, Settings Bus.

#define ROUTER_STATUS_BASE 0x6300

typedef struct {
  volatile uint32_t _padding[8];
  volatile uint32_t status;
  volatile uint32_t hw_config;	         // see below
  volatile uint32_t time64_secs_rb;
  volatile uint32_t time64_ticks_rb;
  volatile uint32_t compat_num;
  volatile uint32_t irqs;
} router_status_t;

#define router_status ((router_status_t *) ROUTER_STATUS_BASE)

#define BUTTON_PUSHED ((router_status->irqs & PIC_BUTTON) ? 0 : 1)

// The hw_config register

#define	HWC_SIMULATION		0x80000000
#define	HWC_WB_CLK_DIV_MASK	0x0000000f

/*!
 * \brief return non-zero if we're running under the simulator
 */
inline static int
hwconfig_simulation_p(void)
{
  return router_status->hw_config & HWC_SIMULATION;
}

/*!
 * \brief Return Wishbone Clock divisor.
 * The processor runs at the Wishbone Clock rate which is MASTER_CLK_RATE / divisor.
 */
inline static int
hwconfig_wishbone_divisor(void)
{
  return router_status->hw_config & HWC_WB_CLK_DIV_MASK;
}

///////////////////////////////////////////////////
// Ethernet Core, Slave 6

#define ETH_BASE 0x6400

#include "eth_mac_regs.h"

#define eth_mac ((eth_mac_regs_t *) ETH_BASE)

////////////////////////////////////////////////////
// Settings Bus, Slave #7, Not Byte Addressable!
//
// Output-only from processor point-of-view.
// 1KB of address space (== 256 32-bit write-only regs)


#define MISC_OUTPUT_BASE        0x5000

#define SR_MISC 0
#define SR_TX_PROT_ENG 32
#define SR_RX_PROT_ENG 48
#define SR_ROUTER_CTRL 64
#define SR_UDP_SM 96
#define SR_TX_DSP 208
#define SR_TX_CTRL 224
#define SR_RX_DSP0 160
#define SR_RX_DSP1 240
#define SR_RX_CTRL0 176
#define SR_RX_CTRL1 32
#define SR_TIME64 192
#define SR_SIMTIMER 198
#define SR_LAST 255

#define	_SR_ADDR(sr)	(MISC_OUTPUT_BASE + (sr) * sizeof(uint32_t))

#define SR_ADDR_BLDRDONE _SR_ADDR(5)

// --- packet router control regs ---

typedef struct {
  volatile uint32_t mode_ctrl;
  volatile uint32_t ip_addr;
  volatile uint32_t data_ports; //dsp0 (low 16) dsp1 (high 16)
  volatile uint32_t iface_ctrl;
} router_ctrl_t;

#define router_ctrl ((router_ctrl_t *) _SR_ADDR(SR_ROUTER_CTRL))

// --- misc outputs ---

typedef struct {
  volatile uint32_t	clk_ctrl;
  volatile uint32_t	serdes_ctrl;
  volatile uint32_t	adc_ctrl;
  volatile uint32_t	leds;
  volatile uint32_t	phy_ctrl;	// LSB is reset line to eth phy
  volatile uint32_t	debug_mux_ctrl;
  volatile uint32_t     ram_page;       // FIXME should go somewhere else...
  volatile uint32_t     flush_icache;   // Flush the icache
  volatile uint32_t     led_src;        // HW or SW control for LEDs
} output_regs_t;

#define CLK_RESET  (1<<4)
#define CLK_ENABLE (1<<3) | (1<<2)
#define CLK_SEL    (1<<1) | (1<<0)

#define SERDES_ENABLE 8
#define SERDES_PRBSEN 4
#define SERDES_LOOPEN 2
#define SERDES_RXEN   1

#define	ADC_CTRL_ON	0x0F
#define	ADC_CTRL_OFF	0x00

// crazy order that matches the labels on the case

#define	LED_A		(1 << 4)
#define	LED_B		(1 << 1)
#define	LED_E		(1 << 2)
#define	LED_D		(1 << 0)
#define	LED_C		(1 << 3)
//      LED_F		// controlled by CPLD
#define	LED_RJ45	(1 << 5)

#define output_regs ((output_regs_t *) MISC_OUTPUT_BASE)

// --- udp tx regs ---

typedef struct {
  // Bits 19:16 are control info; bits 15:0 are data (see below)
  // First two words are unused.
  volatile uint32_t _nope[2];
  //--- ethernet header - 14 bytes---
  volatile struct{
    uint32_t mac_dst_0_1; //word 2
    uint32_t mac_dst_2_3;
    uint32_t mac_dst_4_5;
    uint32_t mac_src_0_1;
    uint32_t mac_src_2_3;
    uint32_t mac_src_4_5;
    uint32_t ether_type; //word 8
  } eth_hdr;
  //--- ip header - 20 bytes ---
  volatile struct{
    uint32_t ver_ihl_tos; //word 9
    uint32_t total_length;
    uint32_t identification;
    uint32_t flags_frag_off;
    uint32_t ttl_proto;
    uint32_t checksum;
    uint32_t src_addr_high;
    uint32_t src_addr_low;
    uint32_t dst_addr_high;
    uint32_t dst_addr_low; //word 18
  } ip_hdr;
  //--- udp header - 8 bytes ---
  volatile struct{
    uint32_t src_port; //word 19
    uint32_t dst_port;
    uint32_t length;
    uint32_t checksum; //word 22
  } udp_hdr;
  volatile uint32_t _pad[1];
  volatile uint32_t dsp0_port;
  volatile uint32_t err0_port;
  volatile uint32_t dsp1_port;
  volatile uint32_t err1_port;
} sr_udp_sm_t;

// control bits (all expect UDP_SM_LAST_WORD are mutually exclusive)

// Insert a UDP source port from the table
#define UDP_SM_INS_UDP_SRC_PORT     (1 << 21)

// Insert a UDP dest port from the table
#define UDP_SM_INS_UDP_DST_PORT     (1 << 20)

// This is the last word of the header
#define	UDP_SM_LAST_WORD		(1 << 19)

// Insert IP header checksum here.  Data is the xor of 16'hFFFF and
// the values written into regs 9-13 and 15-18.
#define	UDP_SM_INS_IP_HDR_CHKSUM	(1 << 18)

// Insert IP Length here (data ignored)
#define	UDP_SM_INS_IP_LEN		(1 << 17)

// Insert UDP Length here (data ignore)
#define	UDP_SM_INS_UDP_LEN		(1 << 16)

#define sr_udp_sm ((sr_udp_sm_t *) _SR_ADDR(SR_UDP_SM))

// --- VITA TX CTRL regs ---

typedef struct {
  volatile uint32_t     num_chan;
  volatile uint32_t     clear_state;	// clears out state machine, fifos,
  volatile uint32_t     report_sid;
  volatile uint32_t     policy;
  volatile uint32_t     cyc_per_up;
  volatile uint32_t     packets_per_up;
} sr_tx_ctrl_t;

#define sr_tx_ctrl ((sr_tx_ctrl_t *) _SR_ADDR(SR_TX_CTRL))

// --- VITA RX CTRL regs ---
typedef struct {
  // The following 3 are logically a single command register.
  // They are clocked into the underlying fifo when time_ticks is written.
  volatile uint32_t	cmd;		// {now, chain, num_samples(30)
  volatile uint32_t	time_secs;
  volatile uint32_t	time_ticks;
} sr_rx_ctrl_t;

#define sr_rx_ctrl0 ((sr_rx_ctrl_t *) _SR_ADDR(SR_RX_CTRL0))
#define sr_rx_ctrl1 ((sr_rx_ctrl_t *) _SR_ADDR(SR_RX_CTRL1))

// ----------------------------------------------------------------
// VITA49 64 bit time (write only)
  /*!
   * \brief Time 64 flags
   *
   * <pre>
   *
   *    3                   2                   1                       
   *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
   * +-----------------------------------------------------------+-+-+
   * |                                                           |S|P|
   * +-----------------------------------------------------------+-+-+
   *
   * P - PPS edge selection (0=negedge, 1=posedge, default=0)
   * S - Source (0=sma, 1=mimo, 0=default)
   *
   * </pre>
   */
typedef struct {
  volatile uint32_t	secs;	// value to set absolute secs to on next PPS
  volatile uint32_t	ticks;	// value to set absolute ticks to on next PPS
  volatile uint32_t flags;  // flags - see chart above
  volatile uint32_t imm;    // set immediate (0=latch on next pps, 1=latch immediate, default=0)
} sr_time64_t;

#define sr_time64 ((sr_time64_t *) _SR_ADDR(SR_TIME64))


/* 
 * --- ethernet tx protocol engine regs (write only) ---
 *
 * These registers control the transmit portion of the ethernet
 * protocol engine (out of USRP2).  The protocol engine handles fifo
 * status and sequence number insertion in outgoing packets, and
 * automagically generates status packets when required to inform the
 * host of changes in fifo availability.
 *
 * All outgoing packets have their fifo_status field set to the number
 * of 32-bit lines of fifo available in the ethernet Rx fifo (see
 * usrp2_eth_packet.h).  Seqno's are set if FIXME, else 0.
 *
 * FIXME clean this up once we know how it's supposed to behave.
 */

typedef struct {
  volatile uint32_t  flags;	     // not yet fully defined (channel?)
  volatile uint32_t  mac_dst0123;    // 4 bytes of destination mac addr
  volatile uint32_t  mac_dst45src01; // 2 bytes of dest mac addr; 2 bytes of src mac addr
  volatile uint32_t  mac_src2345;    // 4 bytes of destination mac addr
  volatile uint32_t  seqno;	     // Write to init seqno.  It autoincs on match
} tx_proto_engine_regs_t;

#define tx_proto_engine ((tx_proto_engine_regs_t *) _SR_ADDR(SR_TX_PROT_ENG))

/*
 * --- ethernet rx protocol engine regs (write only) ---
 *
 * These registers control the receive portion of the ethernet
 * protocol engine (into USRP2).  The protocol engine offloads common
 * packet inspection operations so that firmware has less to do on
 * "fast path" packets.
 *
 * The registers define conditions which must be matched for a packet
 * to be considered a "fast path" packet.  If a received packet
 * matches the src and dst mac address, ethertype, flags field, and
 * expected seqno number it is considered a "fast path" packet, and
 * the expected seqno is updated.  If the packet fails to satisfy any
 * of the above conditions it's a "slow path" packet, and the
 * corresponding SLOWPATH flag will be set buffer_status register.
 */

typedef struct {
  volatile uint32_t  flags;	     // not yet fully defined (channel?)
  volatile uint32_t  mac_dst0123;    // 4 bytes of destination mac addr
  volatile uint32_t  mac_dst45src01; // 2 bytes of dest mac addr; 2 bytes of src mac addr
  volatile uint32_t  mac_src2345;    // 4 bytes of destination mac addr
  volatile uint32_t  ethertype_pad;  // ethertype in high 16-bits
} rx_proto_engine_regs_t;

#define rx_proto_engine ((rx_proto_engine_regs_t *) _SR_ADDR(SR_RX_PROT_ENG))



///////////////////////////////////////////////////
// Simple Programmable Interrupt Controller, Slave 8

#define PIC_BASE  0x6500

// Interrupt request lines
// Bit numbers (LSB == 0) that correpond to interrupts into PIC

#define	IRQ_BUFFER	0	// buffer manager
#define	IRQ_ONETIME	1
#define	IRQ_SPI		2
#define	IRQ_I2C		3
#define	IRQ_PHY		4	// ethernet PHY
#define	IRQ_UNDERRUN	5
#define	IRQ_OVERRUN	6
#define	IRQ_PPS		7	// pulse per second
#define	IRQ_UART_RX	8
#define	IRQ_UART_TX	9
#define	IRQ_SERDES	10
#define	IRQ_CLKSTATUS	11
#define IRQ_PERIODIC    12
#define IRQ_BUTTON	13

#define IRQ_TO_MASK(x) (1 << (x))

#define PIC_BUFFER_INT    IRQ_TO_MASK(IRQ_BUFFER)
#define PIC_ONETIME_INT   IRQ_TO_MASK(IRQ_ONETIME)
#define PIC_SPI_INT       IRQ_TO_MASK(IRQ_SPI)
#define PIC_I2C_INT       IRQ_TO_MASK(IRQ_I2C)
#define PIC_PHY_INT       IRQ_TO_MASK(IRQ_PHY)
#define PIC_UNDERRUN_INT  IRQ_TO_MASK(IRQ_UNDERRUN)
#define PIC_OVERRUN_INT   IRQ_TO_MASK(IRQ_OVERRUN)
#define PIC_PPS_INT   	  IRQ_TO_MASK(IRQ_PPS)
#define PIC_UART_RX_INT   IRQ_TO_MASK(IRQ_UART_RX)
#define PIC_UART_TX_INT   IRQ_TO_MASK(IRQ_UART_TX)
#define PIC_SERDES        IRQ_TO_MASK(IRQ_SERDES)
#define PIC_CLKSTATUS     IRQ_TO_MASK(IRQ_CLKSTATUS)
#define PIC_BUTTON				IRQ_TO_MASK(IRQ_BUTTON)

typedef struct {
  volatile uint32_t edge_enable; // mask: 1 -> edge triggered, 0 -> level
  volatile uint32_t polarity;	 // mask: 1 -> rising edge
  volatile uint32_t mask;	 // mask: 1 -> disabled
  volatile uint32_t pending;	 // mask: 1 -> pending; write 1's to clear pending ints
} pic_regs_t;

#define pic_regs ((pic_regs_t *) PIC_BASE)

// ----------------------------------------------------------------
// WB_CLK_RATE is the time base for this
typedef struct {
  volatile uint32_t	onetime;   // Number of wb clk cycles till the onetime interrupt
  volatile uint32_t	periodic;  // Repeat rate of periodic interrupt
} sr_simple_timer_t;

#define sr_simple_timer ((sr_simple_timer_t *) _SR_ADDR(SR_SIMTIMER))

///////////////////////////////////////////////////
// UNUSED, Slave 9

///////////////////////////////////////////////////
// UART, Slave 10

#define UART_BASE  0x6700

typedef struct {
  //  All elements are 8 bits except for clkdiv (16), but we use uint32 to make 
  //    the hardware for decoding easier
  volatile uint32_t clkdiv;  // Set to 50e6 divided by baud rate (no x16 factor)
  volatile uint32_t txlevel; // Number of spaces in the FIFO for writes
  volatile uint32_t rxlevel; // Number of available elements in the FIFO for reads
  volatile uint32_t txchar;  // Write characters to be sent here
  volatile uint32_t rxchar;  // Read received characters here
  volatile uint32_t x[3]; //padding to reach 32B
} uart_regs_t;

#define uart_regs ((uart_regs_t *) UART_BASE)

///////////////////////////////////////////////////
// ATR Controller, Slave 11

#define ATR_BASE  0x6800

typedef struct {
  volatile uint32_t	v[16];
} atr_regs_t;

#define	ATR_IDLE	0x0	// indicies into v
#define ATR_TX		0x1
#define	ATR_RX		0x2
#define	ATR_FULL	0x3

#define atr_regs ((atr_regs_t *) ATR_BASE)

///////////////////////////////////////////////////
// UNUSED, Slave 12

///////////////////////////////////////////////////
// ICAP, Slave 13

#define ICAP_BASE 0x6A00
typedef struct {
  uint32_t icap; //only the lower 8 bits matter
} icap_regs_t;

#define icap_regs ((icap_regs_t *) ICAP_BASE)

///////////////////////////////////////////////////
// SPI Flash interface, Slave 14
// Control register definitions are the same as SPI, so use SPI_CTRL_ASS, etc.
// Peripheral mask not needed since bus is dedicated (CE held low)

#define SPIF_BASE 0x6B00
typedef struct {
  volatile uint32_t	txrx0;
  volatile uint32_t	txrx1;
  volatile uint32_t	txrx2;
  volatile uint32_t	txrx3;
  volatile uint32_t	ctrl;
  volatile uint32_t	div;
  volatile uint32_t	ss;
} spif_regs_t;

#define spif_regs ((spif_regs_t *) SPIF_BASE)

////////////////////////////////////////////////////////////////
// Main RAM, Slave 15

#define RAM_BASE 0x8000



///////////////////////////////////////////////////
#endif

