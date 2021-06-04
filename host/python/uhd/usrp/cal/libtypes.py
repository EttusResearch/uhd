#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Import cal types into Python
"""

from ... import libpyuhd as lib

# Disable PyLint because we want to make it look like the following classes are
# defined in Python, but they're just renames of lib types. They therefore
# follow name conventions for Python classes, not for global constants.
# pylint: disable=invalid-name
# database is a class, but we treat it like a namespace, i.e., a submodule
database = lib.cal.database
Source = lib.cal.source
IQCal = lib.cal.iq_cal
PwrCal = lib.cal.pwr_cal
ZbxTxDsaCal = lib.cal.zbx_tx_dsa_cal
ZbxRxDsaCal = lib.cal.zbx_rx_dsa_cal
InterpMode = lib.cal.interp_mode
# pylint: enable=invalid-name
