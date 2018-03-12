#!/usr/bin/env python
#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

import socket
import binascii

UDP_IP = "0.0.0.0"
UDP_PORT = 5000

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(((UDP_IP, UDP_PORT)))

send_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)


while True:
    buf = bytearray(4096)
    nbytes, sender = sock.recvfrom_into(buf, 4096)
    print(sender)
    print("number of bytes: {}".format(nbytes))
    print("received bytes: {}".format(binascii.b2a_hex(buf[:nbytes])))

    send_sock.sendto(buf[:nbytes], sender)
