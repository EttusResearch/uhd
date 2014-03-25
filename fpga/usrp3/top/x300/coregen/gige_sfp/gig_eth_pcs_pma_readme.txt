
CHANGE LOG for Xilinx LogiCORE Ethernet 1000BASE-X PCS/PMA or SGMII v11.4

Release Date:  July 25, 2012 
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

  This file contains the change log for all released versions of the Xilinx 
  LogiCORE IP core  Ethernet 1000BASE-X PCS/PMA or SGMII. 
  
  For the latest core updates, see the product page at:

    http://www.xilinx.com/products/ipcenter/DO-DI-GMIITO1GBSXPCS.htm

  For installation instructions for this release, please go to:

    www.xilinx.com/ipcenter/coregen/ip_update_install_instructions.htm

  For system requirements, see:

    www.xilinx.com/ipcenter/coregen/ip_update_system_requirements.htm


2. DEVICE SUPPORT 

  2.1. ISE

    The following device families are supported by the core for this release:

    Virtex-7 devices
      Virtex-7                            
      Virtex-7 HT/XT                

    Kintex-7 devices
      Kintex-7

    Artix-7 devices
      Artix-7

    Zynq-7000 devices
      Zynq-7000

    Virtex-6 devices                        
      Virtex-6                              CXT/LXT/SXT/HXT
      Virtex-6 Lower Power (-1L)            LXT/SXT
      Defense Grade Virtex-6Q     (XQ)      LXT/SXT

    Spartan-6 devices
      Spartan-6                             LX/LXT
      Defense Grade Spartan-6Q              LX/LXT

    All Virtex-5 devices

    Virtex-4 devices
      Virtex-4                              LX/SX/FX

    Spartan-3 device families
      Spartan-3
      Spartan-3A and Spartan-3AN

      Spartan-3A DSP

      Spartan-3E

    
  2.2. VIVADO

 
    The following device families are supported by the core for this release:

    Virtex-7 devices
      Virtex-7                            
      Virtex-7 HT/XT                

    Kintex-7 devices
      Kintex-7

    Artix-7 devices
      Artix-7

    Zynq-7000 devices
      Zynq-7000


3. NEW FEATURE HISTORY


  3.1 ISE

    v11.4  
  
    - ISE 14.2 software support
    - Support for Zynq Devices


    v11.3
  
    - ISE 14.1 software support
    - Support for Artix7 Devices
    - Support for Virtex-7 HT Devices

    v11.2 
  
    - ISE 13.4 software support
    - Added programability through configuration vector


    v11.1

    - ISE 13.1 software support
    - Updated status vector
    - SGMII PHY mode
    - Support for Kintex7 Devices
    - Support for Virtex7 Devices


  3.2 Vivado

    v11.4
    - Vivado 2012.2 software support
    - Initial public release
    - Block level user editable logic delivered as part of the core



4. RESOLVED ISSUES 

  4.1 ISE

    The following issues are resolved in the indicated IP versions:

    v11.4
      - None

    v11.3
      - AR: 45676
      - AR: 46123

    v11.2
      - AR: 42672
      - AR: 36961
      - AR: 42842
      - AR: 43421
      - AR: 43482

    v11.1
      - AR: 36957
      - AR: 36961
      - AR: 35681


  4.2 Vivado

    v11.4
      - None


5. KNOWN ISSUES & LIMITATIONS 


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
07/25/2012  Xilinx, Inc. 11.4          ISE 14.2 and Vivado 2012.2.
                                       Support for Zynq Devices. Sync LVDS Solution
04/24/2012  Xilinx, Inc. 11.3          ISE 14.1, Artix-7 and Vivado 2012.1 support
01/18/2012  Xilinx, Inc. 11.2          ISE 13.4 Support
09/06/2011  Xilinx, Inc. 11.1 Rev 1    Patch release for ISE 13.1
03/01/2011  Xilinx, Inc. 11.1          ISE 13.1  and Virtex-7 / Kintex-7 support
07/30/2010  Xilinx, Inc. 10.5 Rev 1    Patch release for ISE 12.2
07/23/2010  Xilinx, Inc. 10.5          ISE 12.2 support and Virtex-6 LVDS I/O
04/19/2010  Xilinx, Inc. 10.4          Release for ISE 12.1
03/09/2010  Xilinx, Inc. 10.3 Rev 1    Patch release for ISE 11.5
09/16/2009  Xilinx, Inc. 10.3          11.3, Virtex-6 HXT and Lower Power support
06/24/2009  Xilinx, Inc. 10.2          Release for ISE 11.2 and Spartan-6 support
04/27/2009  Xilinx, Inc. 10.1          Release for ISE 11.1
03/24/2008  Xilinx, Inc.  9.1          Release for ISE 10.1
08/15/2007  Xilinx, Inc.  9.0          Release for 9.2i
04/02/2007  Xilinx, Inc.  8.1 Rev 1    Spartan-3A DSP support
03/05/2007  Xilinx, Inc.  8.1          Release for ISE 9.1i
10/26/2006  Xilinx, Inc.  8.0          Release for Virtex-5 and Spartan-3A
07/19/2006  Xilinx, Inc.  7.1          Release for ISE 8.2i
05/22/2006  Xilinx, Inc.  7.0 Rev 1    Virtex-4 FX CES4 update
01/18/2006  Xilinx, Inc.  7.0          Release for ISE 8.1i
06/24/2005  Xilinx, Inc.  6.0 patch1   Patch release
05/12/2005  Xilinx, Inc.  6.0          Release for ISE 7.1i
09/30/2004  Xilinx, Inc.  5.0          Release for ISE 6.3i
================================================================================


8. LEGAL DISCLAIMER

  (c) Copyright 2004 - 2012 Xilinx, Inc. All rights reserved.

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



