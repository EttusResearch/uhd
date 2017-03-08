#!/usr/bin/env python

import libpyusrp_periphs as p

dev = p.n3xx.periph_manager("/dev/spidev1.0")
lmk = dev.get_clock_gen()
lmk.verify_chip_id()
