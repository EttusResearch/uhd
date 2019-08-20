#
# Copyright 2019 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

file(GLOB yml_files "*.yml")

# We always need this, even when RFNoC is 'disabled'
UHD_INSTALL(
    FILES ${yml_files}
    DESTINATION ${PKG_DATA_DIR}/rfnoc/blocks
    COMPONENT headers # TODO: Different component
)
