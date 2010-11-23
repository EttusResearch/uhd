

////////////////////////////////////////////////////////////////
//
//         Memory map for embedded wishbone bus
//
////////////////////////////////////////////////////////////////

// All addresses are byte addresses.  All accesses are word (16-bit) accesses.
//  This means that address bit 0 is usually 0.
//  There are 11 bits of address for the control.

#ifndef INCLUDED_USRP_E100_REGS_HPP
#define INCLUDED_USRP_E100_REGS_HPP

/////////////////////////////////////////////////////
// Slave pointers

#define UE_REG_SLAVE(n) ((n)<<7)
#define UE_REG_SR_ADDR(n) ((UE_REG_SLAVE(5)) + (4*(n)))

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
#define UE_REG_MISC_XFER_RATE  UE_REG_MISC_BASE + 14
#define UE_REG_MISC_COMPAT     UE_REG_MISC_BASE + 16

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

//possible bit values for sel when dbg is 0:
#define GPIO_SEL_SW    0 // if pin is an output, set by software in the io reg
#define GPIO_SEL_ATR   1 // if pin is an output, set by ATR logic

//possible bit values for sel when dbg is 1:
#define GPIO_SEL_DEBUG_0   0 // if pin is an output, debug lines from FPGA fabric
#define GPIO_SEL_DEBUG_1   1 // if pin is an output, debug lines from FPGA fabric


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

#define	UE_REG_ATR_IDLE_RXSIDE  UE_REG_ATR_BASE + 0
#define	UE_REG_ATR_IDLE_TXSIDE  UE_REG_ATR_BASE + 2
#define UE_REG_ATR_INTX_RXSIDE  UE_REG_ATR_BASE + 4
#define UE_REG_ATR_INTX_TXSIDE  UE_REG_ATR_BASE + 6
#define	UE_REG_ATR_INRX_RXSIDE  UE_REG_ATR_BASE + 8
#define	UE_REG_ATR_INRX_TXSIDE  UE_REG_ATR_BASE + 10
#define	UE_REG_ATR_FULL_RXSIDE  UE_REG_ATR_BASE + 12
#define	UE_REG_ATR_FULL_TXSIDE  UE_REG_ATR_BASE + 14

/////////////////////////////////////////////////
// DSP RX Regs
////////////////////////////////////////////////
#define UE_REG_DSP_RX_FREQ         UE_REG_SR_ADDR(0)
#define UE_REG_DSP_RX_SCALE_IQ     UE_REG_SR_ADDR(1)  // {scale_i,scale_q}
#define UE_REG_DSP_RX_DECIM_RATE   UE_REG_SR_ADDR(2)  // hb and decim rate
#define UE_REG_DSP_RX_DCOFFSET_I   UE_REG_SR_ADDR(3) // Bit 31 high sets fixed offset mode, using lower 14 bits, // otherwise it is automatic
#define UE_REG_DSP_RX_DCOFFSET_Q   UE_REG_SR_ADDR(4) // Bit 31 high sets fixed offset mode, using lower 14 bits
#define UE_REG_DSP_RX_MUX          UE_REG_SR_ADDR(5)

///////////////////////////////////////////////////
// VITA RX CTRL regs
///////////////////////////////////////////////////
// The following 3 are logically a single command register.
// They are clocked into the underlying fifo when time_ticks is written.
#define UE_REG_CTRL_RX_STREAM_CMD        UE_REG_SR_ADDR(8) // {now, chain, num_samples(30)
#define UE_REG_CTRL_RX_TIME_SECS         UE_REG_SR_ADDR(9)
#define UE_REG_CTRL_RX_TIME_TICKS        UE_REG_SR_ADDR(10)
#define UE_REG_CTRL_RX_CLEAR_OVERRUN     UE_REG_SR_ADDR(11) // write anything to clear overrun
#define UE_REG_CTRL_RX_VRT_HEADER        UE_REG_SR_ADDR(12) // word 0 of packet.  FPGA fills in packet counter
#define UE_REG_CTRL_RX_VRT_STREAM_ID     UE_REG_SR_ADDR(13) // word 1 of packet.
#define UE_REG_CTRL_RX_VRT_TRAILER       UE_REG_SR_ADDR(14)
#define UE_REG_CTRL_RX_NSAMPS_PER_PKT    UE_REG_SR_ADDR(15)
#define UE_REG_CTRL_RX_NCHANNELS         UE_REG_SR_ADDR(16) // 1 in basic case, up to 4 for vector sources

/////////////////////////////////////////////////
// DSP TX Regs
////////////////////////////////////////////////
#define UE_REG_DSP_TX_FREQ         UE_REG_SR_ADDR(17)
#define UE_REG_DSP_TX_SCALE_IQ     UE_REG_SR_ADDR(18) // {scale_i,scale_q}
#define UE_REG_DSP_TX_INTERP_RATE  UE_REG_SR_ADDR(19)
#define UE_REG_DSP_TX_UNUSED       UE_REG_SR_ADDR(20)
#define UE_REG_DSP_TX_MUX          UE_REG_SR_ADDR(21)

/////////////////////////////////////////////////
// VITA TX CTRL regs
////////////////////////////////////////////////
#define UE_REG_CTRL_TX_NCHANNELS         UE_REG_SR_ADDR(24)
#define UE_REG_CTRL_TX_CLEAR_UNDERRUN    UE_REG_SR_ADDR(25)
#define UE_REG_CTRL_TX_REPORT_SID        UE_REG_SR_ADDR(26)
#define UE_REG_CTRL_TX_POLICY            UE_REG_SR_ADDR(27)

#define UE_FLAG_CTRL_TX_POLICY_WAIT          (0x1 << 0)
#define UE_FLAG_CTRL_TX_POLICY_NEXT_PACKET   (0x1 << 1)
#define UE_FLAG_CTRL_TX_POLICY_NEXT_BURST    (0x1 << 2)

/////////////////////////////////////////////////
// VITA49 64 bit time (write only)
////////////////////////////////////////////////
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
#define UE_REG_TIME64_SECS  UE_REG_SR_ADDR(28)  // value to set absolute secs to on next PPS
#define UE_REG_TIME64_TICKS UE_REG_SR_ADDR(29)  // value to set absolute ticks to on next PPS
#define UE_REG_TIME64_FLAGS UE_REG_SR_ADDR(30)  // flags - see chart above
#define UE_REG_TIME64_IMM   UE_REG_SR_ADDR(31)  // set immediate (0=latch on next pps, 1=latch immediate, default=0)
#define UE_REG_TIME64_TPS   UE_REG_SR_ADDR(31)  // clock ticks per second (counter rollover)

//pps flags (see above)
#define UE_FLAG_TIME64_PPS_NEGEDGE (0 << 0)
#define UE_FLAG_TIME64_PPS_POSEDGE (1 << 0)
#define UE_FLAG_TIME64_PPS_SMA     (0 << 1)
#define UE_FLAG_TIME64_PPS_MIMO    (1 << 1)

#define UE_FLAG_TIME64_LATCH_NOW 1
#define UE_FLAG_TIME64_LATCH_NEXT_PPS 0

#endif

