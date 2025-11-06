#
# Copyright 2022 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

UHD_INSTALL(FILES
    expert_container.hpp
    expert_factory.hpp
    expert_nodes.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/uhd/experts
    COMPONENT headers
)
