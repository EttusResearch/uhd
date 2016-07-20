/*
 * Copyright 2007,2009 Free Software Foundation, Inc.
 * Copyright 2009 Ettus Research LLC
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "../x300/x300_defs.h"
#include "ethernet.h"
#include "cron.h"
#include <trace.h>
#include "wb_i2c.h"
#include "wb_utils.h"
//#include "memory_map.h"
//#include "eth_phy.h"
//#include "pic.h"
//#include "hal_io.h"
//#include "nonstdio.h"
#include <stdint.h>
#include <stdbool.h>
#include "xge_phy.h"
#include "xge_mac.h"
#include <u3_net_stack.h>




#define VERBOSE 0

#define	NETHS	2	// # of ethernet interfaces

static bool links_up[NETHS] = {};

////////////////////////////////////////////////////////////////////////
//
// 10 Gig Ethernet MAC.
//
typedef struct {
  volatile uint32_t config;             // WO
  volatile uint32_t int_pending;        // Clear-on-read
  volatile uint32_t int_status;         // RO
  volatile uint32_t int_mask;           // RW
  volatile uint32_t mdio_data;
  volatile uint32_t mdio_addr;
  volatile uint32_t mdio_op;
  volatile uint32_t mdio_control;
  volatile uint32_t gpio;
} xge_regs_t;

#define xge_regs ((xge_regs_t *) base)

#define SFPP_STATUS_MODABS_CHG     (1 << 5)    // Has MODABS changed since last read?
#define SFPP_STATUS_TXFAULT_CHG    (1 << 4)    // Has TXFAULT changed since last read?
#define SFPP_STATUS_RXLOS_CHG      (1 << 3)    // Has RXLOS changed since last read?
#define SFPP_STATUS_MODABS         (1 << 2)    // MODABS state
#define SFPP_STATUS_TXFAULT        (1 << 1)    // TXFAULT state
#define SFPP_STATUS_RXLOS          (1 << 0)    // RXLOS state


int
ethernet_ninterfaces(void)
{
  return NETHS;
}

////////////////////////////////////////////////////////////////////////
//
// Clause 45 MDIO used for 10Gig Ethernet has two bus transactions to complete a transfer.
// An initial transaction sets up the address, and a subsequent one transfers the read or write data.
//
static uint32_t
xge_read_mdio(const uint32_t base, const uint32_t address, const uint32_t device, const uint32_t port)
{
  // Set register address each iteration
  xge_regs->mdio_addr = address;
  // Its a clause 45 device. We want to ADDRESS
  xge_regs->mdio_op = XGE_MDIO_CLAUSE(CLAUSE45) | XGE_MDIO_OP(MDIO_ADDRESS) | XGE_MDIO_ADDR(port) | XGE_MDIO_MMD(device);
  // Start MDIO bus transaction
  xge_regs->mdio_control = 1;
  // Wait until bus transaction complete
  while (xge_regs->mdio_control == 1);
  // Its a clause 45 device. We want to READ
  xge_regs->mdio_op = XGE_MDIO_CLAUSE(CLAUSE45) | XGE_MDIO_OP(MDIO_READ) | XGE_MDIO_ADDR(port) | XGE_MDIO_MMD(device);
  // Start MDIO bus transaction
  xge_regs->mdio_control = 1;
  // Wait until bus transaction complete
  while (xge_regs->mdio_control == 1);
  // Read MDIO data
  return(xge_regs->mdio_data);
}

static void
xge_write_mdio(const uint32_t base, const uint32_t address, const uint32_t device, const uint32_t port, const uint32_t data)
{
  // Set register address each iteration
  xge_regs->mdio_addr = address;
  // Its a clause 45 device. We want to ADDRESS
  xge_regs->mdio_op = XGE_MDIO_CLAUSE(CLAUSE45) | XGE_MDIO_OP(MDIO_ADDRESS) | XGE_MDIO_ADDR(port) | XGE_MDIO_MMD(device);
  // Start MDIO bus transaction
  xge_regs->mdio_control = 1;
  // Wait until bus transaction complete
  while (xge_regs->mdio_control == 1);
  // Write new value to mdio_write_data reg.
  xge_regs->mdio_data = data;
  // Its a clause 45 device. We want to WRITE
  xge_regs->mdio_op = XGE_MDIO_CLAUSE(CLAUSE45) | XGE_MDIO_OP(MDIO_WRITE) | XGE_MDIO_ADDR(port) | XGE_MDIO_MMD(device);
  // Start MDIO bus transaction
  xge_regs->mdio_control = 1;
  // Wait until bus transaction complete
  while (xge_regs->mdio_control == 1);
}

////////////////////////////////////////////////////////////////////////
//
// Clause 22 MDIO used for 1Gig Ethernet has one bus transaction to complete a transfer.
//
static uint32_t
ge_read_mdio(const uint32_t base, const uint32_t address, const uint32_t port)
{
  // Its a clause 22 device. We want to READ
  xge_regs->mdio_op = XGE_MDIO_CLAUSE(CLAUSE22) | XGE_MDIO_OP(MDIO_C22_READ) | XGE_MDIO_ADDR(port) | address;
  // Start MDIO bus transaction
  xge_regs->mdio_control = 1;
  // Wait until bus transaction complete
  while (xge_regs->mdio_control == 1);
  // Read MDIO data
  return(xge_regs->mdio_data);
}

static void
ge_write_mdio(const uint32_t base, const uint32_t address, const uint32_t port, const uint32_t data)
{
  // Write new value to mdio_write_data reg.
  xge_regs->mdio_data = data;
  // Its a clause 22 device. We want to WRITE
  xge_regs->mdio_op = XGE_MDIO_CLAUSE(CLAUSE22) | XGE_MDIO_OP(MDIO_C22_WRITE) | XGE_MDIO_ADDR(port) | address;
  // Start MDIO bus transaction
  xge_regs->mdio_control = 1;
  // Wait until bus transaction complete
  while (xge_regs->mdio_control == 1);
}

////////////////////////////////////////////////////////////////////////
//
// Read and write MDIO independent of type
//

static uint32_t read_mdio(const uint8_t eth, const uint32_t address, const uint32_t device, const uint32_t port)
{
    const uint32_t rb_addr = (eth==0) ? RB_SFP0_TYPE : RB_SFP1_TYPE;
    const uint32_t base = (eth==0) ? SFP0_MAC_BASE : SFP1_MAC_BASE;
    if (wb_peek32(SR_ADDR(RB0_BASE, rb_addr)) != 0)
    {
        return xge_read_mdio(base, address, device, port);
    }
    else
    {
        return ge_read_mdio(base, address, port);
    }
}

static void write_mdio(const uint8_t eth, const uint32_t address, const uint32_t device, const uint32_t port, const uint32_t data)
{
    const uint32_t rb_addr = (eth==0) ? RB_SFP0_TYPE : RB_SFP1_TYPE;
    const uint32_t base = (eth==0) ? SFP0_MAC_BASE : SFP1_MAC_BASE;
    if (wb_peek32(SR_ADDR(RB0_BASE, rb_addr)) != 0)
    {
        return xge_write_mdio(base, address, device, port, data);
    }
    else
    {
        return ge_write_mdio(base, address, port, data);
    }
}

////////////////////////////////////////////////////////////////////////
//
// Read an 8-bit word from a device attached to the PHY's i2c bus.
//
static int
xge_i2c_rd(const uint32_t base, const uint8_t i2c_dev_addr, const uint8_t i2c_word_addr)
{
  uint8_t buf;
  // IJB. CHECK HERE FOR MODET. Bail immediately if no module

  // SFF-8472 defines a hardcoded bus address of 0xA0, an 8bit internal address and a register map.
  // Write the random access address to the SPF module
  if (wb_i2c_write(base, i2c_dev_addr, &i2c_word_addr, 1) == false)
    return(-1);

  // Now read back a byte of data
  if (wb_i2c_read(base, i2c_dev_addr, &buf, 1) == false)
    return(-1);

  return((int) buf);
}

////////////////////////////////////////////////////////////////////////
//
// Read identity of SFP+ module for XGE PHY
//
// (base is i2c controller)
static int
xge_read_sfpp_type(const uint32_t base, const uint32_t delay_ms)
{
  int x;
 // Delay read of SFPP
  if (delay_ms)
    sleep_ms(delay_ms);
  // Read ID code from SFP
  x = xge_i2c_rd(base, MODULE_DEV_ADDR, 3);
  // I2C Error?
  if (x < 0) {
    UHD_FW_TRACE(ERROR, "I2C error in SFPP_TYPE.");
    return x;
    }
  // Decode module type. These registers and values are defined in SFF-8472
  if (x & 0x01) // Active 1X Infinband Copper
    {
      goto twinax;
    }
  if (x & 0x10)
    {
      UHD_FW_TRACE(DEBUG, "SFFP_TYPE_SR.");
      return SFFP_TYPE_SR;
    }
  if (x & 0x20)
    {
      UHD_FW_TRACE(DEBUG, "SFFP_TYPE_LR.");
      return SFFP_TYPE_LR;
    }
  if (x & 0x40)
    {
      UHD_FW_TRACE(DEBUG, "SFFP_TYPE_LRM.");
      return SFFP_TYPE_LRM;
    }
  // Search for legacy 1000-Base SFP types
  x = xge_i2c_rd(base, MODULE_DEV_ADDR, 0x6);
  if (x < 0) {
    UHD_FW_TRACE(ERROR, "I2C error in SFPP_TYPE.");
    return x;
  }
  if (x & 0x01) {
    UHD_FW_TRACE(DEBUG, "SFFP_TYPE_1000BASE_SX.");
    return SFFP_TYPE_1000BASE_SX;
  }
  if (x & 0x02) {
    UHD_FW_TRACE(DEBUG, "SFFP_TYPE_1000BASE_LX.");
    return SFFP_TYPE_1000BASE_LX;
  }
  if (x & 0x08) {
    UHD_FW_TRACE(DEBUG, "SFFP_TYPE_1000BASE_T.");
    return SFFP_TYPE_1000BASE_T;
  }
  // Not one of the standard optical types..now try to deduce if it's twinax aka 10GSFP+CU
  // which is not covered explicitly in SFF-8472
  x = xge_i2c_rd(base, MODULE_DEV_ADDR, 8);
  if (x < 0) {
    UHD_FW_TRACE(ERROR, "I2C error in SFPP_TYPE.");
    return x;
  }
  if ((x & 4) == 0) // Passive SFP+ cable type
    goto unknown;
//  x = xge_i2c_rd(MODULE_DEV_ADDR, 6);
//   UHD_FW_TRACE(DEBUG, "SFP+ reg6 read as %x",x);
//  if (x < 0)
//    return x;
//  if (x != 0x04)  // Returns  1000Base-CX as Compliance code
//    goto unknown;
  x = xge_i2c_rd(base, MODULE_DEV_ADDR, 0xA);
  if (x < 0) {
    UHD_FW_TRACE(ERROR, "I2C error in SFPP_TYPE.");
    return x;
  }
  if (x & 0x80) {
  twinax:
    // Reports 1200 MBytes/sec fibre channel speed..close enough to 10G ethernet!
    x = xge_i2c_rd(base, MODULE_DEV_ADDR, 0x12);

    if (x < 0) {
      UHD_FW_TRACE(ERROR, "I2C error in SFPP_TYPE.");
      return x;
    }
    UHD_FW_TRACE(DEBUG, "TwinAx.");
    // If cable length support is greater than 10M then pick correct type
    return x > 10 ? SFFP_TYPE_TWINAX_LONG : SFFP_TYPE_TWINAX;
  }
unknown:
  UHD_FW_TRACE(WARN, "Unknown SFP+ type.");
  // Not a supported Module type
  return SFFP_TYPE_UNKNOWN;
}

static void xge_mac_init(const uint32_t base)
{
    UHD_FW_TRACE(DEBUG, "Begining XGE MAC init sequence.");
    xge_regs->config =  XGE_TX_ENABLE;
}

// base is pointer to XGE MAC on Wishbone.
static void xge_phy_init(const uint8_t eth, const uint32_t mdio_port)
{
    int x;
    // Read LASI Ctrl register to capture state.
    //y = xge_read_mdio(0x9002,XGE_MDIO_DEVICE_PMA,XGE_MDIO_ADDR_PHY_A);
    UHD_FW_TRACE(DEBUG, "Begining XGE PHY init sequence.");
    // Software reset
    x = read_mdio(eth, 0x0, XGE_MDIO_DEVICE_PMA,mdio_port);
    x = x | (1 << 15);
    write_mdio(eth, 0x0,XGE_MDIO_DEVICE_PMA,mdio_port,x);
    uint32_t loopCount = 0;
    while(x&(1<<15)) {
        x = read_mdio(eth, 0x0,XGE_MDIO_DEVICE_PMA,mdio_port);
        if( loopCount++ > 200 ) break; // usually succeeds after 22 or 23 polls
    }
}

void update_eth_state(const uint32_t eth, const uint32_t sfp_type)
{
    const bool old_link_up = links_up[eth];
    const uint32_t status_reg_addr = (eth==0) ? RB_SFP0_STATUS : RB_SFP1_STATUS;

    uint32_t sfpp_status = wb_peek32(SR_ADDR(RB0_BASE, status_reg_addr)) & 0xFFFF;
    if ((sfpp_status & (SFPP_STATUS_RXLOS|SFPP_STATUS_TXFAULT|SFPP_STATUS_MODABS)) == 0) {
        //SFP+ pin state changed. Reinitialize PHY and MAC
        if (sfp_type == RB_SFP_10G_ETH) {
            xge_mac_init((eth==0) ? SFP0_MAC_BASE : SFP1_MAC_BASE);
            xge_phy_init(eth, MDIO_PORT);
        } else {
            //No-op for 1G
        }

        int8_t timeout = 100;
        bool link_up = false;
        do {
            if (sfp_type == RB_SFP_10G_ETH) {
                link_up = ((read_mdio(eth, XGE_MDIO_STATUS1,XGE_MDIO_DEVICE_PMA,MDIO_PORT)) & (1 << 2)) != 0;
            } else {
                link_up = ((wb_peek32(SR_ADDR(RB0_BASE, status_reg_addr)) >> 16) & 0x1) != 0;
            }
        } while (!link_up && timeout-- > 0);

        links_up[eth] = link_up;
    }
    else
    {
        links_up[eth] = false;
    }

    if (!old_link_up && links_up[eth]) u3_net_stack_send_arp_request(eth, u3_net_stack_get_ip_addr(eth));
}

void poll_sfpp_status(const uint32_t sfp)
{
    uint32_t type = wb_peek32(SR_ADDR(RB0_BASE, (sfp==0) ? RB_SFP0_TYPE : RB_SFP1_TYPE));
    uint32_t status = wb_peek32(SR_ADDR(RB0_BASE, (sfp==0) ? RB_SFP0_STATUS : RB_SFP1_STATUS));

    if (status & SFPP_STATUS_MODABS_CHG) {
        // MODDET has changed state since last checked
        if (status & SFPP_STATUS_MODABS) {
            // MODDET is high, module currently removed.
            UHD_FW_TRACE_FSTR(INFO, "An SFP+ module has been removed from eth port %d.", sfp);
        } else {
            // MODDET is low, module currently inserted.
            // Return status.
            UHD_FW_TRACE_FSTR(INFO, "A new SFP+ module has been inserted into eth port %d.", sfp);
            if (type == RB_SFP_10G_ETH) {
                xge_read_sfpp_type((sfp==0) ? I2C0_BASE : I2C2_BASE,1);
            }
        }
    }

    if (status & SFPP_STATUS_RXLOS_CHG) {
        UHD_FW_TRACE_FSTR(DEBUG, "SFP%1d RXLOS changed state: %d", sfp, (status & SFPP_STATUS_RXLOS));
    }
    if (status & SFPP_STATUS_TXFAULT_CHG) {
        UHD_FW_TRACE_FSTR(DEBUG, "SFP%1d TXFAULT changed state: %d", sfp, ((status & SFPP_STATUS_TXFAULT) >> 1));
    }
    if (status & SFPP_STATUS_MODABS_CHG) {
        UHD_FW_TRACE_FSTR(DEBUG, "SFP%1d MODABS changed state: %d", sfp, ((status & SFPP_STATUS_MODABS) >> 2));
    }

    //update the link up status
    const bool old_link_up = links_up[sfp];
    if (type == RB_SFP_AURORA) {
        links_up[sfp] = ((wb_peek32(SR_ADDR(RB0_BASE, (sfp==0) ? RB_SFP0_STATUS : RB_SFP1_STATUS)) >> 16) & 0x1) != 0;
    } else {
        if ((status & SFPP_STATUS_RXLOS_CHG) ||
            (status & SFPP_STATUS_TXFAULT_CHG) ||
            (status & SFPP_STATUS_MODABS_CHG))
        {
            update_eth_state(sfp, type);
        }
    }
    if (old_link_up != links_up[sfp]) {
        UHD_FW_TRACE_FSTR(INFO, "The link on SFP port %u is %s", sfp, links_up[sfp]?"up":"down");
    }
}

void ethernet_init(const uint32_t sfp)
{
#ifdef UHD_FW_TRACE_LEVEL
    uint32_t x = wb_peek32(SR_ADDR(RB0_BASE, (sfp==0) ? RB_SFP0_STATUS : RB_SFP1_STATUS ));
    UHD_FW_TRACE_FSTR(DEBUG, "SFP%1d SFP initial state: RXLOS: %d  TXFAULT: %d  MODABS: %d",
        sfp,
        (x & SFPP_STATUS_RXLOS),
        ((x & SFPP_STATUS_TXFAULT) >> 1),
        ((x & SFPP_STATUS_MODABS) >> 2));
#endif
    update_eth_state(sfp, wb_peek32(SR_ADDR(RB0_BASE, (sfp==0) ? RB_SFP0_TYPE : RB_SFP1_TYPE)));
}

//
// Debug code to verbosely read XGE MDIO registers below here.
//


void decode_reg(uint32_t address, uint32_t device, uint32_t data)
{
    UHD_FW_TRACE_FSTR(DEBUG,
        "[MDIO Register Dump for Addr=%x, Device=%x]\n- Raw Value = %x",
        address, device, data);
  int x;
  switch(address) {
  case XGE_MDIO_CONTROL1:
    UHD_FW_TRACE_FSTR_SHORT(DEBUG, "CONTROL1: %x = ", data);
    for (x=15; x >= 0 ; x--)
      if ((data & (1 << x)) != 0)
	// Bits set.
	switch(x) {
	case 15: UHD_FW_TRACE_SHORT(DEBUG, "Reset,"); break;
	case 14: UHD_FW_TRACE_SHORT(DEBUG, "Loopback,"); break;
	case 11: UHD_FW_TRACE_SHORT(DEBUG, "Low Power Mode,"); break;
	case 5:case 4:case 3:case 2: UHD_FW_TRACE_SHORT(DEBUG, "RESERVED speed value,"); break;
	case 0: UHD_FW_TRACE_SHORT(DEBUG, "PMA loopback,"); break;
	} //else
	// Bits clear.
	//switch (x) {
	//case 13: case 6: UHD_FW_TRACE_SHORT(DEBUG, " None 10Gb/s speed set!"); break;
	//}
    UHD_FW_TRACE_SHORT(DEBUG, " \n");
    break;
  case XGE_MDIO_STATUS1:
    UHD_FW_TRACE_FSTR_SHORT(DEBUG, "STATUS1: %x = ", data);
    for (x=15; x >= 0 ; x--)
      if ((data & (1 << x)) != 0)
	// Bits set.
	switch(x) {
	case 7: UHD_FW_TRACE_SHORT(DEBUG, "Fault Detected,"); break;
	case 2: UHD_FW_TRACE_SHORT(DEBUG, "Link is Up,"); break;
	case 1: UHD_FW_TRACE_SHORT(DEBUG, "Supports Low Power,"); break;
	} else
	// Bits Clear
	switch(x) {
	case 2: UHD_FW_TRACE_SHORT(DEBUG, "Link is Down,"); break;
	}
    UHD_FW_TRACE_SHORT(DEBUG, " \n");
    break;
  case XGE_MDIO_SPEED:
    UHD_FW_TRACE_FSTR_SHORT(DEBUG, "SPEED ABILITY: %x = ", data);
    for (x=15; x >= 0 ; x--)
      if ((data & (1 << x)) != 0)
	// Bits set.
	switch(x) {
	case 15:case 14:case 13:case 12:case 11:case 10:case 9:
	case 8:case 7:case 6:case 5:case 4:case 3:case 2:case 1: UHD_FW_TRACE_SHORT(DEBUG, "RESERVED bits set!,"); break;
	case 0: UHD_FW_TRACE_SHORT(DEBUG, "Capable of 10Gb/s,");
	} else
	// Bits clear.
	switch(x) {
	case 0: UHD_FW_TRACE_SHORT(DEBUG, "Incapable of 10Gb/s,"); break;
	}
    UHD_FW_TRACE_SHORT(DEBUG, " \n");
    break;
  case XGE_MDIO_DEVICES1:
    UHD_FW_TRACE_FSTR_SHORT(DEBUG, "DEVICES IN PACKAGE: %x = ", data);
    for (x=15; x >= 0 ; x--)
      if ((data & (1 << x)) != 0)
	// Bits set.
	switch(x) {
	case 7: UHD_FW_TRACE_SHORT(DEBUG, "Auto-Negotiation,"); break;
	case 6: UHD_FW_TRACE_SHORT(DEBUG, "TC,"); break;
 	case 5: UHD_FW_TRACE_SHORT(DEBUG, "DTE XS,"); break;
	case 4: UHD_FW_TRACE_SHORT(DEBUG, "PHY XS,"); break;
	case 3: UHD_FW_TRACE_SHORT(DEBUG, "PCS,"); break;
	case 2: UHD_FW_TRACE_SHORT(DEBUG, "WIS,"); break;
	case 1: UHD_FW_TRACE_SHORT(DEBUG, "PMD/PMA,"); break;
	case 0: UHD_FW_TRACE_SHORT(DEBUG, "Clause 22 registers,"); break;
	}
    UHD_FW_TRACE_SHORT(DEBUG, " \n");
    break;
  case XGE_MDIO_DEVICES2:
    UHD_FW_TRACE_FSTR_SHORT(DEBUG, "DEVICES IN PACKAGE (cont): %x = ", data);
    for (x=15; x >= 0 ; x--)
      if ((data & (1 << x)) != 0)
	// Bits set.
	switch(x) {
	case 15: UHD_FW_TRACE_SHORT(DEBUG, "Vendor device 2,"); break;
	case 14: UHD_FW_TRACE_SHORT(DEBUG, "Vendor device 1,"); break;
	case 13: UHD_FW_TRACE_SHORT(DEBUG, "Clause 22 extension,"); break;
	}
    UHD_FW_TRACE_SHORT(DEBUG, " \n");
    break;
  case XGE_MDIO_CONTROL2:
    UHD_FW_TRACE_FSTR_SHORT(DEBUG, "CONTROL2: %x = ", data);
    // PMA/PMD
    if (device == XGE_MDIO_DEVICE_PMA)
      switch((data & 0xf)) {
      case 0xF: UHD_FW_TRACE_SHORT(DEBUG, "10BASE-T,"); break;
      case 0xE: UHD_FW_TRACE_SHORT(DEBUG, "100BASE-TX,"); break;
      case 0xD: UHD_FW_TRACE_SHORT(DEBUG, "1000BASE-KX,"); break;
      case 0xC: UHD_FW_TRACE_SHORT(DEBUG, "1000BASE-T,"); break;
      case 0xB: UHD_FW_TRACE_SHORT(DEBUG, "10GBASE-KR,"); break;
      case 0xA: UHD_FW_TRACE_SHORT(DEBUG, "10GBASE-KX4,"); break;
      case 0x9: UHD_FW_TRACE_SHORT(DEBUG, "10GBASE-T,"); break;
      case 0x8: UHD_FW_TRACE_SHORT(DEBUG, "10GBASE-LRM,"); break;
      case 0x7: UHD_FW_TRACE_SHORT(DEBUG, "10GBASE-SR,"); break;
      case 0x6: UHD_FW_TRACE_SHORT(DEBUG, "10GBASE-LR,"); break;
      case 0x5: UHD_FW_TRACE_SHORT(DEBUG, "10GBASE-ER,"); break;
      case 0x4: UHD_FW_TRACE_SHORT(DEBUG, "10GBASE-LX4,"); break;
	//     case 0x3: UHD_FW_TRACE_SHORT(DEBUG, "10GBASE-SW,"); break;
	//      case 0x2: UHD_FW_TRACE_SHORT(DEBUG, "10GBASE-LW,"); break;
	//     case 0x1: UHD_FW_TRACE_SHORT(DEBUG, "10GBASE-EW,"); break;
      case 0x0: UHD_FW_TRACE_SHORT(DEBUG, "10GBASE-CX4,"); break;
      } else if (device == XGE_MDIO_DEVICE_PCS)
      // PCS
      switch((data & 0x3)) {
      case 0x3: UHD_FW_TRACE_SHORT(DEBUG, "10GBASE-T PCS,"); break;
      case 0x2: UHD_FW_TRACE_SHORT(DEBUG, "10GBASE-W PCS,"); break;
      case 0x1: UHD_FW_TRACE_SHORT(DEBUG, "10GBASE-X PCS,"); break;
      case 0x0: UHD_FW_TRACE_SHORT(DEBUG, "10GBASE-R PCS,"); break;
      }
    UHD_FW_TRACE_SHORT(DEBUG, " \n");
    break;
  case XGE_MDIO_STATUS2:
    UHD_FW_TRACE_FSTR_SHORT(DEBUG, "STATUS2: %x = ", data);
    for (x=15; x >= 0 ; x--)
      if ((data & (1 << x)) != 0)
	// Bits set.
	switch(x) {
	case 15: if ((data & (1 << 14)) == 0) UHD_FW_TRACE_SHORT(DEBUG, "Device responding,"); break;
	case 13: if (device == XGE_MDIO_DEVICE_PMA) UHD_FW_TRACE_SHORT(DEBUG, "Able detect a Tx fault,"); break;
	case 12: if (device == XGE_MDIO_DEVICE_PMA) UHD_FW_TRACE_SHORT(DEBUG, "Able detect an Rx fault,"); break;
	case 11: UHD_FW_TRACE_SHORT(DEBUG, "Fault on Tx path,"); break;
	case 10: UHD_FW_TRACE_SHORT(DEBUG, "Fault on Rx path,"); break;
	case 9:  if (device == XGE_MDIO_DEVICE_PMA) UHD_FW_TRACE_SHORT(DEBUG, "Extended abilities in Reg1.11,"); break;
	case 8:  if (device == XGE_MDIO_DEVICE_PMA) UHD_FW_TRACE_SHORT(DEBUG, "Able to disable TX,"); break;
	case 7:  if (device == XGE_MDIO_DEVICE_PMA) UHD_FW_TRACE_SHORT(DEBUG, "10GBASE-SR,"); break;
	case 6:  if (device == XGE_MDIO_DEVICE_PMA) UHD_FW_TRACE_SHORT(DEBUG, "10GBASE-LR,"); break;
	case 5:  if (device == XGE_MDIO_DEVICE_PMA) UHD_FW_TRACE_SHORT(DEBUG, "10GBASE-ER,"); break;
	case 4:  if (device == XGE_MDIO_DEVICE_PMA) UHD_FW_TRACE_SHORT(DEBUG, "10GBASE-LX4,"); break;
	case 3:  if (device == XGE_MDIO_DEVICE_PMA) UHD_FW_TRACE_SHORT(DEBUG, "10GBASE-SW,"); break;
	case 2:  if (device == XGE_MDIO_DEVICE_PMA) UHD_FW_TRACE_SHORT(DEBUG, "10GBASE-LW,"); break;
	case 1:  if (device == XGE_MDIO_DEVICE_PMA) UHD_FW_TRACE_SHORT(DEBUG, "10GBASE-EW,"); break;
	case 0:  if (device == XGE_MDIO_DEVICE_PMA) UHD_FW_TRACE_SHORT(DEBUG, "loopback,"); break;
	}
    UHD_FW_TRACE_SHORT(DEBUG, " \n");
    break;
  case XGE_MDIO_LANESTATUS:
    UHD_FW_TRACE_FSTR_SHORT(DEBUG, "LANE STATUS: %x = ", data);
    for (x=15; x >= 0 ; x--)
      if ((data & (1 << x)) != 0)
	// Bits set.
	switch(x) {
	case 12: UHD_FW_TRACE_SHORT(DEBUG, "Lanes aligned,"); break;
	case 11: UHD_FW_TRACE_SHORT(DEBUG, "Able to generate test patterns,"); break;
	case 3:  UHD_FW_TRACE_SHORT(DEBUG, "Lane 3 synced,"); break;
	case 2:  UHD_FW_TRACE_SHORT(DEBUG, "Lane 2 synced,"); break;
	case 1:  UHD_FW_TRACE_SHORT(DEBUG, "Lane 1 synced,"); break;
	case 0:  UHD_FW_TRACE_SHORT(DEBUG, "Lane 0 synced,"); break;
	} else
	// Bits clear
	switch(x) {
 	case 3:  UHD_FW_TRACE_SHORT(DEBUG, "Lane 3 not synced,"); break;
	case 2:  UHD_FW_TRACE_SHORT(DEBUG, "Lane 2 not synced,"); break;
	case 1:  UHD_FW_TRACE_SHORT(DEBUG, "Lane 1 not synced,"); break;
	case 0:  UHD_FW_TRACE_SHORT(DEBUG, "Lane 0 not synced,"); break;
	}
    UHD_FW_TRACE_SHORT(DEBUG, " \n");
    break;
  case XILINX_CORE_VERSION:
    UHD_FW_TRACE_FSTR_SHORT(DEBUG, "XILINX CORE VERSION: %x  ",data);
    UHD_FW_TRACE_FSTR_SHORT(DEBUG, "Version: %d.%d ",((data&0xf000)>>12),((data&0xf00)>>8));
    UHD_FW_TRACE_FSTR_SHORT(DEBUG, "Patch: %d ",((data&0xE)>>1));
    UHD_FW_TRACE_SHORT(DEBUG, " \n");
    if (data&0x1) UHD_FW_TRACE(WARN, "Evaluation Version of core");
    break;
  default:
    UHD_FW_TRACE_SHORT(DEBUG, "Register @ address: ");
    UHD_FW_TRACE_FSTR_SHORT(DEBUG, "%x",address);
    UHD_FW_TRACE_SHORT(DEBUG, " has value: ");
    UHD_FW_TRACE_FSTR_SHORT(DEBUG, "%x\n",data);
    break;
  }
}

void
dump_mdio_regs(const uint8_t eth, uint32_t mdio_port)
{
    volatile unsigned int x;
    int y;
    unsigned int regs_a[9] = {0,1,4,5,6,7,8,32,33};
    unsigned int regs_b[10] = {0,1,4,5,6,7,8,10,11,65535};


    for (y = 0; y < 10; y++)
	{
	  // Read MDIO data
	  x = read_mdio(eth,regs_b[y],XGE_MDIO_DEVICE_PMA,mdio_port);
	  decode_reg(regs_b[y],XGE_MDIO_DEVICE_PMA,x);
	}

      for (y = 0; y < 9; y++)
	{
	  // Read MDIO data
	  x = read_mdio(eth,regs_a[y],XGE_MDIO_DEVICE_PCS,mdio_port);
	  decode_reg(regs_a[y],XGE_MDIO_DEVICE_PCS,x);
	}


      /* for (y = 0; y < 8; y++) */
      /* 	{ */
      /* 	  // Read MDIO data */
      /* 	  x = xge_read_mdio(base,regs_a[y],XGE_MDIO_DEVICE_PHY_XS,mdio_port); */
      /* 	  decode_reg(regs_a[y],XGE_MDIO_DEVICE_PHY_XS,x); */
      /* 	}  */

      /* for (y = 0; y < 8; y++) */
      /* 	{ */
      /* 	  // Read MDIO data */
      /* 	  x = xge_read_mdio(base,regs_a[y],XGE_MDIO_DEVICE_DTE_XS,mdio_port); */
      /* 	  decode_reg(regs_a[y],XGE_MDIO_DEVICE_DTE_XS,x); */
      /* 	} */
}

bool ethernet_get_link_up(const uint32_t eth)
{
    return links_up[eth];
}
