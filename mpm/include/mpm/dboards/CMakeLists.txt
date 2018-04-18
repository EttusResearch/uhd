#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
IF(ENABLE_MAGNESIUM)
    INSTALL(FILES
        magnesium_manager.hpp
        DESTINATION ${INCLUDE_DIR}/mpm/dboards
    )
ELSEIF(ENABLE_E320)
    INSTALL(FILES
        neon_manager.hpp
        DESTINATION ${INCLUDE_DIR}/mpm/dboards
    )
ENDIF(ENABLE_MAGNESIUM)
