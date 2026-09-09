#!/usr/bin/env python3
#
# Copyright 2025 Ettus Research, a National Instruments Brand
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

    # Note: The hwtools version for CI purposes is fixed in run_setup.sh
    rootapi.add_tool(name="hwtools", version="head")
    rootapi.add_tool(name="Vivado", version="2025.2")
    rootapi.add_tool(name="Questasim", version="2025.1")
    rootapi.add_tool(name="Diamond", version="3.12")
    rootapi.add_tool(name="Quartus", version="20.1")


def do_finish():
    pass


def do_publish():
    pass


if __name__ == "__main__":
    sys.exit(-1)
