

////////////////////////////////////////////////////////////////
//
//         Memory map for embedded wishbone bus
//
////////////////////////////////////////////////////////////////

// All addresses are byte addresses.  All accesses are word (16-bit) accesses.
//  This means that address bit 0 is usually 0.
//  There are 11 bits of address for the control.

#ifndef __USRP_E_REGS_H
#define __USRP_E_REGS_H

/////////////////////////////////////////////////////
// Slave pointers

#define UE_REG_SLAVE(n) ((n)<<7)

/////////////////////////////////////////////////////
// Slave 0 -- Misc Regs

#define UE_REG_MISC_BASE UE_REG_SLAVE(0)

#define UE_REG_MISC_LED        UE_REG_MISC_BASE + 0
#define UE_REG_MISC_SW         UE_REG_MISC_BASE + 2
#define UE_REG_MISC_CGEN_CTRL  UE_REG_MISC_BASE + 4
#define UE_REG_MISC_CGEN_ST    UE_REG_MISC_BASE + 6
#define UE_REG_MISC_TEST       UE_REG_MISC_BASE + 8
#define UE_REG_MISC_RX_LEN     UE_REG_MISC_BASE + 10
#define UE_REG_MISC_TX_LEN     UE_REG_MISC_BASE + 12

/////////////////////////////////////////////////////
// Slave 1 -- UART
//   CLKDIV is 16 bits, others are only 8

#define UE_REG_UART_BASE UE_REG_SLAVE(1)

#define UE_REG_UART_CLKDIV  UE_REG_UART_BASE + 0
#define UE_REG_UART_TXLEVEL UE_REG_UART_BASE + 2
#define UE_REG_UART_RXLEVEL UE_REG_UART_BASE + 4
#define UE_REG_UART_TXCHAR  UE_REG_UART_BASE + 6
#define UE_REG_UART_RXCHAR  UE_REG_UART_BASE + 8

/////////////////////////////////////////////////////
// Slave 2 -- SPI Core
//   This should be accessed through the IOCTL
//   Users should not touch directly

#define UE_REG_SPI_BASE UE_REG_SLAVE(2)

//spi slave constants
#define UE_SPI_SS_AD9522    (1 << 3)
#define UE_SPI_SS_AD9862    (1 << 2)
#define UE_SPI_SS_TX_DB     (1 << 1)
#define UE_SPI_SS_RX_DB     (1 << 0)

////////////////////////////////////////////////
// Slave 3 -- I2C Core
//   This should be accessed through the IOCTL
//   Users should not touch directly

#define UE_REG_I2C_BASE UE_REG_SLAVE(3)


////////////////////////////////////////////////
// Slave 4 -- GPIO

#define UE_REG_GPIO_BASE UE_REG_SLAVE(4)

#define UE_REG_GPIO_RX_IO      UE_REG_GPIO_BASE + 0
#define UE_REG_GPIO_TX_IO      UE_REG_GPIO_BASE + 2
#define UE_REG_GPIO_RX_DDR     UE_REG_GPIO_BASE + 4
#define UE_REG_GPIO_TX_DDR     UE_REG_GPIO_BASE + 6
#define UE_REG_GPIO_RX_SEL     UE_REG_GPIO_BASE + 8
#define UE_REG_GPIO_TX_SEL     UE_REG_GPIO_BASE + 10
#define UE_REG_GPIO_RX_DBG     UE_REG_GPIO_BASE + 12
#define UE_REG_GPIO_TX_DBG     UE_REG_GPIO_BASE + 14

// each 2-bit sel field is layed out this way
#define GPIO_SEL_SW	   0 // if pin is an output, set by software in the io reg
#define	GPIO_SEL_ATR	   1 // if pin is an output, set by ATR logic
#define	GPIO_SEL_DEBUG_0   0 // if pin is an output, debug lines from FPGA fabric
#define	GPIO_SEL_DEBUG_1   1 // if pin is an output, debug lines from FPGA fabric


////////////////////////////////////////////////////
// Slave 5 -- Settings Bus
//
// Output-only, no readback, 32 registers total
//  Each register must be written 32 bits at a time
//  First the address xxx_xx00 and then xxx_xx10

#define UE_REG_SETTINGS_BASE UE_REG_SLAVE(5)

///////////////////////////////////////////////////
// Slave 6 -- ATR Controller
//   16 regs

#define UE_REG_ATR_BASE  UE_REG_SLAVE(6)

#define	UE_REG_ATR_IDLE_RXSIDE	UE_REG_ATR_BASE + 0
#define	UE_REG_ATR_IDLE_TXSIDE	UE_REG_ATR_BASE + 2
#define UE_REG_ATR_INTX_RXSIDE  UE_REG_ATR_BASE + 4
#define UE_REG_ATR_INTX_TXSIDE  UE_REG_ATR_BASE + 6
#define	UE_REG_ATR_INRX_RXSIDE  UE_REG_ATR_BASE + 8
#define	UE_REG_ATR_INRX_TXSIDE  UE_REG_ATR_BASE + 10
#define	UE_REG_ATR_FULL_RXSIDE  UE_REG_ATR_BASE + 12
#define	UE_REG_ATR_FULL_TXSIDE  UE_REG_ATR_BASE + 14

#endif

