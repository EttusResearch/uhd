#!/usr/bin/env python3
#
# Copyright 2025 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: MIT
#
# Generates the register maps for the X400 based FPGA designs.
#

import os
from hwtools.api import WorkingFolder, cmd

# enumerate the subdirectories
sub_directories = [d for d in os.listdir() if os.path.isdir(d)]

# assume the variant is called as the directory
for variant in sub_directories:
    with WorkingFolder(variant, delete_and_recreate=False, create_ok=False):
        # check if vsmakesettings.py exists
        if not os.path.isfile("vsmakesettings.py"):
            continue
        # generate register map
        cmd.runlive("xmlparse", "--clean", raise_on_err=True)
        cmd.runlive("xmlparse", raise_on_err=True)
