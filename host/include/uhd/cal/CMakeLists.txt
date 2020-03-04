#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

UHD_INSTALL(FILES
    database.hpp
    container.hpp
    interpolation.hpp
    DESTINATION ${INCLUDE_DIR}/uhd/cal
    COMPONENT headers
)

