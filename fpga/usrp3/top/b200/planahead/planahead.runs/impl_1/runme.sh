#!/bin/sh

# 
# PlanAhead(TM)
# runme.sh: a PlanAhead-generated Runs Script for UNIX
# Copyright 1986-1999, 2001-2012 Xilinx, Inc. All Rights Reserved.
# 

if [ -z "$PATH" ]; then
  PATH=/opt/Xilinx/14.4/ISE_DS/EDK/bin/lin64:/opt/Xilinx/14.4/ISE_DS/ISE/bin/lin64:/opt/Xilinx/14.4/ISE_DS/common/bin/lin64:/opt/Xilinx/14.4/ISE_DS/PlanAhead/bin
else
  PATH=/opt/Xilinx/14.4/ISE_DS/EDK/bin/lin64:/opt/Xilinx/14.4/ISE_DS/ISE/bin/lin64:/opt/Xilinx/14.4/ISE_DS/common/bin/lin64:/opt/Xilinx/14.4/ISE_DS/PlanAhead/bin:$PATH
fi
export PATH

if [ -z "$LD_LIBRARY_PATH" ]; then
  LD_LIBRARY_PATH=/opt/Xilinx/14.4/ISE_DS/EDK/lib/lin64:/opt/Xilinx/14.4/ISE_DS/ISE/lib/lin64:/opt/Xilinx/14.4/ISE_DS/common/lib/lin64
else
  LD_LIBRARY_PATH=/opt/Xilinx/14.4/ISE_DS/EDK/lib/lin64:/opt/Xilinx/14.4/ISE_DS/ISE/lib/lin64:/opt/Xilinx/14.4/ISE_DS/common/lib/lin64:$LD_LIBRARY_PATH
fi
export LD_LIBRARY_PATH

HD_PWD=`dirname "$0"`
cd "$HD_PWD"

HD_LOG=runme.log
/bin/touch $HD_LOG

ISEStep="./ISEWrap.sh"
EAStep()
{
     $ISEStep $HD_LOG "$@" >> $HD_LOG 2>&1
     if [ $? -ne 0 ]
     then
         exit
     fi
}

EAStep ngdbuild -intstyle ise -p xc6slx75fgg484-3 -dd _ngo -uc "b200.ucf" "b200.edf"
EAStep map -intstyle pa -w "b200.ngd"
EAStep par -intstyle pa "b200.ncd" -w "b200_routed.ncd"
EAStep trce -intstyle ise -o "b200.twr" -v 30 -l 30 "b200_routed.ncd" "b200.pcf"
EAStep xdl -secure -ncd2xdl -nopips "b200_routed.ncd" "b200_routed.xdl"
