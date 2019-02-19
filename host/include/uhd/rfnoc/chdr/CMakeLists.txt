#
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

UHD_INSTALL(FILES
    chdr_header.hpp
    chdr_packet.hpp
    DESTINATION ${PKG_DATA_DIR}/rfnoc/chdr
    COMPONENT headers # TODO: Different component
)
