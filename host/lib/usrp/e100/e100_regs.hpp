

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
#define E100_REG_MISC_COMPAT     E100_REG_MISC_BASE + 16

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
//   This should be accessed through the IOCTL
//   Users should not touch directly

#define E100_REG_SPI_BASE E100_REG_SLAVE(2)

//spi slave constants
#define UE_SPI_SS_AD9522    (1 << 3)
#define UE_SPI_SS_AD9862    (1 << 2)
#define UE_SPI_SS_TX_DB     (1 << 1)
#define UE_SPI_SS_RX_DB     (1 << 0)

////////////////////////////////////////////////
// Slave 3 -- I2C Core
//   This should be accessed through the IOCTL
//   Users should not touch directly

#define E100_REG_I2C_BASE E100_REG_SLAVE(3)


////////////////////////////////////////////////
// Slave 4 -- GPIO

#define E100_REG_GPIO_BASE E100_REG_SLAVE(4)

#define E100_REG_GPIO_RX_IO      E100_REG_GPIO_BASE + 0
#define E100_REG_GPIO_TX_IO      E100_REG_GPIO_BASE + 2
#define E100_REG_GPIO_RX_DDR     E100_REG_GPIO_BASE + 4
#define E100_REG_GPIO_TX_DDR     E100_REG_GPIO_BASE + 6
#define E100_REG_GPIO_RX_SEL     E100_REG_GPIO_BASE + 8
#define E100_REG_GPIO_TX_SEL     E100_REG_GPIO_BASE + 10
#define E100_REG_GPIO_RX_DBG     E100_REG_GPIO_BASE + 12
#define E100_REG_GPIO_TX_DBG     E100_REG_GPIO_BASE + 14

//possible bit values for sel when dbg is 0:
#define GPIO_SEL_SW    0 // if pin is an output, set by software in the io reg
#define GPIO_SEL_ATR   1 // if pin is an output, set by ATR logic

//possible bit values for sel when dbg is 1:
#define GPIO_SEL_DEBUG_0   0 // if pin is an output, debug lines from FPGA fabric
#define GPIO_SEL_DEBUG_1   1 // if pin is an output, debug lines from FPGA fabric

///////////////////////////////////////////////////
// Slave 6 -- ATR Controller
//   16 regs

#define E100_REG_ATR_BASE  E100_REG_SLAVE(6)

#define	E100_REG_ATR_IDLE_RXSIDE  E100_REG_ATR_BASE + 0
#define	E100_REG_ATR_IDLE_TXSIDE  E100_REG_ATR_BASE + 2
#define E100_REG_ATR_INTX_RXSIDE  E100_REG_ATR_BASE + 4
#define E100_REG_ATR_INTX_TXSIDE  E100_REG_ATR_BASE + 6
#define	E100_REG_ATR_INRX_RXSIDE  E100_REG_ATR_BASE + 8
#define	E100_REG_ATR_INRX_TXSIDE  E100_REG_ATR_BASE + 10
#define	E100_REG_ATR_FULL_RXSIDE  E100_REG_ATR_BASE + 12
#define	E100_REG_ATR_FULL_TXSIDE  E100_REG_ATR_BASE + 14

///////////////////////////////////////////////////
// Slave 7 -- Readback Mux 32

#define E100_REG_RB_MUX_32_BASE  E100_REG_SLAVE(7)

#define E100_REG_RB_TIME_NOW_SECS   E100_REG_RB_MUX_32_BASE + 0
#define E100_REG_RB_TIME_NOW_TICKS  E100_REG_RB_MUX_32_BASE + 4
#define E100_REG_RB_TIME_PPS_SECS   E100_REG_RB_MUX_32_BASE + 8
#define E100_REG_RB_TIME_PPS_TICKS  E100_REG_RB_MUX_32_BASE + 12
#define E100_REG_RB_MISC_TEST32     E100_REG_RB_MUX_32_BASE + 16

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
#define UE_SR_TX_CTRL 32       // 4 regs (+0 to +3)
#define UE_SR_TX_DSP 38        // 3 regs (+0 to +2)

#define UE_SR_TIME64 42        // 6 regs (+0 to +5)
#define UE_SR_RX_FRONT 48      // 5 regs (+0 to +4)
#define UE_SR_TX_FRONT 54      // 5 regs (+0 to +4)

#define UE_SR_REG_TEST32 60    // 1 reg
#define UE_SR_CLEAR_RX_FIFO 61 // 1 reg
#define UE_SR_CLEAR_TX_FIFO 62 // 1 reg
#define UE_SR_GLOBAL_RESET 63  // 1 reg

#define E100_REG_SR_ADDR(n) (E100_REG_SLAVE(8) + (4*(n)))

#define E100_REG_SR_MISC_TEST32        E100_REG_SR_ADDR(UE_SR_REG_TEST32)

/////////////////////////////////////////////////
// Magic reset regs
////////////////////////////////////////////////
#define E100_REG_CLEAR_RX           E100_REG_SR_ADDR(UE_SR_CLEAR_RX_FIFO)
#define E100_REG_CLEAR_TX           E100_REG_SR_ADDR(UE_SR_CLEAR_RX_FIFO)
#define E100_REG_GLOBAL_RESET       E100_REG_SR_ADDR(UE_SR_GLOBAL_RESET)

/////////////////////////////////////////////////
// DSP RX Regs
////////////////////////////////////////////////
#define E100_REG_DSP_RX_HELPER(which, offset) ((which == 0)? \
    (E100_REG_SR_ADDR(UE_SR_RX_DSP0 + offset)) : \
    (E100_REG_SR_ADDR(UE_SR_RX_DSP1 + offset)))

#define E100_REG_DSP_RX_FREQ(which)       E100_REG_DSP_RX_HELPER(which, 0)
#define E100_REG_DSP_RX_DECIM(which)      E100_REG_DSP_RX_HELPER(which, 2)
#define E100_REG_DSP_RX_MUX(which)        E100_REG_DSP_RX_HELPER(which, 3)

#define E100_FLAG_DSP_RX_MUX_SWAP_IQ   (1 << 0)
#define E100_FLAG_DSP_RX_MUX_REAL_MODE (1 << 1)

///////////////////////////////////////////////////
// RX CTRL regs
///////////////////////////////////////////////////
#define E100_REG_RX_CTRL_HELPER(which, offset) ((which == 0)? \
    (E100_REG_SR_ADDR(UE_SR_RX_CTRL0 + offset)) : \
    (E100_REG_SR_ADDR(UE_SR_RX_CTRL1 + offset)))

#define E100_REG_RX_CTRL_STREAM_CMD(which)     E100_REG_RX_CTRL_HELPER(which, 0)
#define E100_REG_RX_CTRL_TIME_SECS(which)      E100_REG_RX_CTRL_HELPER(which, 1)
#define E100_REG_RX_CTRL_TIME_TICKS(which)     E100_REG_RX_CTRL_HELPER(which, 2)
#define E100_REG_RX_CTRL_CLEAR(which)          E100_REG_RX_CTRL_HELPER(which, 3)
#define E100_REG_RX_CTRL_VRT_HDR(which)        E100_REG_RX_CTRL_HELPER(which, 4)
#define E100_REG_RX_CTRL_VRT_SID(which)        E100_REG_RX_CTRL_HELPER(which, 5)
#define E100_REG_RX_CTRL_VRT_TLR(which)        E100_REG_RX_CTRL_HELPER(which, 6)
#define E100_REG_RX_CTRL_NSAMPS_PP(which)      E100_REG_RX_CTRL_HELPER(which, 7)
#define E100_REG_RX_CTRL_NCHANNELS(which)      E100_REG_RX_CTRL_HELPER(which, 8)

/////////////////////////////////////////////////
// RX FE
////////////////////////////////////////////////
#define E100_REG_RX_FE_SWAP_IQ             E100_REG_SR_ADDR(UE_SR_RX_FRONT + 0) //lower bit
#define E100_REG_RX_FE_MAG_CORRECTION      E100_REG_SR_ADDR(UE_SR_RX_FRONT + 1) //18 bits
#define E100_REG_RX_FE_PHASE_CORRECTION    E100_REG_SR_ADDR(UE_SR_RX_FRONT + 2) //18 bits
#define E100_REG_RX_FE_OFFSET_I            E100_REG_SR_ADDR(UE_SR_RX_FRONT + 3) //18 bits
#define E100_REG_RX_FE_OFFSET_Q            E100_REG_SR_ADDR(UE_SR_RX_FRONT + 4) //18 bits

/////////////////////////////////////////////////
// DSP TX Regs
////////////////////////////////////////////////
#define E100_REG_DSP_TX_FREQ          E100_REG_SR_ADDR(UE_SR_TX_DSP + 0)
#define E100_REG_DSP_TX_SCALE_IQ      E100_REG_SR_ADDR(UE_SR_TX_DSP + 1)
#define E100_REG_DSP_TX_INTERP_RATE   E100_REG_SR_ADDR(UE_SR_TX_DSP + 2)

///////////////////////////////////////////////////
// TX CTRL regs
///////////////////////////////////////////////////
#define E100_REG_TX_CTRL_NUM_CHAN       E100_REG_SR_ADDR(UE_SR_TX_CTRL + 0)
#define E100_REG_TX_CTRL_CLEAR_STATE    E100_REG_SR_ADDR(UE_SR_TX_CTRL + 1)
#define E100_REG_TX_CTRL_REPORT_SID     E100_REG_SR_ADDR(UE_SR_TX_CTRL + 2)
#define E100_REG_TX_CTRL_POLICY         E100_REG_SR_ADDR(UE_SR_TX_CTRL + 3)
#define E100_REG_TX_CTRL_CYCLES_PER_UP  E100_REG_SR_ADDR(UE_SR_TX_CTRL + 4)
#define E100_REG_TX_CTRL_PACKETS_PER_UP E100_REG_SR_ADDR(UE_SR_TX_CTRL + 5)

#define E100_FLAG_TX_CTRL_POLICY_WAIT          (0x1 << 0)
#define E100_FLAG_TX_CTRL_POLICY_NEXT_PACKET   (0x1 << 1)
#define E100_FLAG_TX_CTRL_POLICY_NEXT_BURST    (0x1 << 2)

/////////////////////////////////////////////////
// TX FE
////////////////////////////////////////////////
#define E100_REG_TX_FE_DC_OFFSET_I         E100_REG_SR_ADDR(UE_SR_TX_FRONT + 0) //24 bits
#define E100_REG_TX_FE_DC_OFFSET_Q         E100_REG_SR_ADDR(UE_SR_TX_FRONT + 1) //24 bits
#define E100_REG_TX_FE_MAC_CORRECTION      E100_REG_SR_ADDR(UE_SR_TX_FRONT + 2) //18 bits
#define E100_REG_TX_FE_PHASE_CORRECTION    E100_REG_SR_ADDR(UE_SR_TX_FRONT + 3) //18 bits
#define E100_REG_TX_FE_MUX                 E100_REG_SR_ADDR(UE_SR_TX_FRONT + 4) //8 bits (std output = 0x10, reversed = 0x01)

/////////////////////////////////////////////////
// VITA49 64 bit time (write only)
////////////////////////////////////////////////
#define E100_REG_TIME64_SECS      E100_REG_SR_ADDR(UE_SR_TIME64 + 0)
#define E100_REG_TIME64_TICKS     E100_REG_SR_ADDR(UE_SR_TIME64 + 1)
#define E100_REG_TIME64_FLAGS     E100_REG_SR_ADDR(UE_SR_TIME64 + 2)
#define E100_REG_TIME64_IMM       E100_REG_SR_ADDR(UE_SR_TIME64 + 3)
#define E100_REG_TIME64_TPS       E100_REG_SR_ADDR(UE_SR_TIME64 + 4)
#define E100_REG_TIME64_MIMO_SYNC E100_REG_SR_ADDR(UE_SR_TIME64 + 5)

//pps flags (see above)
#define E100_FLAG_TIME64_PPS_NEGEDGE (0 << 0)
#define E100_FLAG_TIME64_PPS_POSEDGE (1 << 0)
#define E100_FLAG_TIME64_PPS_SMA     (0 << 1)
#define E100_FLAG_TIME64_PPS_MIMO    (1 << 1)

#define E100_FLAG_TIME64_LATCH_NOW 1
#define E100_FLAG_TIME64_LATCH_NEXT_PPS 0

#endif

