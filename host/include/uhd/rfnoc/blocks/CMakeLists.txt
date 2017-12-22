#
# Copyright 2014-2016 Ettus Research LLC
#
# SPDX-License-Identifier: GPL-3.0
#

FILE(GLOB xml_files "*.xml")

# We always need this, even when RFNoC is 'disabled'
UHD_INSTALL(
    FILES ${xml_files}
    DESTINATION ${PKG_DATA_DIR}/rfnoc/blocks
    COMPONENT headers # TODO: Different component
)
