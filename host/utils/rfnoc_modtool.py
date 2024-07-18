#!/usr/bin/env python3
"""RFNoC Modtool: The tool to create and manipulate RFNoC OOT modules.

Copyright 2024 Ettus Research, A National Instrument Brand

SPDX-License-Identifier: GPL-3.0-or-later
"""

import os
import sys

from uhd.rfnoc_utils import rfnoc_modtool


def get_pkg_dir():
    """Return package data directory.

    This is the main UHD installation path for package data (e.g., /usr/share/uhd).
    """
    return os.path.normpath("@CONFIG_PATH@")


if __name__ == "__main__":
    sys.exit(rfnoc_modtool.main(get_pkg_dir()))
