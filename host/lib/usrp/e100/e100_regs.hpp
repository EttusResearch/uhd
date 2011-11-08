

////////////////////////////////////////////////////////////////
//
//         Memory map for embedded wishbone bus
//
////////////////////////////////////////////////////////////////

// All addresses are byte addresses.  All accesses are word (16-bit) accesses.
//  This means that address bit 0 is usually 0.
//  There are 11 bits of address for the control.

#ifndef INCLUDED_E100_REGS_HPP
#define INCLUDED_E100_REGS_HPP

/////////////////////////////////////////////////////
// Slave pointers

#define E100_REG_SLAVE(n) ((n)<<7)

/////////////////////////////////////////////////////
// Slave 0 -- Misc Regs

#define E100_REG_MISC_BASE E100_REG_SLAVE(0)

#define E100_REG_MISC_LED        E100_REG_MISC_BASE + 0
#define E100_REG_MISC_SW         E100_REG_MISC_BASE + 2
#define E100_REG_MISC_CGEN_CTRL  E100_REG_MISC_BASE + 4
#define E100_REG_MISC_CGEN_ST    E100_REG_MISC_BASE + 6
#define E100_REG_MISC_TEST       E100_REG_MISC_BASE + 8
#define E100_REG_MISC_RX_LEN     E100_REG_MISC_BASE + 10
#define E100_REG_MISC_TX_LEN     E100_REG_MISC_BASE + 12
#define E100_REG_MISC_XFER_RATE  E100_REG_MISC_BASE + 14

/////////////////////////////////////////////////////
// Slave 1 -- UART
//   CLKDIV is 16 bits, others are only 8

#define E100_REG_UART_BASE E100_REG_SLAVE(1)

#define E100_REG_UART_CLKDIV  E100_REG_UART_BASE + 0
#define E100_REG_UART_TXLEVEL E100_REG_UART_BASE + 2
#define E100_REG_UART_RXLEVEL E100_REG_UART_BASE + 4
#define E100_REG_UART_TXCHAR  E100_REG_UART_BASE + 6
#define E100_REG_UART_RXCHAR  E100_REG_UART_BASE + 8

/////////////////////////////////////////////////////
// Slave 2 -- SPI Core
//these are 32-bit registers mapped onto the 16-bit Wishbone bus.
//Using peek32/poke32 should allow transparent use of these registers.
#define E100_REG_SPI_BASE E100_REG_SLAVE(2)

//spi slave constants
#define UE_SPI_SS_AD9522    (1 << 3)
#define UE_SPI_SS_AD9862    (1 << 2)
#define UE_SPI_SS_TX_DB     (1 << 1)
#define UE_SPI_SS_RX_DB     (1 << 0)

////////////////////////////////////////////////
// Slave 3 -- I2C Core

#define E100_REG_I2C_BASE E100_REG_SLAVE(3)

////////////////////////////////////////////////
// Slave 5 -- Error messages buffer

#define E100_REG_ERR_BUFF E100_REG_SLAVE(5)

///////////////////////////////////////////////////
// Slave 7 -- Readback Mux 32

#define E100_REG_RB_MUX_32_BASE  E100_REG_SLAVE(7)

#define E100_REG_RB_TIME_NOW_SECS   E100_REG_RB_MUX_32_BASE + 0
#define E100_REG_RB_TIME_NOW_TICKS  E100_REG_RB_MUX_32_BASE + 4
#define E100_REG_RB_TIME_PPS_SECS   E100_REG_RB_MUX_32_BASE + 8
#define E100_REG_RB_TIME_PPS_TICKS  E100_REG_RB_MUX_32_BASE + 12
#define E100_REG_RB_MISC_TEST32     E100_REG_RB_MUX_32_BASE + 16
#define E100_REG_RB_ERR_STATUS      E100_REG_RB_MUX_32_BASE + 20
#define E100_REG_RB_COMPAT          E100_REG_RB_MUX_32_BASE + 24
#define E100_REG_RB_GPIO            E100_REG_RB_MUX_32_BASE + 28

////////////////////////////////////////////////////
// Slave 8 -- Settings Bus
//
// Output-only, no readback, 64 registers total
//  Each register must be written 64 bits at a time
//  First the address xxx_xx00 and then xxx_xx10

// 64 total regs in address space
#define UE_SR_RX_CTRL0 0       // 9 regs (+0 to +8)
#define UE_SR_RX_DSP0 10       // 4 regs (+0 to +3)
#define UE_SR_RX_CTRL1 16      // 9 regs (+0 to +8)
#define UE_SR_RX_DSP1 26       // 4 regs (+0 to +3)
#define UE_SR_ERR_CTRL 30      // 1 reg
#define UE_SR_TX_CTRL 32       // 4 regs (+0 to +3)
#define UE_SR_TX_DSP 38        // 3 regs (+0 to +2)

#define UE_SR_TIME64 42        // 6 regs (+0 to +5)
#define UE_SR_RX_FRONT 48      // 5 regs (+0 to +4)
#define UE_SR_TX_FRONT 54      // 5 regs (+0 to +4)

#define UE_SR_REG_TEST32 60    // 1 reg
#define UE_SR_CLEAR_RX_FIFO 61 // 1 reg
#define UE_SR_CLEAR_TX_FIFO 62 // 1 reg
#define UE_SR_GLOBAL_RESET 63  // 1 reg

#define UE_SR_GPIO 128

#define E100_REG_SR_ADDR(n) (E100_REG_SLAVE(8) + (4*(n)))

#define E100_REG_SR_MISC_TEST32        E100_REG_SR_ADDR(UE_SR_REG_TEST32)
#define E100_REG_SR_ERR_CTRL           E100_REG_SR_ADDR(UE_SR_ERR_CTRL)

/////////////////////////////////////////////////
// Magic reset regs
////////////////////////////////////////////////
#define E100_REG_CLEAR_RX           E100_REG_SR_ADDR(UE_SR_CLEAR_RX_FIFO)
#define E100_REG_CLEAR_TX           E100_REG_SR_ADDR(UE_SR_CLEAR_RX_FIFO)
#define E100_REG_GLOBAL_RESET       E100_REG_SR_ADDR(UE_SR_GLOBAL_RESET)

#endif

