##!/bin/csh -f
##****************************************************************************
## (c) Copyright 2009 - 2011 Xilinx, Inc. All rights reserved.
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
##  /   /         Filename              : rem_files.tcl
## /___/   /\     Date Last Modified    : $Date: 2011/06/02 08:31:13 $
## \   \  /  \    Date Created          : Fri Jan 27 2012
##  \___\/\___\
##
## Device            : 7 Series
## Design Name       : DDR3 SDRAM
## Purpose           : Tcl file to remove files generated from PlanAhead
## Reference         :
## Revision History  :
##****************************************************************************

file delete -force  coregen.cgp
file delete -force  coregen.cgc
file delete -force  coregen.log

# Files and folders generated Coregen ChipScope Modules
file delete -force ddr_icon.asy
file delete -force ddr_icon.ngc
file delete -force ddr_icon.xco
file delete -force ddr_icon_xmdf.tcl
file delete -force ddr_icon.gise
file delete -force ddr_icon.ise
file delete -force ddr_icon.xise
file delete -force ddr_icon_flist.txt
file delete -force ddr_icon_readme.txt
file delete -force ddr_icon.cdc
file delete -force ddr_icon_xdb

file delete -force ddr_ila_basic.asy
file delete -force ddr_ila_basic.ngc
file delete -force ddr_ila_basic.xco
file delete -force ddr_ila_basic_xmdf.tcl
file delete -force ddr_ila_basic.gise
file delete -force ddr_ila_basic.ise
file delete -force ddr_ila_basic.xise
file delete -force ddr_ila_basic_flist.txt
file delete -force ddr_ila_basic_readme.txt
file delete -force ddr_ila_basic.cdc
file delete -force ddr_ila_basic_xdb

file delete -force ddr_ila_wrpath.asy
file delete -force ddr_ila_wrpath.ngc
file delete -force ddr_ila_wrpath.xco
file delete -force ddr_ila_wrpath_xmdf.tcl
file delete -force ddr_ila_wrpath.gise
file delete -force ddr_ila_wrpath.ise
file delete -force ddr_ila_wrpath.xise
file delete -force ddr_ila_wrpath_flist.txt
file delete -force ddr_ila_wrpath_readme.txt
file delete -force ddr_ila_wrpath.cdc
file delete -force ddr_ila_wrpath_xdb

file delete -force ddr_ila_rdpath.asy
file delete -force ddr_ila_rdpath.ngc
file delete -force ddr_ila_rdpath.xco
file delete -force ddr_ila_rdpath_xmdf.tcl
file delete -force ddr_ila_rdpath.gise
file delete -force ddr_ila_rdpath.ise
file delete -force ddr_ila_rdpath.xise
file delete -force ddr_ila_rdpath_flist.txt
file delete -force ddr_ila_rdpath_readme.txt
file delete -force ddr_ila_rdpath.cdc
file delete -force ddr_ila_rdpath_xdb

file delete -force ddr_vio_sync_async_out72.asy
file delete -force ddr_vio_sync_async_out72.ngc
file delete -force ddr_vio_sync_async_out72.xco
file delete -force ddr_vio_sync_async_out72_xmdf.tcl
file delete -force ddr_vio_sync_async_out72.gise
file delete -force ddr_vio_sync_async_out72.ise
file delete -force ddr_vio_sync_async_out72.xise
file delete -force ddr_vio_sync_async_out72_flist.txt
file delete -force ddr_vio_sync_async_out72_readme.txt
file delete -force ddr_vio_sync_async_out72.cdc
file delete -force ddr_vio_sync_async_out72_xdb

file delete -force ddr_vio_async_in_sync_out.asy
file delete -force ddr_vio_async_in_sync_out.ngc
file delete -force ddr_vio_async_in_sync_out.xco
file delete -force ddr_vio_async_in_sync_out_xmdf.tcl
file delete -force ddr_vio_async_in_sync_out.gise
file delete -force ddr_vio_async_in_sync_out.ise
file delete -force ddr_vio_async_in_sync_out.xise
file delete -force ddr_vio_async_in_sync_out_flist.txt
file delete -force ddr_vio_async_in_sync_out_readme.txt
file delete -force ddr_vio_async_in_sync_out.cdc
file delete -force ddr_vio_async_in_sync_out_xdb

# Files and folders generated by planAhead & ISE
file delete -force example_top.bgn
file delete -force example_top.bit
file delete -force example_top.drc
file delete -force example_top.edf
file delete -force example_top.ncd
file delete -force example_top.pcf
file delete -force example_top.planahead.xdc
file delete -force example_top.routed.v
file delete -force example_top.sta
file delete -force example_top.v
file delete -force example_top_drc.txt
file delete -force proj1.ppr
file delete -force xilinx_device_details.xml
file delete -force .Xil
file delete -force proj1.data
file delete -force proj1.srcs
file delete -force ise_flow_results.txt
file delete -force xlnx_auto_0.ise
file delete -force example_top_vhdl.prj
file delete -force example_top.syr
file delete -force example_top.ngc
file delete -force example_top.ngr
file delete -force example_top_xst.xrpt
file delete -force example_top.bld
file delete -force example_top.ngd
file delete -force example_top_ngdbuild.xrpt
file delete -force example_top_map.map
file delete -force example_top_map.mrp
file delete -force example_top_map.ngm
file delete -force example_top.pcf
file delete -force example_top_map.ncd
file delete -force example_top_map.xrpt
file delete -force example_top_summary.xml
file delete -force example_top_usage.xml
file delete -force example_top.ncd
file delete -force example_top.par
file delete -force example_top.xpi
file delete -force smartpreview.twr
file delete -force example_top.ptwx
file delete -force example_top.pad
file delete -force example_top.unroutes
file delete -force example_top_pad.csv
file delete -force example_top_pad.txt
file delete -force example_top_par.xrpt
file delete -force example_top.twx
file delete -force example_top.twr
file delete -force example_top.drc
file delete -force example_top_bitgen.xwbt
file delete -force synthesis.log
file delete -force optimization.log
file delete -force place.log
file delete -force route.log
file delete -force bitgen.log
file delete -force post_synth.dcp
file delete -force post_optimize.dcp
file delete -force post_place.dcp
file delete -force post_route.dcp
