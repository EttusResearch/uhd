#!/usr/bin/env python3
#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

import libpyusrp_periphs as p

dev = p.n3xx.periph_manager("/dev/spidev1.0")
lmk = dev.get_clock_gen()
lmk.verify_chip_id()
