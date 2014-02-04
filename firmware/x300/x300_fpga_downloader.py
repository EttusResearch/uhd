#!/usr/bin/env python
#
# Copyright 2010-2011 Ettus Research LLC
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

import optparse
import math
import socket
import struct
import time
import sys

########################################################################
# constants
########################################################################
B250_FPGA_IMAGE_SIZE_BYTES  = 15877916
B250_FPGA_PROG_UDP_PORT     = 49157

B250_FPGA_PROG_FLAGS_ACK     = 1
B250_FPGA_PROG_FLAGS_ERROR   = 2
B250_FPGA_PROG_FLAGS_INIT    = 4
B250_FPGA_PROG_FLAGS_CLEANUP = 8
B250_FPGA_PROG_FLAGS_ERASE   = 16
B250_FPGA_PROG_FLAGS_VERIFY  = 32
B250_FPGA_PROG_CONFIGURE     = 64
B250_FPGA_PROG_CONFIG_STATUS = 128

B250_FLASH_SECTOR_SIZE      = 131072
B250_PACKET_SIZE            = 256
B250_FPGA_SECTOR_START      = 16

B250_MAX_RESPONSE_BYTES     = 128
UDP_TIMEOUT                 = 3
FPGA_LOAD_TIMEOUT           = 15

########################################################################
# helper functions
########################################################################
def get_options():
    parser = optparse.OptionParser()
    parser.add_option("--addr", type="string",          help="USRP-B250 device address",            default='')
    parser.add_option("--download", type="string",      help="Path to the FPGA image to download.", default=None)
    parser.add_option("--posc", type="string",          help="Path to the POSC image to download.", default=None)
    parser.add_option("--program", action="store_true", help="Program the FPGA from flash.",        default=False)
    parser.add_option("--verify", action="store_true",  help="Verify data downloaded to flash.",    default=False)
    (options, args) = parser.parse_args()
    return options

def draw_progress_bar(percent, bar_len = 28):
    sys.stdout.write("\r")
    progress = ""
    for i in range(bar_len):
        if i < int((bar_len * percent) / 100):
            progress += "="
        else:
            progress += "-"
    sys.stdout.write("[%s] %d%%" % (progress, percent))
    sys.stdout.flush()

def unpack_x300_fpga_flags_fmt(s):
    return struct.unpack('!L', s)[0] #(flags)

def pack_x300_fpga_prog_fmt(flags, sector, offset, size, data):
    return struct.pack("!LLLL%dH" % (len(data),), flags, sector, offset, size, *data)

def bit_reverse(byte):
    b = ord(byte)
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1
    return chr(b)

def chunkify(stuff, n):
    return [stuff[i:i+n] for i in range(0, len(stuff), n)]

########################################################################
# Burner class, holds a socket and send/recv routines
########################################################################
class ctrl_socket(object):
    def __init__(self, addr):
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self._sock.settimeout(UDP_TIMEOUT)
        self._sock.connect((addr, B250_FPGA_PROG_UDP_PORT))
        self.set_callbacks(lambda *a: None, lambda *a: None)

    def set_callbacks(self, progress_cb, status_cb):
        self._progress_cb = progress_cb
        self._status_cb = status_cb

    def send_without_ack(self, pkt):
        self._sock.send(pkt)

    def send_with_ack(self, out_pkt, message):
        self.send_without_ack(out_pkt)
        in_pkt = self._sock.recv(B250_MAX_RESPONSE_BYTES)
        flags = unpack_x300_fpga_flags_fmt(in_pkt)
        if flags & B250_FPGA_PROG_FLAGS_ERROR == B250_FPGA_PROG_FLAGS_ERROR:
            raise Exception(message)

    def download_fpga(self, image, verify):
        print("Reading FPGA image file...")
        fpga_file = open(image, 'rb')
        print("Processing FPGA image...")
        fpga_image = ''.join([ bit_reverse(b) for b in fpga_file.read() ])
        
        print("Writing FPGA image to flash...")
        out_pkt = pack_x300_fpga_prog_fmt(B250_FPGA_PROG_FLAGS_ACK|B250_FPGA_PROG_FLAGS_INIT,
                                          0, 0, 0, [0]*(B250_PACKET_SIZE/2))
        self.send_with_ack(out_pkt, "Flash initialization failed!!!")

        sector_chunks = chunkify(fpga_image, B250_FLASH_SECTOR_SIZE)
        for sector_idx in range(len(sector_chunks)):
            packet_chunks = chunkify(sector_chunks[sector_idx], B250_PACKET_SIZE)
            offset = 0
            for packet_data in packet_chunks:
                data = struct.unpack("%dH" % (len(packet_data) / 2), packet_data)
                flags = B250_FPGA_PROG_FLAGS_ACK
                if offset == 0: flags |= B250_FPGA_PROG_FLAGS_ERASE
                if verify: flags |= B250_FPGA_PROG_FLAGS_VERIFY
                out_pkt = pack_x300_fpga_prog_fmt(flags, B250_FPGA_SECTOR_START+sector_idx, offset, len(data), data)
                self.send_with_ack(out_pkt, "Transfer failed or data verification failed!!!")
                offset += len(data)
            draw_progress_bar(((sector_idx+1)*100)/len(sector_chunks))

        print(' ')
        out_pkt = pack_x300_fpga_prog_fmt(B250_FPGA_PROG_FLAGS_ACK|B250_FPGA_PROG_FLAGS_CLEANUP,
                                          0, 0, 0, [0]*(B250_PACKET_SIZE/2))
        self.send_with_ack(out_pkt, "Flash cleanup failed!!!")

    def program(self):
        print("Programming FPGA from flash...")
        out_pkt = pack_x300_fpga_prog_fmt(B250_FPGA_PROG_CONFIGURE, 0, 0, 0, [0]*(B250_PACKET_SIZE/2))
        self.send_without_ack(out_pkt)
        print("Waiting for FPGA to load...")
        self._sock.settimeout(1)
        out_pkt = pack_x300_fpga_prog_fmt(B250_FPGA_PROG_FLAGS_ACK|B250_FPGA_PROG_CONFIG_STATUS, 0, 0, 0, [0]*(B250_PACKET_SIZE/2))
        for i in range(FPGA_LOAD_TIMEOUT):
            try:
                self.send_without_ack(out_pkt)
                in_pkt = self._sock.recv(B250_MAX_RESPONSE_BYTES)
                flags = unpack_x300_fpga_flags_fmt(in_pkt)
                if not(flags & B250_FPGA_PROG_FLAGS_ERROR == B250_FPGA_PROG_FLAGS_ERROR):
                    break
            except socket.error:
                pass


########################################################################
# main
########################################################################
if __name__=='__main__':
    options = get_options()

    if not options.addr: 
        raise Exception('No address specified')

    status = ctrl_socket(addr=options.addr)

    start_time = time.time()
    if options.download is not None:
        status.download_fpga(options.download, options.verify)

    if options.program:
        status.program()

    print("(Elapsed time: %.1fs)" % (time.time() - start_time))
