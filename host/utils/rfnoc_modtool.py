#!/usr/bin/env python3
"""RFNoC Modtool: The tool to create and manipulate RFNoC OOT modules.

Copyright 2024 Ettus Research, A National Instrument Brand

SPDX-License-Identifier: GPL-3.0-or-later
"""

import sys

from uhd.rfnoc_utils import rfnoc_modtool

if __name__ == "__main__":
    sys.exit(rfnoc_modtool.main())
