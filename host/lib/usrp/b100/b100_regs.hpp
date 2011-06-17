

////////////////////////////////////////////////////////////////
//
//         Memory map for wishbone bus
//
////////////////////////////////////////////////////////////////

// All addresses are byte addresses.  All accesses are word (16-bit) accesses.
//  This means that address bit 0 is usually 0.
//  There are 11 bits of address for the control.

#ifndef __B100_REGS_H
#define __B100_REGS_H

/////////////////////////////////////////////////////
// Slave pointers

#define B100_REG_SLAVE(n) ((n)<<7)

/////////////////////////////////////////////////////
// Slave 0 -- Misc Regs

#define B100_REG_MISC_BASE B100_REG_SLAVE(0)

#define B100_REG_MISC_LED        B100_REG_MISC_BASE + 0
#define B100_REG_MISC_SW         B100_REG_MISC_BASE + 2
#define B100_REG_MISC_CGEN_CTRL  B100_REG_MISC_BASE + 4
#define B100_REG_MISC_CGEN_ST    B100_REG_MISC_BASE + 6
#define B100_REG_MISC_TEST       B100_REG_MISC_BASE + 8
#define B100_REG_MISC_RX_LEN     B100_REG_MISC_BASE + 10
#define B100_REG_MISC_TX_LEN     B100_REG_MISC_BASE + 12
#define B100_REG_MISC_XFER_RATE  B100_REG_MISC_BASE + 14
#define B100_REG_MISC_COMPAT     B100_REG_MISC_BASE + 16

/////////////////////////////////////////////////////
// Slave 1 -- UART
//   CLKDIV is 16 bits, others are only 8

#define B100_REG_UART_BASE B100_REG_SLAVE(1)

#define B100_REG_UART_CLKDIV  B100_REG_UART_BASE + 0
#define B100_REG_UART_TXLEVEL B100_REG_UART_BASE + 2
#define B100_REG_UART_RXLEVEL B100_REG_UART_BASE + 4
#define B100_REG_UART_TXCHAR  B100_REG_UART_BASE + 6
#define B100_REG_UART_RXCHAR  B100_REG_UART_BASE + 8

/////////////////////////////////////////////////////
// Slave 2 -- SPI Core
//these are 32-bit registers mapped onto the 16-bit Wishbone bus. 
//Using peek32/poke32 should allow transparent use of these registers.
#define B100_REG_SPI_BASE B100_REG_SLAVE(2)
#define B100_REG_SPI_TXRX0 B100_REG_SPI_BASE + 0
#define B100_REG_SPI_TXRX1 B100_REG_SPI_BASE + 4
#define B100_REG_SPI_TXRX2 B100_REG_SPI_BASE + 8
#define B100_REG_SPI_TXRX3 B100_REG_SPI_BASE + 12
#define B100_REG_SPI_CTRL  B100_REG_SPI_BASE + 16
#define B100_REG_SPI_DIV   B100_REG_SPI_BASE + 20
#define B100_REG_SPI_SS    B100_REG_SPI_BASE + 24

//spi slave constants
#define B100_SPI_SS_AD9862    (1 << 2)
#define B100_SPI_SS_TX_DB     (1 << 1)
#define B100_SPI_SS_RX_DB     (1 << 0)

//spi ctrl register bit definitions
#define SPI_CTRL_ASS      (1<<13)
#define SPI_CTRL_IE       (1<<12)
#define SPI_CTRL_LSB      (1<<11)
#define SPI_CTRL_TXNEG    (1<<10) //mosi edge, push on falling edge when 1
#define SPI_CTRL_RXNEG    (1<< 9) //miso edge, latch on falling edge when 1
#define SPI_CTRL_GO_BSY   (1<< 8)
#define SPI_CTRL_CHAR_LEN_MASK 0x7F

////////////////////////////////////////////////
// Slave 3 -- I2C Core

#define B100_REG_I2C_BASE B100_REG_SLAVE(3)
#define B100_REG_I2C_PRESCALER_LO B100_REG_I2C_BASE + 0
#define B100_REG_I2C_PRESCALER_HI B100_REG_I2C_BASE + 2
#define B100_REG_I2C_CTRL         B100_REG_I2C_BASE + 4
#define B100_REG_I2C_DATA         B100_REG_I2C_BASE + 6
#define B100_REG_I2C_CMD_STATUS   B100_REG_I2C_BASE + 8

//and while we're here...

//
// STA, STO, RD, WR, and IACK bits are cleared automatically
//

#define	I2C_CTRL_EN	(1 << 7)	// core enable
#define	I2C_CTRL_IE	(1 << 6)	// interrupt enable

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
// Slave 4 -- GPIO

#define B100_REG_GPIO_BASE B100_REG_SLAVE(4)

#define B100_REG_GPIO_RX_IO      B100_REG_GPIO_BASE + 0
#define B100_REG_GPIO_TX_IO      B100_REG_GPIO_BASE + 2
#define B100_REG_GPIO_RX_DDR     B100_REG_GPIO_BASE + 4
#define B100_REG_GPIO_TX_DDR     B100_REG_GPIO_BASE + 6
#define B100_REG_GPIO_RX_SEL     B100_REG_GPIO_BASE + 8
#define B100_REG_GPIO_TX_SEL     B100_REG_GPIO_BASE + 10
#define B100_REG_GPIO_RX_DBG     B100_REG_GPIO_BASE + 12
#define B100_REG_GPIO_TX_DBG     B100_REG_GPIO_BASE + 14

//possible bit values for sel when dbg is 0:
#define GPIO_SEL_SW    0 // if pin is an output, set by software in the io reg
#define GPIO_SEL_ATR   1 // if pin is an output, set by ATR logic

//possible bit values for sel when dbg is 1:
#define GPIO_SEL_DEBUG_0   0 // if pin is an output, debug lines from FPGA fabric
#define GPIO_SEL_DEBUG_1   1 // if pin is an output, debug lines from FPGA fabric

///////////////////////////////////////////////////
// Slave 6 -- ATR Controller
//   16 regs

#define B100_REG_ATR_BASE  B100_REG_SLAVE(6)

#define	B100_REG_ATR_IDLE_RXSIDE  B100_REG_ATR_BASE + 0
#define	B100_REG_ATR_IDLE_TXSIDE  B100_REG_ATR_BASE + 2
#define B100_REG_ATR_INTX_RXSIDE  B100_REG_ATR_BASE + 4
#define B100_REG_ATR_INTX_TXSIDE  B100_REG_ATR_BASE + 6
#define	B100_REG_ATR_INRX_RXSIDE  B100_REG_ATR_BASE + 8
#define	B100_REG_ATR_INRX_TXSIDE  B100_REG_ATR_BASE + 10
#define	B100_REG_ATR_FULL_RXSIDE  B100_REG_ATR_BASE + 12
#define	B100_REG_ATR_FULL_TXSIDE  B100_REG_ATR_BASE + 14

///////////////////////////////////////////////////
// Slave 7 -- Readback Mux 32

#define B100_REG_RB_MUX_32_BASE  B100_REG_SLAVE(7)

#define B100_REG_RB_TIME_NOW_SECS   B100_REG_RB_MUX_32_BASE + 0
#define B100_REG_RB_TIME_NOW_TICKS  B100_REG_RB_MUX_32_BASE + 4
#define B100_REG_RB_TIME_PPS_SECS   B100_REG_RB_MUX_32_BASE + 8
#define B100_REG_RB_TIME_PPS_TICKS  B100_REG_RB_MUX_32_BASE + 12
#define B100_REG_RB_MISC_TEST32     B100_REG_RB_MUX_32_BASE + 16

////////////////////////////////////////////////////
// Slaves 8 & 9 -- Settings Bus
//
// Output-only, no readback, 64 registers total
//  Each register must be written 32 bits at a time
//  First the address xxx_xx00 and then xxx_xx10
// 64 total regs in address space
#define B100_SR_RX_CTRL0 0       // 9 regs (+0 to +8)
#define B100_SR_RX_DSP0 10       // 4 regs (+0 to +3)
#define B100_SR_RX_CTRL1 16      // 9 regs (+0 to +8)
#define B100_SR_RX_DSP1 26       // 4 regs (+0 to +3)
#define B100_SR_TX_CTRL 32       // 4 regs (+0 to +3)
#define B100_SR_TX_DSP 38        // 3 regs (+0 to +2)

#define B100_SR_TIME64 42        // 6 regs (+0 to +5)
#define B100_SR_RX_FRONT 48      // 5 regs (+0 to +4)
#define B100_SR_TX_FRONT 54      // 5 regs (+0 to +4)

#define B100_SR_REG_TEST32 60    // 1 reg
#define B100_SR_CLEAR_RX_FIFO 61 // 1 reg
#define B100_SR_CLEAR_TX_FIFO 62 // 1 reg
#define B100_SR_GLOBAL_RESET 63  // 1 reg

#define B100_REG_SR_ADDR(n) (B100_REG_SLAVE(8) + (4*(n)))

#define B100_REG_SR_MISC_TEST32        B100_REG_SR_ADDR(B100_SR_REG_TEST32)

/////////////////////////////////////////////////
// Magic reset regs
////////////////////////////////////////////////
#define B100_REG_CLEAR_RX           B100_REG_SR_ADDR(B100_SR_CLEAR_RX_FIFO)
#define B100_REG_CLEAR_TX           B100_REG_SR_ADDR(B100_SR_CLEAR_RX_FIFO)
#define B100_REG_GLOBAL_RESET       B100_REG_SR_ADDR(B100_SR_GLOBAL_RESET)

/////////////////////////////////////////////////
// DSP RX Regs
////////////////////////////////////////////////
#define B100_REG_DSP_RX_HELPER(which, offset) ((which == 0)? \
    (B100_REG_SR_ADDR(B100_SR_RX_DSP0 + offset)) : \
    (B100_REG_SR_ADDR(B100_SR_RX_DSP1 + offset)))

#define B100_REG_DSP_RX_FREQ(which)       B100_REG_DSP_RX_HELPER(which, 0)
#define B100_REG_DSP_RX_DECIM(which)      B100_REG_DSP_RX_HELPER(which, 2)
#define B100_REG_DSP_RX_MUX(which)        B100_REG_DSP_RX_HELPER(which, 3)

#define B100_FLAG_DSP_RX_MUX_SWAP_IQ   (1 << 0)
#define B100_FLAG_DSP_RX_MUX_REAL_MODE (1 << 1)

///////////////////////////////////////////////////
// RX CTRL regs
///////////////////////////////////////////////////
#define B100_REG_RX_CTRL_HELPER(which, offset) ((which == 0)? \
    (B100_REG_SR_ADDR(B100_SR_RX_CTRL0 + offset)) : \
    (B100_REG_SR_ADDR(B100_SR_RX_CTRL1 + offset)))

#define B100_REG_RX_CTRL_STREAM_CMD(which)     B100_REG_RX_CTRL_HELPER(which, 0)
#define B100_REG_RX_CTRL_TIME_SECS(which)      B100_REG_RX_CTRL_HELPER(which, 1)
#define B100_REG_RX_CTRL_TIME_TICKS(which)     B100_REG_RX_CTRL_HELPER(which, 2)
#define B100_REG_RX_CTRL_CLEAR(which)          B100_REG_RX_CTRL_HELPER(which, 3)
#define B100_REG_RX_CTRL_VRT_HDR(which)        B100_REG_RX_CTRL_HELPER(which, 4)
#define B100_REG_RX_CTRL_VRT_SID(which)        B100_REG_RX_CTRL_HELPER(which, 5)
#define B100_REG_RX_CTRL_VRT_TLR(which)        B100_REG_RX_CTRL_HELPER(which, 6)
#define B100_REG_RX_CTRL_NSAMPS_PP(which)      B100_REG_RX_CTRL_HELPER(which, 7)
#define B100_REG_RX_CTRL_NCHANNELS(which)      B100_REG_RX_CTRL_HELPER(which, 8)

/////////////////////////////////////////////////
// RX FE
////////////////////////////////////////////////
#define B100_REG_RX_FE_SWAP_IQ             B100_REG_SR_ADDR(B100_SR_RX_FRONT + 0) //lower bit
#define B100_REG_RX_FE_MAG_CORRECTION      B100_REG_SR_ADDR(B100_SR_RX_FRONT + 1) //18 bits
#define B100_REG_RX_FE_PHASE_CORRECTION    B100_REG_SR_ADDR(B100_SR_RX_FRONT + 2) //18 bits
#define B100_REG_RX_FE_OFFSET_I            B100_REG_SR_ADDR(B100_SR_RX_FRONT + 3) //18 bits
#define B100_REG_RX_FE_OFFSET_Q            B100_REG_SR_ADDR(B100_SR_RX_FRONT + 4) //18 bits

/////////////////////////////////////////////////
// DSP TX Regs
////////////////////////////////////////////////
#define B100_REG_DSP_TX_FREQ          B100_REG_SR_ADDR(B100_SR_TX_DSP + 0)
#define B100_REG_DSP_TX_SCALE_IQ      B100_REG_SR_ADDR(B100_SR_TX_DSP + 1)
#define B100_REG_DSP_TX_INTERP_RATE   B100_REG_SR_ADDR(B100_SR_TX_DSP + 2)

///////////////////////////////////////////////////
// TX CTRL regs
///////////////////////////////////////////////////
#define B100_REG_TX_CTRL_NUM_CHAN       B100_REG_SR_ADDR(B100_SR_TX_CTRL + 0)
#define B100_REG_TX_CTRL_CLEAR_STATE    B100_REG_SR_ADDR(B100_SR_TX_CTRL + 1)
#define B100_REG_TX_CTRL_REPORT_SID     B100_REG_SR_ADDR(B100_SR_TX_CTRL + 2)
#define B100_REG_TX_CTRL_POLICY         B100_REG_SR_ADDR(B100_SR_TX_CTRL + 3)
#define B100_REG_TX_CTRL_CYCLES_PER_UP  B100_REG_SR_ADDR(B100_SR_TX_CTRL + 4)
#define B100_REG_TX_CTRL_PACKETS_PER_UP B100_REG_SR_ADDR(B100_SR_TX_CTRL + 5)

#define B100_FLAG_TX_CTRL_POLICY_WAIT          (0x1 << 0)
#define B100_FLAG_TX_CTRL_POLICY_NEXT_PACKET   (0x1 << 1)
#define B100_FLAG_TX_CTRL_POLICY_NEXT_BURST    (0x1 << 2)

/////////////////////////////////////////////////
// TX FE
////////////////////////////////////////////////
#define B100_REG_TX_FE_DC_OFFSET_I         B100_REG_SR_ADDR(B100_SR_TX_FRONT + 0) //24 bits
#define B100_REG_TX_FE_DC_OFFSET_Q         B100_REG_SR_ADDR(B100_SR_TX_FRONT + 1) //24 bits
#define B100_REG_TX_FE_MAC_CORRECTION      B100_REG_SR_ADDR(B100_SR_TX_FRONT + 2) //18 bits
#define B100_REG_TX_FE_PHASE_CORRECTION    B100_REG_SR_ADDR(B100_SR_TX_FRONT + 3) //18 bits
#define B100_REG_TX_FE_MUX                 B100_REG_SR_ADDR(B100_SR_TX_FRONT + 4) //8 bits (std output = 0x10, reversed = 0x01)

/////////////////////////////////////////////////
// VITA49 64 bit time (write only)
////////////////////////////////////////////////
#define B100_REG_TIME64_SECS      B100_REG_SR_ADDR(B100_SR_TIME64 + 0)
#define B100_REG_TIME64_TICKS     B100_REG_SR_ADDR(B100_SR_TIME64 + 1)
#define B100_REG_TIME64_FLAGS     B100_REG_SR_ADDR(B100_SR_TIME64 + 2)
#define B100_REG_TIME64_IMM       B100_REG_SR_ADDR(B100_SR_TIME64 + 3)
#define B100_REG_TIME64_TPS       B100_REG_SR_ADDR(B100_SR_TIME64 + 4)
#define B100_REG_TIME64_MIMO_SYNC B100_REG_SR_ADDR(B100_SR_TIME64 + 5)

//pps flags (see above)
#define B100_FLAG_TIME64_PPS_NEGEDGE (0 << 0)
#define B100_FLAG_TIME64_PPS_POSEDGE (1 << 0)
#define B100_FLAG_TIME64_PPS_SMA     (0 << 1)
#define B100_FLAG_TIME64_PPS_MIMO    (1 << 1)

#define B100_FLAG_TIME64_LATCH_NOW 1
#define B100_FLAG_TIME64_LATCH_NEXT_PPS 0

#endif

