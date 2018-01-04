#
# Copyright 2018 Ettus Research, National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(SYSTEMD "systemd" REQUIRED)

PKG_GET_VARIABLE(SYSTEMD_SYSTEM_UNITDIR systemd systemdsystemunitdir)
MESSAGE(STATUS "Systemd system unit directory ${SYSTEMD_SYSTEM_UNITDIR}")

CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/usrp-hwd.service.in
    ${CMAKE_CURRENT_BINARY_DIR}/usrp-hwd.service
)

INSTALL(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/usrp-hwd.service
  DESTINATION ${SYSTEMD_SYSTEM_UNITDIR}
)
