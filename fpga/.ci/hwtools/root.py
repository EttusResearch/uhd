#!/usr/bin/env python
#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Description:
#
#   Setup script for configuring the pipeline agent's environment to allow us
#   to run proprietary EDA vendor tools.
#

import sys

def do_setup():
    from hwtools.api import rootapi

    rootapi.set_workspace_name(name="uhddev", pretty_name="UHD FPGA")
    rootapi.set_major_version("1")

    rootapi.add_tool(name="hwtools",  version="head")
    rootapi.add_tool(name="Vivado",   version="2021.1", allowlater=False)
    rootapi.add_tool(name="modelsim", version="2020.4", allowlater=True)

def do_finish():
    pass

def do_publish():
    pass

if __name__ == "__main__":
    sys.exit(-1)
