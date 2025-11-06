#
# Copyright 2025 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""Import low-level interface helpers into Python for Max287x chips."""

from ... import libpyuhd as lib

# Disable PyLint because we want to make it look like the following classes are
# defined in Python, but they're just renames of lib types. They therefore
# follow name conventions for Python classes, not for global constants.
# pylint: disable=invalid-name
Max2871Iface = lib.usrp.chips.max287x.max_2871
OutputPower = lib.usrp.chips.max287x.output_power
AuxOutputPower = lib.usrp.chips.max287x.aux_output_power
MuxoutMode = lib.usrp.chips.max287x.muxout_mode
ChargePumpCurrent = lib.usrp.chips.max287x.charge_pump_current
LdPinMode = lib.usrp.chips.max287x.ld_pin_mode
RFOutPort = lib.usrp.chips.max287x.rf_output_port
# pylint: enable=invalid-name
