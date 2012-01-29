

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

///////////////////////////////////////////////////
// Slave 7 -- Readback Mux 32

#define B100_REG_RB_MUX_32_BASE  B100_REG_SLAVE(7)

#define B100_REG_RB_TIME_NOW_SECS   B100_REG_RB_MUX_32_BASE + 0
#define B100_REG_RB_TIME_NOW_TICKS  B100_REG_RB_MUX_32_BASE + 4
#define B100_REG_RB_TIME_PPS_SECS   B100_REG_RB_MUX_32_BASE + 8
#define B100_REG_RB_TIME_PPS_TICKS  B100_REG_RB_MUX_32_BASE + 12
#define B100_REG_RB_MISC_TEST32     B100_REG_RB_MUX_32_BASE + 16
#define B100_REG_RB_COMPAT          B100_REG_RB_MUX_32_BASE + 24
#define B100_REG_RB_GPIO            B100_REG_RB_MUX_32_BASE + 28

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
#define B100_SR_USER_REGS 64     // 2 regs

#define B100_SR_GPIO 128

#define B100_REG_SR_ADDR(n) (B100_REG_SLAVE(8) + (4*(n)))

#define B100_REG_SR_MISC_TEST32        B100_REG_SR_ADDR(B100_SR_REG_TEST32)

/////////////////////////////////////////////////
// Magic reset regs
////////////////////////////////////////////////
#define B100_REG_CLEAR_RX           B100_REG_SR_ADDR(B100_SR_CLEAR_RX_FIFO)
#define B100_REG_CLEAR_TX           B100_REG_SR_ADDR(B100_SR_CLEAR_RX_FIFO)
#define B100_REG_GLOBAL_RESET       B100_REG_SR_ADDR(B100_SR_GLOBAL_RESET)

#endif

