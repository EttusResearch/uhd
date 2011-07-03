

////////////////////////////////////////////////////////////////
//
//         Memory map for wishbone bus
//
////////////////////////////////////////////////////////////////

// All addresses are byte addresses.  All accesses are word (16-bit) accesses.
//  This means that address bit 0 is usually 0.
//  There are 11 bits of address for the control.

#ifndef INCLUDED_B100_REGS_HPP
#define INCLUDED_B100_REGS_HPP

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

//spi slave constants
#define B100_SPI_SS_AD9862    (1 << 2)
#define B100_SPI_SS_TX_DB     (1 << 1)
#define B100_SPI_SS_RX_DB     (1 << 0)

////////////////////////////////////////////////
// Slave 3 -- I2C Core

#define B100_REG_I2C_BASE B100_REG_SLAVE(3)

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

#endif

