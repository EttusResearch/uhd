
CHANGE LOG for TEN GIGABIT ETHERNET PCS/PMA

Release Date:  December 18, 2012 
--------------------------------------------------------------------------------

Table of Contents

1. INTRODUCTION 
2. DEVICE SUPPORT    
3. NEW FEATURE HISTORY   
4. RESOLVED ISSUES 
5. KNOWN ISSUES & LIMITATIONS 
6. TECHNICAL SUPPORT & FEEDBACK
7. CORE RELEASE HISTORY 
8. LEGAL DISCLAIMER 


--------------------------------------------------------------------------------


1. INTRODUCTION

  This file contains release notes for the Xilinx LogiCORE IP Ten Gigabit 
  Ethernet PCS/PMA. 

  For the latest core updates, see the product page at:

    http://www.xilinx.com/products/ipcenter/10GBASE-R.htm

  For installation instructions for this release, please go to:

    http://www.xilinx.com/ipcenter/coregen/ip_update_install_instructions.htm

  For system requirements:

    http://www.xilinx.com/ipcenter/coregen/ip_update_system_requirements.htm


2. DEVICE SUPPORT

  2.1 ISE 
 
    The following device families are supported by the core for this release.

      Virtex-7 devices
        Virtex-7 T/XT -2L             (v2.4 or later)
        Virtex-7 T/XT -2, -3          (v2.1 or later)

      Kintex-7 devices
        Kintex-7 -2L                  (v2.4 or later)
        Kintex-7 -2, -3               (v2.1 or later)

      Virtex-6 devices                                     
        Virtex-6 HXT                  (v2.1 or later)

  2.2 Vivado 

    The following device families are supported by the core for this release.

      Virtex-7 devices
        Virtex-7 T/HT/XT -2L             (v2.4 or later)
        Virtex-7 T/XT -2, -3             (v2.1 or later)

      Kintex-7 devices
        Kintex-7 -2L                     (v2.4 or later)
        Kintex-7 -2, -3                  (v2.1 or later)


3. NEW FEATURE HISTORY

  3.1 ISE 

    v2.4
      - Added -2L speedgrade support
    
    v2.5
      - Revision for 14.3

    v2.6
      - Revision for 14.4

  3.2 Vivado

    v2.4
      - Added -2L speedgrade support

    v2.5
      - Revision for 2012.3

    v2.6
      - Revision for 2012.4


4. RESOLVED ISSUES

  4.1 ISE 

    The following issues are resolved in the indicated IP versions:

    v2.4
      - CR667829 - fixed simulation problem in FEC block lock FSM
      - CR666972 - marked v2.3 of the core as superseded
      - CR666235 - Implemented latest guidance on RXUSRRDY timer for GT reset
                   sequencing
      - CR666082 - fixed some ucf constraints which were not matching the 
                   MMCM-derived clocks due to use of TIGs
      - CR665417 - added constraint to stop synthesis from inferring 3 BRAMs 
                   as ROMs
      - CR655365 - Added core name and version to all RTL file names, modules
                   and entities
      - CR665300 - Added an_enable to core_status vector bit 4 and an_link_up
                   to core_status vector bit 5
      - CR664844 - Added Synplify-specific KEEP constraints
      - CR664839 - Removed LVDS constraints from ucf/xdc files
      - CR664835 - Added -2L part support for Kintex7 and Virtex7
      - CR664077 - Added block-level logic to remove reset deadlock when cable 
                   is pulled
      - CR663760 - Added free-running counter for AN Nonce generation 
                   initialization
      - CR662882 - Fixed startup value of block_lock indication to 0
      - CR662553 - Incomplete sensitivity list - this was the constant 0 which
                   is used in the block level to stop synthesis optimizing
                   away the management logic.
      - CR661877 - Update the example design with the latest GTH attributes
      - CR661247 - PCS Reset was resetting the FEC error counters - should be
                   PMA Reset
      - CR661196 - Training interface requires a reg on the ipif_cs line to 
                   align rdack with rddata
      - CR660819 - Need to switch GT to LPM mde during AN phase
      - CR659706 - Empty toggling on RX Elastic Buffer
      - CR656721 - Cannot consistently drive a MMCM from a IBUFDS_GTE2 in K7
      
    v2.4 Rev 1
      - CR670313 - Remove spurious 'else' in VHDL block levels
      - CR670190 - Connect signal_detect port to core FSMs
      - CR670342 - Separate TX and RX resets in block level
      - CR668070 - Improve hot-plug capabilities

    v2.4 Rev 2
      - CR673048 - Made sure Training FSM stops in Fail state
      - CR668070 - Further improve hot-plug capabilities
      
    v2.4 Rev 3
      - CR674913 - Fixed txratefifo block so that corrupted data is not passed 
                   to transceiver

    v2.5 
      - CR670303 - Remove deprecated ROCBUF from VHDL demo tbs
      - CR670811 - ditto
      - CR670914 - Make rxuuserrdy dependant on qplllock
      - CR671668 - Various improvements to reset logic for example design
      - CR673048 - Do not reset Training block with general reset
      - CR675239 - Connect RXSLIDE in GT Instance to allow Training to work
      - CR676437 - Fix local Training backdoor programming when no MDIO
      - CR676950 - Fix DRP register locations for Training
      - CR679275 - Fix Autoneg block to cope with Next Pages
      - CR679904 - Fix Bug in Block Lock FSM which counts 65 blocks and 17 errors
                     instead of 64 and 16 respectively
      - CR680101 - Adjust Training block to meet 802.3 electrical spec
      
    v2.6
      - CR689072 - Fixed a UCF constraint so that both XST and Synplify can use it
      - CR685716 - Fixed a UCF constraint so that Synplify can use it
      - CR685384 - Removed some unnecessary synchronizers and max_delay constraints
      - CR685209 - Make TX Nonce generation more uniformly random
      - CR684447 - Add synchronizers to async resets
      - CR684332 - Remove PCS loopback feature for BaseKR cores and PCS loopback 
                   fifo for all cores - not required
      - CR683731 - Constraints updated to cover replicated registers
      - CR683507 - Fixed the incorrect setting for the minimum (negative) value for
                   the -ve TX coefficient
      - CR682881 - Training block is now reset by PMA reset - was PCS reset
      - CR681669 - Added registers on MDIO signals in BaseR core to match BaseKR core
      - CR681628 - Removed unused (reset) logic from Block Level code
      - CR681627 - Fixed issue of possible deadlock in reset logic
      - CR681281 - Added TIG constraint to UCF to ignore otherwise unconstrained path
                  
  4.2 Vivado 

    The following issues are resolved in the indicated IP versions:

    v2.4
      - As above
      - CR666971 - added support for native Vivado simulation

    v2.4 Rev 1
      - CR655363 - Improve timing closure with different XDC constraints
      - CR665888 - Improve timing closure with synthesis pragmas 
      - CR670313 - Remove spurious 'else' in VHDL block levels
      - CR670190 - Connect signal_detect port to core FSMs
      - CR670342 - Separate TX and RX resets in block level
      - CR668070 - Improve hot-plug capabilities

    v2.4 Rev 2
      - CR673048 - Made sure Training FSM stops in Fail state
      - CR668070 - Further improve hot-plug capabilities

    v2.4 Rev 3
      - CR674913 - Fixed txratefifo block so that corrupted data is not passed 
                   to transceiver

    v2.5 
      - CR670303 - Remove deprecated ROCBUF from VHDL demo tbs
      - CR670811 - ditto
      - CR670914 - Make rxuuserrdy dependant on qplllock
      - CR671668 - Various improvements to reset logic for example design
      - CR673048 - Do not reset Training block with general reset
      - CR675239 - Connect RXSLIDE in GT Instance to allow Training to work
      - CR676437 - Fix local Training backdoor programming when no MDIO
      - CR676950 - Fix DRP register locations for Training
      - CR677172 - Remove support for 290T and 1500T devices
      - CR679275 - Fix Autoneg block to cope with Next Pages
      - CR679904 - Fix Bug in Block Lock FSM which counts 65 blocks and 17 errors
                     instead of 64 and 16 respectively
      - CR680101 - Adjust Training block to meet 802.3 electrical spec
      
    v2.6
      - CR689842 - Major improvement to XDC constraints
      - CR688446 - Add reset to register in AN block to aid constraints matching
      - CR688445 - Add duplicate register in training block to aid constraints 
                   matching
      - CR685384 - Removed some unnecessary synchronizers and max_delay constraints
      - CR685209 - Make TX Nonce generation more uniformly random
      - CR684447 - Add synchronizers to async resets
      - CR684332 - Remove PCS loopback feature for BaseKR cores and PCS loopback 
                   fifo for all cores - not required
      - CR683731 - Constraints updated to cover replicated registers
      - CR683507 - Fixed the incorrect setting for the minimum (negative) value for
                   the -ve TX coefficient
      - CR682881 - Training block is now reset by PMA reset - was PCS reset
      - CR681669 - Added registers on MDIO signals in BaseR core to match BaseKR core
      - CR681637 - Revisions to XDC constraints
      - CR681628 - Removed unused (reset) logic from Block Level code
      - CR681627 - Fixed issue of possible deadlock in reset logic

5. KNOWN ISSUES & LIMITATIONS

  5.1 ISE 
  
    - Problem with Router for Kintex - AR52137
    
  5.2 Vivado 
  
    - None
    
  - For a comprehensive listing of Known Issues for this core, please see the IP 
    Release Notes Guide,  

   www.xilinx.com/support/documentation/user_guides/xtp025.pdf


6. TECHNICAL SUPPORT & FEEDBACK

   To obtain technical support, create a WebCase at www.xilinx.com/support.  
   Questions are routed to a team with expertise using this product.  
   Feedback on this IP core may also be submitted under the "Leave Feedback" 
   menu item in Vivado/PlanAhead.

   Xilinx provides technical support for use of this product when used
   according to the guidelines described in the core documentation, and
   cannot guarantee timing, functionality, or support of this product for
   designs that do not follow specified guidelines.


7. CORE RELEASE HISTORY

Date        By            Version      Description
================================================================================
12/18/2012  Xilinx, Inc.  2.6          Various improvements for new release 
10/16/2012  Xilinx, Inc.  2.5          Various improvements for new release, 
                                         including to Training and AutoNegotiation.
                                       Enhancements to documentation.
08/27/2012  Xilinx, Inc.  2.4 Rev 3    Fixed bug in txsequence passed to transceiver
08/10/2012  Xilinx, Inc.  2.4 Rev 2    Further enhanced hot-plug capabilities
                                         Fixed Training FSM Fail behaviour
08/01/2012  Xilinx, Inc.  2.4 Rev 1    Enhanced hot-plug capabilities
                                         Fixed some tool-related issues
07/25/2012  Xilinx, Inc.  2.4          ISE 14.2 and Vivado 2012.2 support
04/24/2012  Xilinx, Inc.  2.3          ISE 14.1 and Vivado 2012.1 support, 
                                         Add Series7 GTHE2 support
10/19/2011  Xilinx. Inc.  2.2          ISE 13.3 support
03/01/2011  Xilinx, Inc.  2.1          ISE 13.1 support, add KR and Series7
04/19/2010  Xilinx, Inc.  1.2          ISE 12.1 support
12/02/2009  Xilinx, Inc.  1.1          ISE 11.4 support
================================================================================


8. LEGAL DISCLAIMER

  (c) Copyright 2009 - 2012 Xilinx, Inc. All rights reserved.

  This file contains confidential and proprietary information
  of Xilinx, Inc. and is protected under U.S. and
  international copyright and other intellectual property
  laws.

  DISCLAIMER
  This disclaimer is not a license and does not grant any
  rights to the materials distributed herewith. Except as
  otherwise provided in a valid license issued to you by
  Xilinx, and to the maximum extent permitted by applicable
  law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
  WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
  AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
  BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
  INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
  (2) Xilinx shall not be liable (whether in contract or tort,
  including negligence, or under any other theory of
  liability) for any loss or damage of any kind or nature
  related to, arising under or in connection with these
  materials, including for any direct, or any indirect,
  special, incidental, or consequential loss or damage
  (including loss of data, profits, goodwill, or any type of
  loss or damage suffered as a result of any action brought
  by a third party) even if such damage or loss was
  reasonably foreseeable or Xilinx had been advised of the
  possibility of the same. 

  CRITICAL APPLICATIONS
  Xilinx products are not designed or intended to be fail-
  safe, or for use in any application requiring fail-safe
  performance, such as life-support or safety devices or
  systems, Class III medical devices, nuclear facilities,
  applications related to the deployment of airbags, or any
  other applications that could lead to death, personal
  injury, or severe property or environmental damage
  (individually and collectively, "Critical 
  Applications"). Customer assumes the sole risk and 
  liability of any use of Xilinx products in Critical 
  Applications, subject only to applicable laws and 
  regulations governing limitations on product liability. 
 
  THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
  PART OF THIS FILE AT ALL TIMES.


