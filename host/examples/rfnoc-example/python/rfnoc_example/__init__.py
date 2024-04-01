#
# Copyright 2024 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
rfnoc-example: Example module for Python support
"""

# Import all bindings from C++
from . import rfnoc_example_python as lib

# In UHD, we use CamelCase for names in Python, so we'll do the same here
GainBlockControl = lib.gain_block_control
