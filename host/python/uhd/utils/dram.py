#
# Copyright 2025 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

""" @package rfnoc Python UHD module containing DRAM helpers.
"""

from .. import libpyuhd as lib

upload = lib.rfnoc.dram.upload
download = lib.rfnoc.dram.download
