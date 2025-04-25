#
# Copyright 2022 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

UHD_INSTALL(FILES
    extension.hpp

    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/uhd/extension
    COMPONENT headers
)
