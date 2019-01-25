#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
if(ENABLE_MAGNESIUM)
    install(FILES
        magnesium_manager.hpp
        DESTINATION ${INCLUDE_DIR}/mpm/dboards
    )
elseif(ENABLE_E320)
    install(FILES
        neon_manager.hpp
        DESTINATION ${INCLUDE_DIR}/mpm/dboards
    )
elseif(ENABLE_E300)
    install(FILES
        e31x_db_manager.hpp
        DESTINATION ${INCLUDE_DIR}/mpm/dboards
    )
endif(ENABLE_MAGNESIUM)
