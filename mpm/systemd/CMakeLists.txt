#
# Copyright 2018 Ettus Research, National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

include(FindPkgConfig)
PKG_CHECK_MODULES(SYSTEMD "systemd" REQUIRED)

PKG_GET_VARIABLE(SYSTEMD_SYSTEM_UNITDIR systemd systemdsystemunitdir)
message(STATUS "Systemd system unit directory ${SYSTEMD_SYSTEM_UNITDIR}")

PKG_GET_VARIABLE(SYSTEMD_SYSCTLDIR systemd sysctldir)
message(STATUS "Systemd sysctl directory ${SYSTEMD_SYSCTLDIR}")

PKG_GET_VARIABLE(UDEV_UDEVDIR udev udevdir)
message(STATUS "Udev directory ${UDEV_UDEVDIR}")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/system/usrp-hwd.service.in
    ${CMAKE_CURRENT_BINARY_DIR}/system/usrp-hwd.service
)

# service files
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/system/usrp-hwd.service
  DESTINATION ${SYSTEMD_SYSTEM_UNITDIR}
)

# network configuration files
install(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/network/eth0.network
  ${CMAKE_CURRENT_SOURCE_DIR}/network/int0.network
  ${CMAKE_CURRENT_SOURCE_DIR}/network/sfp0.network
  ${CMAKE_CURRENT_SOURCE_DIR}/network/sfp0_1.network
  ${CMAKE_CURRENT_SOURCE_DIR}/network/sfp0_2.network
  ${CMAKE_CURRENT_SOURCE_DIR}/network/sfp0_3.network
  ${CMAKE_CURRENT_SOURCE_DIR}/network/sfp1.network
  ${CMAKE_CURRENT_SOURCE_DIR}/network/sfp1_1.network
  ${CMAKE_CURRENT_SOURCE_DIR}/network/sfp1_2.network
  ${CMAKE_CURRENT_SOURCE_DIR}/network/sfp1_3.network
  DESTINATION ${SYSTEMD_SYSTEM_UNITDIR}/../network
)

# sysctl configuration files
install(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/sysctl/usrp-hwd.conf
  DESTINATION ${SYSTEMD_SYSCTLDIR}
)

# udev rules
install(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/udev/${MPM_DEVICE}/70-sfp-net.rules
  DESTINATION ${UDEV_UDEVDIR}/rules.d
)
