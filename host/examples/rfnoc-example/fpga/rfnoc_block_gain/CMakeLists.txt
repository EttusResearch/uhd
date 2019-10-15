#
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#


# This macro will tell CMake that this directory contains an RFNoC block. It
# will parse Makefile.srcs to see which files need to be installed, and it will
# register a testbench target for this directory.
RFNOC_REGISTER_BLOCK_DIR()

# This will do the same, but it will skip the testbench target.
#RFNOC_REGISTER_BLOCK_DIR(NOTESTBENCH)


