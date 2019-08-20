#
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

file(GLOB yml_files "*.yml")
file(GLOB json_files "*.json")

# We always need this, even when RFNoC is 'disabled'
UHD_INSTALL(
    FILES ${yml_files} ${json_files}
    DESTINATION ${PKG_DATA_DIR}/rfnoc/core
    COMPONENT headers # TODO: Different component
)
