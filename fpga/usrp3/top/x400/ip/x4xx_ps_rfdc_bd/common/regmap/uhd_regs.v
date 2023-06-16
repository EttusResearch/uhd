//
// Copyright 2021 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: uhd_regs
// Description:
//   Registers definition within the x4xx_ps_rfdc_bd IP.

//XmlParse xml_on
//<regmap name="CMAC_REGMAP" markdown="true" generateverilog="false">
//  <group name="XILINX_CMAC_REGISTERS">
//      <info>
//        100G MAC ethernet registers (Link 0) defined in the CMAC Manual starting on pg 187.
//
//        - http://www.xilinx.com/support/documentation/ip_documentation/cmac_usplus/v2_4/pg203-cmac-usplus.pdf
//
//      </info>
//  </group>
//</regmap>

//XmlParse xml_on
//<regmap name="XGE_MAC_REGMAP" markdown="true" generateverilog="false">
//  <group name="OPENCORE_XGE_REGISTERS">
//      <info>
//
//        10G MAC ethernet registers defined in the USRP OSS distribution fpga/usrp3/lib/xge/doc/xge_mac_spec.pdf
//
//      </info>
//  </group>
//</regmap>


//<regmap name="DMA_REGMAP" markdown="true" generateverilog="false">
//  <group name="XILINX_DMA_REGISTERS">
//      <info>
//        Scatter Gather DMA block defined in Xilinx DMA manual start on pg 11
//
//        - https://www.xilinx.com/support/documentation/ip_documentation/axi_dma/v7_1/pg021_axi_dma.pdf
//
//      </info>
//  </group>
//</regmap>

//<regmap name="NIXGE_REGMAP" markdown="true" generateverilog="false">
//  <group name="XGE_MAC_WINDOW">
//    <window name="XGE_MAC" offset="0x1000" size="0x1000" targetregmap="XGE_MAC_REGMAP"/>
//  </group>
//  <group name="XGE_MAC_REGS">
//      <info>
//         nixge (maps to 10g mac if present)
//      </info>
//    <register name="PORT_INFO" offset="0x0000">
//      <bitfield name="COMPAT_NUM" range="31..24">
//        <info>
//          Constant indicating version for this space.
//          Not used by the NIXGE driver (12/4/2020)
//        </info>
//      </bitfield>
//      <bitfield name="ACTIVITY" range="17">
//        <info>
//          Generically this mirrors the activity LED.  Specific meaning varies based on the MGT_PROTOCOL.
//        </info>
//      </bitfield>
//      <bitfield name="LINK_UP" range="16">
//        <info>
//          Generically means that a connection with a peer has been established.  Specific
//          meaning varies based on the MGT_PROTOCOL.
//        </info>
//      </bitfield>
//      <bitfield name="MGT_PROTOCOL" range="15..8">
//        <info>
//          Constant indicating what flavor of communication this port is using
//
//          - 0 = NONE
//          - 1 = 1GbE
//          - 2 = 10GbE
//          - 3 = Aurora
//          - 4 = WhiteRabbit
//          - 5 = 100GbE
//
//        </info>
//      </bitfield>
//      <bitfield name="PORTNUM" range="7..0">
//        <info>
//          Constant indicating which port this register is hooked to
//
//          - 0 = QSFP0
//          - 1 = QSFP1
//
//        </info>
//      </bitfield>
//    </register>
//    <register name="MAC_CTRL_STATUS" offset="0x0004">
//        <info>
//          Definition of this register depends on Protocol
//
//          **10GBE**
//
//          *READ - Status*
//
//          - 0 = status_crc_error
//          - 1 = status_fragment_error
//          - 2 = status_txdfifo_ovflow
//          - 3 = status_txdfifo_udflow
//          - 4 = status_rxdfifo_ovflow
//          - 5 = status_rxdfifo_udflow
//          - 6 = status_pause_frame_rx
//          - 7 = status_local_fault
//          - 8 = status_remote_fault
//
//          *WRITE - Ctl*
//
//          - 0 = ctrl_tx_enable
//
//          **100 GBE**
//
//          *READ - Status*
//
//          - 0 = tx_ovfout - Sets if TX overflow reported by CMAC
//                (Stays set till MAC is reset).  This is a fatal error
//          - 1 = tx_unfout - Sets if TX underflow reported by CMAC
//                (Stays set till MAC is reset).  This is a fatal error
//          - 2 = stat_rx_aligned - goes high when CMAC has finished
//                alignment, and is ready to start reception of traffic.
//          - 3 = mac_dropped_packet - If the mac RX wants to push data(TVALID)
//                but upstream is trying to hold(TREADY)off we drop a packet.
//                Upstream circuitry should detect this when traffic is forked
//                between CHDR and CPU, so this bit will only set if there is a
//                HW design error.
//          - 4 = auto_config_done - This bit goes high when the auto_config
//                state machine finishes operation.  It is very similiar to
//                stat_rx_alligned, but waits for extra writes which occur
//                after allignement to complete.
//          - 24:16 = pause_mask - readable version of pause_mask bellow.
//
//          *WRITE - Ctl*
//
//          - 0 = auto_enable - Defaults to ON after reset - Enables a
//                state machine that performs CMAC register writes to
//                bring up the MAC without SW intervention.
//          - 24:16 = pause_mask - A second layer of enables(the first being
//               register in the CMAC) on the pause_request mechanic. Bits
//               7:0 of enable pause on PFC7:0. Bit 8 enables global pause
//               request (not priority controlled). The mask is used for TX
//               and RX.
//        </info>
//    </register>
//    <register name="MAC_PHY_STATUS" offset="0x0008">
//        <info>
//
//          Definition of this register depends on Protocol
//
//          **10GBE**
//
//          *READ - Status *
//
//          - 0 = core_status 0 - link_up
//          - 1 = core_status 1
//          - 2 = core_status 2
//          - 3 = core_status 3
//          - 4 = core_status 4
//          - 5 = core_status 5
//          - 6 = core_status 6
//          - 7 = core_status 7
//
//          **100 GBE**
//
//          *READ - Status*
//
//          - 0 = usr_tx_reset - TX PLL's have locked - The clock for the 100G mac isn't stable till this bit sets.
//          - 1 = usr_rx_reset - RX PLL's have locked
//
//        </info>
//    </register>
//    <register name="MAC_LED_CTL" offset="0x000C">
//      <bitfield name="identify_enable" range="0">
//        <info>
//           When set identify_value is used to control the activity LED.
//           When clear the activity LED set on any TX or RX traffic to the mgt
//        </info>
//      </bitfield>
//      <bitfield name="identify_value" range="1">
//        <info>
//           When identify_enable is set, this value controls the activity LED.
//        </info>
//      </bitfield>
//    </register>
//    <register name="ETH_MDIO_BASE" offset="0x0010">
//        <info>
//           The x4xx family of products does not use MDIO.
//        </info>
//    </register>
//    <register name="AURORA_OVERRUNS" offset="0x0020">
//        <info>
//           Only valid if the protocol is Aurora.
//        </info>
//    </register>
//    <register name="AURORA_CHECKSUM_ERRORS" offset="0x0024">
//        <info>
//           Only valid if the protocol is Aurora.
//        </info>
//    </register>
//    <register name="AURORA_BIST_CHECKER_SAMPS" offset="0x0028">
//        <info>
//           Only valid if the protocol is Aurora.
//        </info>
//    </register>
//    <register name="AURORA_BIST_CHECKER_ERRORS" offset="0x002C">
//        <info>
//           Only valid if the protocol is Aurora.
//        </info>
//    </register>
//  </group>
//</regmap>
//
//<regmap name="UIO_REGMAP" markdown="true" generateverilog="false">
//  <group name="UIO_REGS">
//    <info>
//       UIO
//    </info>
//    <register name="IP" offset="0x0000">
//        <info>
//           Set this port's IP address
//        </info>
//    </register>
//    <register name="UDP" offset="0x0004">
//        <info>
//           Set the UDP port for CHDR_traffic
//        </info>
//    </register>
//    <register name="BRIDGE_MAC_LSB" offset="0x0010">
//        <info>
//           If BRIDGE_ENABLE is set use this MAC_ID
//        </info>
//    </register>
//    <register name="BRIDGE_MAC_MSB" offset="0x0014">
//        <info>
//           If BRIDGE_ENABLE is set use this MAC_ID
//        </info>
//    </register>
//    <register name="BRIDGE_IP" offset="0x0018">
//        <info>
//           If BRIDGE_ENABLE is set use this IP Address
//        </info>
//    </register>
//    <register name="BRIDGE_UDP" offset="0x001C">
//        <info>
//           If BRIDGE_ENABLE is set use this UDP Port for CHDR_traffic
//        </info>
//    </register>
//    <register name="BRIDGE_ENABLE" offset="0x0020">
//        <info>
//           Bit 0 Controls the following logic
//
//```verilog
//  always_comb begin : bridge_mux
//    my_mac            = bridge_en ? bridge_mac_reg : mac_reg;
//    my_ip             = bridge_en ? bridge_ip_reg : ip_reg;
//    my_udp_chdr_port  = bridge_en ? bridge_udp_port : udp_port;
//  end
//```
//
//        </info>
//    </register>
//    <register name="CHDR_DROPPED" offset="0x0030">
//        <info>
//           Count the number of Packets dropped that were addressed to the CHDR section.
//        </info>
//    </register>
//    <register name="CPU_DROPPED" offset="0x0034">
//        <info>
//           Count the number of Packets dropped that were addressed to us, but not to the CHDR section.
//        </info>
//    </register>
//    <register name="PAUSE" offset="0x0038">
//      <bitfield name="pause_set" range="15..0">
//        <info>
//           If the fullness of the CHDR_FIFO in ETH_W words exceeds this value request an ethernet pause.  This feature is only
//           used with 100Gb ethernet
//        </info>
//      </bitfield>
//      <bitfield name="pause_clear" range="31..16">
//        <info>
//           If the fullness of the CHDR_FIFO in ETH_W words falls bellow this value stop requesting an ethernet pause.
//           *Pause clear must be less than pause set or terrible things will happen.*
//           The clearing of the pause request causes the MAC to send a request to resume traffic. This feature is only
//           used with 100Gb ethernet
//        </info>
//      </bitfield>
//    </register>
//  </group>
//</regmap>

//<regmap name="QSFP_REGMAP" markdown="true" generateverilog="false">
//  <group name="QSFP_WINDOWS">
//    <info>
//       Register space for a single QSFP Communication port.  This currently breaks into 2 possible configurations
//
//       - 1X10GB Ethernet - Using OpenCore XGE MAC
//       - 1x100GB Ethernet - Using Xilinx CMAC
//       - (future possible) - Xilinx Aurora (various rates and lane widths)
//       - (future possible) - 4X10GB Ethernet
//
//    </info>
//    <window name="ETH_DMA" offset="0x000" size="0x4000" targetregmap="DMA_REGMAP"/>
//    <window name="NIXGE"   offset="0x8000" size="0x2000" targetregmap="NIXGE_REGMAP"/>
//    <window name="UIO"     offset="0xA000" size="0x2000" targetregmap="UIO_REGMAP"/>
//    <window name="CMAC"    offset="0xC000" size="0x2000" targetregmap="CMAC_REGMAP"/>
//  </group>
//</regmap>


//<regmap name="AXI_HPM0_REGMAP" markdown="true" generateverilog="false">
//  <group name="UHD_ONLY">
//    <info>
//      - 0_0 indicates QSFP0 - Lane0 or a 4 LANE QSFP0
//      - 0_1 indicates QSFP0 - Lane1
//      - 0_2 indicates QSFP0 - Lane2
//      - 0_3 indicates QSFP0 - Lane3
//      - 1_0 indicates QSFP1 - Lane0 or a 4 LANE QSFP1
//      - 1_1 indicates QSFP1 - Lane1
//      - 1_2 indicates QSFP1 - Lane2
//      - 1_3 indicates QSFP1 - Lane3
//    </info>
//    <window name="QSFP_0_0"  offset="0x1200000000" size="0x10000" targetregmap="QSFP_REGMAP"/>
//    <window name="QSFP_0_1"  offset="0x1200010000" size="0x10000" targetregmap="QSFP_REGMAP"/>
//    <window name="QSFP_0_2"  offset="0x1200020000" size="0x10000" targetregmap="QSFP_REGMAP"/>
//    <window name="QSFP_0_3"  offset="0x1200030000" size="0x10000" targetregmap="QSFP_REGMAP"/>
//    <window name="QSFP_1_0"  offset="0x1200040000" size="0x10000" targetregmap="QSFP_REGMAP"/>
//    <window name="QSFP_1_1"  offset="0x1200050000" size="0x10000" targetregmap="QSFP_REGMAP"/>
//    <window name="QSFP_1_2"  offset="0x1200060000" size="0x10000" targetregmap="QSFP_REGMAP"/>
//    <window name="QSFP_1_3"  offset="0x1200070000" size="0x10000" targetregmap="QSFP_REGMAP"/>
//  </group>
//</regmap>
//XmlParse xml_off
