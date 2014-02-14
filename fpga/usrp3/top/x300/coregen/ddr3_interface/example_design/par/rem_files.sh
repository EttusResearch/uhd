##!/bin/csh -f
##****************************************************************************
## (c) Copyright 2009 - 2010 Xilinx, Inc. All rights reserved.
##
## This file contains confidential and proprietary information
## of Xilinx, Inc. and is protected under U.S. and
## international copyright and other intellectual property
## laws.
##
## DISCLAIMER
## This disclaimer is not a license and does not grant any
## rights to the materials distributed herewith. Except as
## otherwise provided in a valid license issued to you by
## Xilinx, and to the maximum extent permitted by applicable
## law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
## WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
## AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
## BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
## INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
## (2) Xilinx shall not be liable (whether in contract or tort,
## including negligence, or under any other theory of
## liability) for any loss or damage of any kind or nature
## related to, arising under or in connection with these
## materials, including for any direct, or any indirect,
## special, incidental, or consequential loss or damage
## (including loss of data, profits, goodwill, or any type of
## loss or damage suffered as a result of any action brought
## by a third party) even if such damage or loss was
## reasonably foreseeable or Xilinx had been advised of the
## possibility of the same.
##
## CRITICAL APPLICATIONS
## Xilinx products are not designed or intended to be fail-
## safe, or for use in any application requiring fail-safe
## performance, such as life-support or safety devices or
## systems, Class III medical devices, nuclear facilities,
## applications related to the deployment of airbags, or any
## other applications that could lead to death, personal
## injury, or severe property or environmental damage
## (individually and collectively, "Critical
## Applications"). Customer assumes the sole risk and
## liability of any use of Xilinx products in Critical
## Applications, subject only to applicable laws and
## regulations governing limitations on product liability.
##
## THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
## PART OF THIS FILE AT ALL TIMES.
##
##****************************************************************************
##   ____  ____
##  /   /\/   /
## /___/  \  /    Vendor                : Xilinx
## \   \   \/     Version               : 1.8
##  \   \         Application           : MIG
##  /   /         Filename              : rem_files.bat
## /___/   /\     Date Last Modified    : $Date: 2011/06/02 08:31:13 $
## \   \  /  \    Date Created          : Tue Sept 21 2010
##  \___\/\___\
##
## Device            : 7 Series
## Design Name       : DDR3 SDRAM
## Purpose           : Batch file to remove files generated from ISE
## Reference         :
## Revision History  :
##****************************************************************************

rm -rf "../synth/__projnav"
rm -rf "../synth/xst"
rm -rf "../synth/_ngo"

rm -rf tmp
rm -rf _xmsgs

rm -rf xst
rm -rf xlnx_auto_0_xdb

rm -rf coregen.cgp
rm -rf coregen.cgc
rm -rf coregen.log

rm -rf ise_flow_results.txt
rm -rf xlnx_auto_0.ise
rm -rf example_top_vhdl.prj
rm -rf example_top.syr
rm -rf example_top.ngc
rm -rf example_top.ngr
rm -rf example_top_xst.xrpt
rm -rf example_top.bld
rm -rf example_top.ngd
rm -rf example_top_ngdbuild.xrpt
rm -rf example_top_map.map
rm -rf example_top_map.mrp
rm -rf example_top_map.ngm
rm -rf example_top.pcf
rm -rf example_top_map.ncd
rm -rf example_top_map.xrpt
rm -rf example_top_summary.xml
rm -rf example_top_usage.xml
rm -rf example_top.ncd
rm -rf example_top.par
rm -rf example_top.xpi
rm -rf smartpreview.twr
rm -rf example_top.ptwx
rm -rf example_top.pad
rm -rf example_top.unroutes
rm -rf example_top_pad.csv
rm -rf example_top_pad.txt
rm -rf example_top_par.xrpt
rm -rf example_top.twx
rm -rf example_top.bgn
rm -rf example_top.twr
rm -rf example_top.drc
rm -rf example_top_bitgen.xwbt
rm -rf example_top.bit

# Files and folders generated Coregen ChipScope Modules
rm -rf ddr_icon.asy
rm -rf ddr_icon.ngc
rm -rf ddr_icon.xco
rm -rf ddr_icon_xmdf.tcl
rm -rf ddr_icon.gise
rm -rf ddr_icon.ise
rm -rf ddr_icon.xise
rm -rf ddr_icon_flist.txt
rm -rf ddr_icon_readme.txt
rm -rf ddr_icon.cdc
rm -rf ddr_icon_xdb

rm -rf ddr_ila_basic.asy
rm -rf ddr_ila_basic.ngc
rm -rf ddr_ila_basic.constraints
rm -rf ddr_ila_basic.ncf
rm -rf ddr_ila_basic.ucf
rm -rf ddr_ila_basic.xdc
rm -rf ddr_ila_basic.xco
rm -rf ddr_ila_basic_xmdf.tcl
rm -rf ddr_ila_basic.gise
rm -rf ddr_ila_basic.ise
rm -rf ddr_ila_basic.xise
rm -rf ddr_ila_basic_flist.txt
rm -rf ddr_ila_basic_readme.txt
rm -rf ddr_ila_basic.cdc
rm -rf ddr_ila_basic_xdb

rm -rf ddr_ila_wrpath.asy
rm -rf ddr_ila_wrpath.ngc
rm -rf ddr_ila_wrpath.constraints
rm -rf ddr_ila_wrpath.ncf
rm -rf ddr_ila_wrpath.ucf
rm -rf ddr_ila_wrpath.xdc
rm -rf ddr_ila_wrpath.xco
rm -rf ddr_ila_wrpath_xmdf.tcl
rm -rf ddr_ila_wrpath.gise
rm -rf ddr_ila_wrpath.ise
rm -rf ddr_ila_wrpath.xise
rm -rf ddr_ila_wrpath_flist.txt
rm -rf ddr_ila_wrpath_readme.txt
rm -rf ddr_ila_wrpath.cdc
rm -rf ddr_ila_wrpath_xdb

rm -rf ddr_ila_rdpath.asy
rm -rf ddr_ila_rdpath.ngc
rm -rf ddr_ila_rdpath.constraints
rm -rf ddr_ila_rdpath.ncf
rm -rf ddr_ila_rdpath.ucf
rm -rf ddr_ila_rdpath.xdc
rm -rf ddr_ila_rdpath.xco
rm -rf ddr_ila_rdpath_xmdf.tcl
rm -rf ddr_ila_rdpath.gise
rm -rf ddr_ila_rdpath.ise
rm -rf ddr_ila_rdpath.xise
rm -rf ddr_ila_rdpath_flist.txt
rm -rf ddr_ila_rdpath_readme.txt
rm -rf ddr_ila_rdpath.cdc
rm -rf ddr_ila_rdpath_xdb

rm -rf ddr_vio_sync_async_out72.asy
rm -rf ddr_vio_sync_async_out72.ngc
rm -rf ddr_vio_sync_async_out72.constraints
rm -rf ddr_vio_sync_async_out72.ncf
rm -rf ddr_vio_sync_async_out72.ucf
rm -rf ddr_vio_sync_async_out72.xdc
rm -rf ddr_vio_sync_async_out72.xco
rm -rf ddr_vio_sync_async_out72_xmdf.tcl
rm -rf ddr_vio_sync_async_out72.gise
rm -rf ddr_vio_sync_async_out72.ise
rm -rf ddr_vio_sync_async_out72.xise
rm -rf ddr_vio_sync_async_out72_flist.txt
rm -rf ddr_vio_sync_async_out72_readme.txt
rm -rf ddr_vio_sync_async_out72.cdc
rm -rf ddr_vio_sync_async_out72_xdb

rm -rf ddr_vio_async_in_sync_out.asy
rm -rf ddr_vio_async_in_sync_out.ngc
rm -rf ddr_vio_async_in_sync_out.constraints
rm -rf ddr_vio_async_in_sync_out.ncf
rm -rf ddr_vio_async_in_sync_out.ucf
rm -rf ddr_vio_async_in_sync_out.xdc
rm -rf ddr_vio_async_in_sync_out.xco
rm -rf ddr_vio_async_in_sync_out_xmdf.tcl
rm -rf ddr_vio_async_in_sync_out.gise
rm -rf ddr_vio_async_in_sync_out.ise
rm -rf ddr_vio_async_in_sync_out.xise
rm -rf ddr_vio_async_in_sync_out_flist.txt
rm -rf ddr_vio_async_in_sync_out_readme.txt
rm -rf ddr_vio_async_in_sync_out.cdc
rm -rf ddr_vio_async_in_sync_out_xdb

# Files and folders generated by create ise
rm -rf test_xdb
rm -rf _xmsgs
rm -rf test.gise
rm -rf test.xise
rm -rf test.xise

# Files and folders generated by ISE through GUI mode
rm -rf _ngo
rm -rf xst
rm -rf example_top.cmd_log
rm -rf example_top.lso
rm -rf example_top.prj
rm -rf example_top.stx
rm -rf example_top.ut
rm -rf example_top.xst
rm -rf example_top_guide.ncd
rm -rf example_top_prev_built.ngd
rm -rf example_top_summary.html
rm -rf par_usage_statistics.html
rm -rf usage_statistics_webtalk.html
rm -rf webtalk.log
rm -rf device_usage_statistics.html
rm -rf test.ntrc_log
