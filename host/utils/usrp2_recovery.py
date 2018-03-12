#!/usr/bin/env python
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
import optparse

BCAST_MAC_ADDR = 'ff:ff:ff:ff:ff:ff'
RECOVERY_ETHERTYPE = 0xbeee
IP_RECOVERY_CODE = 'addr'

def mac_addr_repr_to_binary_string(mac_addr):
    return ''.join([chr(int(x, 16)) for x in mac_addr.split(':')])

if __name__ == '__main__':
    parser = optparse.OptionParser(usage='usage: %prog [options]\n'+__doc__)
    parser.add_option('--ifc', type='string', help='ethernet interface name [default=%default]', default='eth0')
    parser.add_option('--new-ip', type='string', help='ip address to set [default=%default]', default='192.168.10.2')
    (options, args) = parser.parse_args()

    #create the raw socket
    print("Opening raw socket on interface:", options.ifc)
    soc = socket.socket(socket.PF_PACKET, socket.SOCK_RAW)
    soc.bind((options.ifc, RECOVERY_ETHERTYPE))

    #create the recovery packet
    print("Loading packet with ip address:", options.new_ip)
    packet = struct.pack(
        '!6s6sH4s4s',
        mac_addr_repr_to_binary_string(BCAST_MAC_ADDR),
        mac_addr_repr_to_binary_string(BCAST_MAC_ADDR),
        RECOVERY_ETHERTYPE,
        IP_RECOVERY_CODE,
        socket.inet_aton(options.new_ip),
    )

    print("Sending packet (%d bytes)"%len(packet))
    soc.send(packet)
    print("Done")
