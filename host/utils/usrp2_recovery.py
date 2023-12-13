#!/usr/bin/env python3
#
# Copyright 2010-2011 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

"""
The usrp2 recovery app:

When the usrp2 has an unknown or bad ip address in its eeprom,
it may not be possible to communicate with the usrp2 over ip/udp.

This app will send a raw ethernet packet to bypass the ip layer.
The packet will contain a known ip address to burn into eeprom.
Because the recovery packet is sent with a broadcast mac address,
only one usrp2 should be present on the interface upon execution.

This app requires super-user privileges and only works on linux.
"""

import socket
import struct
import argparse

BCAST_MAC_ADDR = 'ff:ff:ff:ff:ff:ff'
RECOVERY_ETHERTYPE = 0xbeee
IP_RECOVERY_CODE = b'addr'

def mac_addr_repr_to_binary_string(mac_addr):
    """
    Convert string representation of MAC address (xx:yy:zz:aa:bb:cc) into a
    binary string.
    """
    return bytes(bytearray([int(x, 16) for x in mac_addr.split(':')]))

if __name__ == '__main__':
    parser = argparse.ArgumentParser(usage='usage: %(prog)s [options]\n'+__doc__)
    parser.add_argument(
        '--ifc',
        help='ethernet interface name [default=%(default)s]',
        default='eth0')
    parser.add_argument(
        '--new-ip',
        help='ip address to set [default=%(default)s]',
        default='192.168.10.2')
    args = parser.parse_args()

    #create the raw socket
    print("Opening raw socket on interface:", args.ifc)
    soc = socket.socket(socket.PF_PACKET, socket.SOCK_RAW)
    soc.bind((args.ifc, RECOVERY_ETHERTYPE))

    #create the recovery packet
    print("Loading packet with ip address:", args.new_ip)
    packet = struct.pack(
        '!6s6sH4s4s',
        mac_addr_repr_to_binary_string(BCAST_MAC_ADDR),
        mac_addr_repr_to_binary_string(BCAST_MAC_ADDR),
        RECOVERY_ETHERTYPE,
        IP_RECOVERY_CODE,
        socket.inet_aton(args.new_ip),
    )

    print("Sending packet ({} bytes)".format(len(packet)))
    soc.send(packet)
    print("Done")
