#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

UHD_INSTALL(FILES
    chdr_packet.hpp
    chdr_packet.ipp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/uhd/utils/chdr
    COMPONENT headers
)
