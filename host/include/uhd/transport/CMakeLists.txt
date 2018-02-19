#
# Copyright 2010-2014 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

UHD_INSTALL(FILES
    bounded_buffer.hpp
    bounded_buffer.ipp
    buffer_pool.hpp
    chdr.hpp
    if_addrs.hpp
    udp_constants.hpp
    udp_simple.hpp
    udp_zero_copy.hpp
    tcp_zero_copy.hpp
    usb_control.hpp
    usb_zero_copy.hpp
    usb_device_handle.hpp
    vrt_if_packet.hpp
    zero_copy.hpp
    DESTINATION ${INCLUDE_DIR}/uhd/transport
    COMPONENT headers
)
