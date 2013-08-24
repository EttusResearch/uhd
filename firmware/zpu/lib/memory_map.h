// Copyright 2010-2012 Ettus Research LLC
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

#define MASTER_CLK_RATE 100000000 // 100 MHz

////////////////////////////////////////////////////////////////////////
// Define slave bases
////////////////////////////////////////////////////////////////////////
#define ROUTER_RAM_BASE     0x4000
#define SPI_BASE            0x5000
#define I2C_BASE            0x5400
#define GPIO_BASE           0x5800
#define READBACK_BASE       0x5C00
#define ETH_BASE            0x6000
#define SETTING_REGS_BASE   0x7000
#define PIC_BASE            0x8000
#define UART_BASE           0x8800
#define ATR_BASE            0x8C00
#ifdef USRP2
#define SDSPI_BASE          0xB000
#endif
#ifdef USRP2P
#define ICAP_BASE           0xA000
#define SPIF_BASE           0xB000
#define RAM_BASE            0xC000
#endif

/////////////////////////////////////////////////////
// SPI Core, Slave 2.  See core docs for more info
/////////////////////////////////////////////////////

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
////////////////////////////////////////////////

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

///////////////////////////////////////////////////
// Packet Router Status, Slave 5
///////////////////////////////////////////////////

typedef struct {
  volatile uint32_t spi;
  volatile uint32_t _padding[7];
  volatile uint32_t status;
  volatile uint32_t _unused;
  volatile uint32_t time64_secs_rb;
  volatile uint32_t time64_ticks_rb;
  volatile uint32_t compat_num;
  volatile uint32_t irqs;
} router_status_t;

#define SPI_READY_IRQ (1 << 12)

#define router_status ((router_status_t *) READBACK_BASE)
#define readback_mux ((router_status_t *) READBACK_BASE) //alias with a better name

/*!
 * \brief return non-zero if we're running under the simulator
 */
inline static int
hwconfig_simulation_p(void)
{
  return 0;
}

/*!
 * \brief Return Wishbone Clock divisor.
 * The processor runs at the Wishbone Clock rate which is MASTER_CLK_RATE / divisor.
 */
inline static int
hwconfig_wishbone_divisor(void)
{
  return 2;
}

///////////////////////////////////////////////////
// Ethernet Core, Slave 6
///////////////////////////////////////////////////

typedef struct {
  volatile int settings;
  volatile int ucast_hi;
  volatile int ucast_lo;
  volatile int mcast_hi;
  volatile int mcast_lo;
  volatile int miimoder;
  volatile int miiaddress;
  volatile int miitx_data;
  volatile int miicommand;
  volatile int miistatus;
  volatile int miirx_data;
  volatile int pause_time;
  volatile int pause_thresh;
} eth_mac_regs_t;

// settings register
#define MAC_SET_PAUSE_EN  (1 << 0)   // Makes us respect received pause frames (normally on)
#define MAC_SET_PASS_ALL  (1 << 1)   // Enables promiscuous mode, currently broken
#define MAC_SET_PASS_PAUSE (1 << 2)  // Sends pause frames through (normally off)
#define MAC_SET_PASS_BCAST (1 << 3)  // Sends broadcast frames through (normally on)
#define MAC_SET_PASS_MCAST (1 << 4)  // Sends multicast frames that match mcast addr (normally off)
#define MAC_SET_PASS_UCAST (1 << 5)  // Sends unicast (normal) frames through if they hit in address filter (normally on)
#define MAC_SET_PAUSE_SEND_EN (1 << 6) // Enables sending pause frames

// miicommand register
#define MIIC_SCANSSTAT	(1 << 0)	// Scan status
#define MIIC_RSTAT      (1 << 1)	// Read status
#define	MIIC_WCTRLDATA	(1 << 2)	// Write control data

// miistatus register
#define MIIS_LINKFAIL	(1 << 0)	// The link failed
#define	MIIS_BUSY	(1 << 1)	// The MII is busy (operation in progress)
#define	MIIS_NVALID	(1 << 2)	// The data in the status register is invalid
					//   This it is only valid when the scan status is active.

#define eth_mac ((eth_mac_regs_t *) ETH_BASE)

////////////////////////////////////////////////////
// Settings Bus, Slave #7, Not Byte Addressable!
//
// Output-only from processor point-of-view.
// 1KB of address space (== 256 32-bit write-only regs)
////////////////////////////////////////////////////

#define SR_MISC       0   // 7 regs
#define SR_SIMTIMER   8   // 2
#define SR_TIME64    10   // 6
#define SR_BUF_POOL  16   // 4
#define SR_SPI_CORE  20   // 3
#define SR_RX_FRONT  24   // 5
#define SR_RX_CTRL0  32   // 9
#define SR_RX_DSP0   48   // 7
#define SR_RX_CTRL1  80   // 9
#define SR_RX_DSP1   96   // 7

#define SR_TX_FRONT 128   // ?
#define SR_TX_CTRL  144   // 6
#define SR_TX_DSP   160   // 5

#define SR_UDP_SM   192   // 64

#define	_SR_ADDR(sr) (SETTING_REGS_BASE + (sr) * sizeof(uint32_t))

#define SR_ADDR_BLDRDONE _SR_ADDR(5)

// --- spi core control regs ---

typedef struct {
  volatile uint32_t divider;
  volatile uint32_t control;
  volatile uint32_t data;
} spi_core_t;

#define SPI_CORE_SLAVE_SELECT_SHIFT 0
#define SPI_CORE_NUM_BITS_SHIFT 24
#define SPI_CORE_DATA_IN_EDGE_SHIFT 30
#define SPI_CORE_DATA_OUT_EDGE_SHIFT 31

#define spi_core ((spi_core_t *) _SR_ADDR(SR_SPI_CORE))

// --- packet router control regs ---

typedef struct {
  volatile uint32_t mode_ctrl;
  volatile uint32_t ip_addr;
  volatile uint32_t data_ports; //dsp0 (low 16) dsp1 (high 16)
  volatile uint32_t iface_ctrl;
} router_ctrl_t;

#define router_ctrl ((router_ctrl_t *) _SR_ADDR(SR_BUF_POOL))

// --- misc outputs ---

typedef struct {
  volatile uint32_t clk_ctrl;
  volatile uint32_t serdes_ctrl;
  volatile uint32_t adc_ctrl;
  volatile uint32_t leds;
  volatile uint32_t phy_ctrl;        // LSB is reset line to eth phy
  volatile uint32_t debug_mux_ctrl;
  volatile uint32_t led_src;         // HW or SW control for LEDs
  volatile uint32_t flush_icache;    // Flush the icache
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

#define output_regs ((output_regs_t *) SETTING_REGS_BASE)

// --- protocol framer regs ---

typedef struct{
    struct{
        volatile uint32_t entry[16];
    } table[4];
} sr_proto_framer_t;

#define sr_proto_framer_regs ((sr_proto_framer_t *) _SR_ADDR(SR_UDP_SM))

// --- VITA TX CTRL regs ---

typedef struct {
  volatile uint32_t     _pad[4];
  volatile uint32_t     cyc_per_up;
} sr_tx_ctrl_t;

#define sr_tx_ctrl ((sr_tx_ctrl_t *) _SR_ADDR(SR_TX_CTRL))

// --- VITA RX CTRL regs ---
typedef struct {
  volatile uint32_t	_pad[8];
  volatile uint32_t	clear;
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

///////////////////////////////////////////////////////
// Simple Programmable Interrupt Controller, Slave 8
///////////////////////////////////////////////////////

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
// UART, Slave 10
///////////////////////////////////////////////////

typedef struct {
  //  All elements are 8 bits except for clkdiv (16), but we use uint32 to make 
  //    the hardware for decoding easier
  volatile uint32_t clkdiv;  // Set to 50e6 divided by baud rate (no x16 factor)
  volatile uint32_t txlevel; // Number of spaces in the FIFO for writes
  volatile uint32_t rxlevel; // Number of available elements in the FIFO for reads
  volatile uint32_t txchar;  // Write characters to be sent here
  volatile uint32_t rxchar;  // Read received characters here
  volatile uint32_t padding[3]; //what is this i don't even
} uart_regs_t;

#define uart_regs ((uart_regs_t *) UART_BASE)

///////////////////////////////////////////////////
// SD Card SPI interface, Slave 13
//   All regs are 8 bits wide, but are accessed as if they are 32 bits
///////////////////////////////////////////////////
#ifdef USRP2

typedef struct {
  volatile uint32_t status;  // Write a 1 or 0 for controlling CS
  volatile uint32_t clkdiv;
  volatile uint32_t send_dat;
  volatile uint32_t receive_dat;
} sdspi_regs_t;

#define sdspi_regs ((sdspi_regs_t *) SDSPI_BASE)

#endif //USRP2

///////////////////////////////////////////////////
// ICAP, Slave 13
///////////////////////////////////////////////////
#ifdef USRP2P

typedef struct {
  uint32_t icap; //only the lower 8 bits matter
} icap_regs_t;

#define icap_regs ((icap_regs_t *) ICAP_BASE)

///////////////////////////////////////////////////
// SPI Flash interface, Slave 14
// Control register definitions are the same as SPI, so use SPI_CTRL_ASS, etc.
// Peripheral mask not needed since bus is dedicated (CE held low)
///////////////////////////////////////////////////

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

#endif //USRP2P

#endif /* INCLUDED_MEMORY_MAP_H */
