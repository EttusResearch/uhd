#
# Copyright 2024 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# ModelSim DO file to run a simulation in batch mode. This script adds
# detection of error/failure assertions so that non-zero values are returned by
# ModelSim when the testbench doesn't pass. Calling std.env.finish() or
# $finish() will return 0. Detecting $fatal() requires that -onfinish be set to
# stop so the simulator doesn't quit before the DO file finishes.
#

quietly set BreakOnAssertion 2
quietly set StdArithNoWarnings 1
quietly set NumericStdNoWarnings 1
quietly set SIM_ERROR 0
onbreak {
    quietly set FINISH [lindex [runStatus -full] 2]
    if {$FINISH eq "unknown"} { set SIM_ERROR 255 }
}
onerror { set SIM_ERROR 255 }
run -all
quit -force -code $SIM_ERROR
