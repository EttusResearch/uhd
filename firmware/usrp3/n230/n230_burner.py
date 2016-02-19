#!/usr/bin/env python
#
# Copyright 2014 Ettus Research LLC
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
import os.path
import sys
from array import array

########################################################################
# constants
########################################################################
N230_FLASH_COMM_UDP_PORT          = 49154
N230_FLASH_COMM_PAYLOAD_SIZE      = 128
N230_FLASH_COMM_SECTOR_SIZE       = 65536

N230_FLASH_COMM_FLAGS_ACK         = 0x00000001
N230_FLASH_COMM_FLAGS_CMD_MASK    = 0x00000FF0
N230_FLASH_COMM_FLAGS_ERROR_MASK  = 0xFF000000

N230_FLASH_COMM_CMD_READ_NV_DATA  = 0x00000010
N230_FLASH_COMM_CMD_WRITE_NV_DATA = 0x00000020
N230_FLASH_COMM_CMD_READ_FPGA     = 0x00000030
N230_FLASH_COMM_CMD_WRITE_FPGA    = 0x00000040
N230_FLASH_COMM_CMD_ERASE_FPGA    = 0x00000050

N230_FLASH_COMM_ERR_PKT_ERROR     = 0x80000000
N230_FLASH_COMM_ERR_CMD_ERROR     = 0x40000000
N230_FLASH_COMM_ERR_SIZE_ERROR    = 0x20000000

N230_FLASH_COMM_SAFE_IMG_BASE     = 0x000000
N230_FLASH_COMM_PROD_IMG_BASE     = 0x400000
N230_FLASH_COMM_FPGA_IMG_MAX_SIZE = 0x400000

UDP_MAX_XFER_BYTES = 256
UDP_TIMEOUT = 3

_seq = -1
def next_seq():
    global _seq
    _seq = _seq+1
    return _seq

def seq():
    return _seq

########################################################################
# helper functions
########################################################################

short = struct.Struct('>H')
ulong = struct.Struct('>I')

def unpack_flash_transaction(buf):
    (flags, seqno, offset, size) = struct.unpack_from('!LLLL', buf)
    check_error(flags)
    if (seqno != seq()):
        raise Exception("The flash transaction operation returned an incorrect sequence number")
    data = bytes()
    for i in xrange(16, len(buf), 1):
        data += buf[i]
    return (flags, offset, size, data)

def pack_flash_transaction(flags, offset, size, data):
    buf = bytes()
    buf = struct.pack('!LLLL', flags, next_seq(), offset, size)
    for i in range(N230_FLASH_COMM_PAYLOAD_SIZE):
        if (i < size):
            buf += struct.pack('!B', data[i])
        else:
            buf += struct.pack('!B', 0)
    return buf

def check_error(flags):
    if flags & N230_FLASH_COMM_ERR_PKT_ERROR == N230_FLASH_COMM_ERR_PKT_ERROR:
        raise Exception("The flash transaction operation returned a packet error")
    if flags & N230_FLASH_COMM_ERR_CMD_ERROR == N230_FLASH_COMM_ERR_CMD_ERROR:
        raise Exception("The flash transaction operation returned a command error")
    if flags & N230_FLASH_COMM_ERR_SIZE_ERROR == N230_FLASH_COMM_ERR_SIZE_ERROR:
        raise Exception("The flash transaction operation returned a size error")

def chunkify(stuff, n):
    return [stuff[i:i+n] for i in range(0, len(stuff), n)]

def draw_progress_bar(percent, bar_len = 32):
    sys.stdout.write("\r")
    progress = ""
    for i in range(bar_len):
        if i < int((bar_len * percent) / 100):
            progress += "="
        else:
            progress += "-"
    sys.stdout.write("[%s] %d%%" % (progress, percent))
    sys.stdout.flush()

########################################################################
# Burner class, holds a socket and send/recv routines
########################################################################
class ctrl_socket(object):
    def __init__(self, addr):
        self._safe_image = False
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self._sock.settimeout(UDP_TIMEOUT)
        self._sock.connect((addr, N230_FLASH_COMM_UDP_PORT))
        self.set_callbacks(lambda *a: None, lambda *a: None)

    def set_safe_image(self, noprompt):
        confirm_msg = ('----------------------------------------------------------------------\n'
                       'WARNING!!! You are about to access the safe-image stored in the flash \n'
                       '----------------------------------------------------------------------\n'
                       'Writing a non-functional image will brick the device.\n'
                       'Are you sure you want to proceed?')
        if not noprompt:
            if raw_input("%s (y/N) " % confirm_msg).lower() == 'y':
                self._safe_image = True
            else:
                print 'Aborted by user'
                sys.exit(1)
        else:
            print '[WARNING] Operating on safe image without a prompt as requested'
            self._safe_image = True

    def set_callbacks(self, progress_cb, status_cb):
        self._progress_cb = progress_cb
        self._status_cb = status_cb

    def send_and_recv(self, pkt):
        self._sock.send(pkt)
        return self._sock.recv(UDP_MAX_XFER_BYTES)
    
    def compute_offset(self, offset):
        base = N230_FLASH_COMM_SAFE_IMG_BASE if (self._safe_image) else N230_FLASH_COMM_PROD_IMG_BASE
        return base + offset

    def burn_fpga_to_flash(self, bitfile_path, noprompt):
        print '[BURN] Reading ' + bitfile_path + '...'
        with open(bitfile_path, 'rb') as bitfile:
            header = file_bytes = bitfile.read()
            if (self._safe_image != self.parse_bitfile_header(file_bytes)['safe-image']):
                confirm_msg = ('----------------------------------------------------------------------\n'
                               'WARNING!!! You are about to burn a safe image into a production slot  \n'
                               '           or a production image into a safe slot.                    \n'
                               '----------------------------------------------------------------------\n'
                               'This is dangerous and can cause the device to boot incorrectly.\n'
                               'Are you sure you want to proceed?')
                if not noprompt:
                    if raw_input("%s (y/N) " % confirm_msg).lower() != 'y':
                        print '[BURN] Aborted by user'
                        return
                else:
                    print '[WARNING] Burning image to the wrong slot without a prompt as requested'

            print '[BURN] Writing to flash...'
            pkt_chunks = chunkify(file_bytes, N230_FLASH_COMM_PAYLOAD_SIZE)
            offset = 0
            for pkt_data in pkt_chunks:
                pkt_data = array("B", pkt_data)
                size = N230_FLASH_COMM_PAYLOAD_SIZE if (len(pkt_data) >= N230_FLASH_COMM_PAYLOAD_SIZE) else len(pkt_data)
                #Erase sector
                if (offset % N230_FLASH_COMM_SECTOR_SIZE == 0):
                    flags = N230_FLASH_COMM_FLAGS_ACK|N230_FLASH_COMM_CMD_ERASE_FPGA
                    out_pkt = pack_flash_transaction(flags, self.compute_offset(offset), size, pkt_data)
                    (flags, real_offset, size, data) = unpack_flash_transaction(self.send_and_recv(out_pkt))
                #Write data
                flags = N230_FLASH_COMM_FLAGS_ACK|N230_FLASH_COMM_CMD_WRITE_FPGA
                out_pkt = pack_flash_transaction(flags, self.compute_offset(offset), size, pkt_data)
                (flags, real_offset, size, data) = unpack_flash_transaction(self.send_and_recv(out_pkt))
                #Increment
                offset += N230_FLASH_COMM_PAYLOAD_SIZE
                draw_progress_bar((((offset/N230_FLASH_COMM_PAYLOAD_SIZE)+1)*100)/len(pkt_chunks))
            print('\n[BURN] DONE')

    def parse_bitfile_header(self, header_bytes):
        xil_header = dict()
        n230_header = dict()
        n230_header['valid'] = False
        ptr = 0
        #Field 1
        if short.unpack(header_bytes[ptr:ptr+2])[0] == 9 and ulong.unpack(header_bytes[ptr+2:ptr+6])[0] == 0x0ff00ff0:
            #Headers
            ptr += short.unpack(header_bytes[ptr:ptr+2])[0] + 2
            ptr += short.unpack(header_bytes[ptr:ptr+2])[0] + 1
            #Fields a-d
            for keynum in range(0, 4):
                key = header_bytes[ptr]
                ptr += 1
                val_len = short.unpack(header_bytes[ptr:ptr+2])[0]
                ptr += 2
                val = header_bytes[ptr:ptr+val_len]
                ptr += val_len
                xil_header[key] = val
            #Field e
            ptr += 1
            length = ulong.unpack(header_bytes[ptr:ptr+4])[0]
            xil_header['bl'] = length     #Bitstream length
            ptr += 4
            xil_header['hl'] = ptr        #Header lengt

            #Map Xilinx header field to N230 specific ones
            if xil_header and xil_header['a'].split(';')[0] == 'n230':
                n230_header['valid'] = True
                n230_header['user-id'] = int(xil_header['a'].split(';')[1].split('=')[1], 16)
                n230_header['safe-image'] = (n230_header['user-id'] >> 16 == 0x5AFE)
                n230_header['product'] = xil_header['b']
                n230_header['timestamp'] = xil_header['c'] + ' ' + xil_header['d']
                n230_header['filesize'] = xil_header['hl'] + xil_header['bl']
        return n230_header

    def read_bitfile_header_from_flash(self):
        max_header_size = 1024  #Should be enough
        header_bytes = bytes()
        for offset in range(0, max_header_size, N230_FLASH_COMM_PAYLOAD_SIZE):
            #Read data
            flags = N230_FLASH_COMM_FLAGS_ACK|N230_FLASH_COMM_CMD_READ_FPGA
            out_pkt = pack_flash_transaction(flags, self.compute_offset(offset), N230_FLASH_COMM_PAYLOAD_SIZE, [0]*N230_FLASH_COMM_PAYLOAD_SIZE)
            (flags, real_offset, size, data) = unpack_flash_transaction(self.send_and_recv(out_pkt))
            header_bytes += data
        return self.parse_bitfile_header(header_bytes)

    def extract_fpga_from_flash(self, bitfile_path):
        header = self.read_bitfile_header_from_flash();
        if not header['valid']:
            raise Exception("Could not detect a vaild Xilinx .bit burned into the flash")
        max_offset = header['filesize']
        print '[EXTRACT] Writing ' + bitfile_path + '...'
        with open(bitfile_path, 'wb') as bitfile:
            for i in range(0, int(math.ceil(float(max_offset)/N230_FLASH_COMM_PAYLOAD_SIZE))):
                offset = i * N230_FLASH_COMM_PAYLOAD_SIZE
                size = N230_FLASH_COMM_PAYLOAD_SIZE if (max_offset - offset >= N230_FLASH_COMM_PAYLOAD_SIZE) else (max_offset - offset)
                #Read data
                flags = N230_FLASH_COMM_FLAGS_ACK|N230_FLASH_COMM_CMD_READ_FPGA
                out_pkt = pack_flash_transaction(flags, self.compute_offset(offset), size, [0]*N230_FLASH_COMM_PAYLOAD_SIZE)
                (flags, real_offset, size, data) = unpack_flash_transaction(self.send_and_recv(out_pkt))
                bitfile.write(data[:size])
                draw_progress_bar(((offset*100)/max_offset) + 1)
            print('\n[EXTRACT] DONE')

    def erase_fpga_from_flash(self):
        print '[ERASE] Erasing image from flash...'
        for offset in range(0, N230_FLASH_COMM_FPGA_IMG_MAX_SIZE, N230_FLASH_COMM_SECTOR_SIZE):
            flags = N230_FLASH_COMM_FLAGS_ACK|N230_FLASH_COMM_CMD_ERASE_FPGA
            out_pkt = pack_flash_transaction(flags, self.compute_offset(offset), N230_FLASH_COMM_PAYLOAD_SIZE, [0]*N230_FLASH_COMM_PAYLOAD_SIZE)
            (flags, real_offset, size, data) = unpack_flash_transaction(self.send_and_recv(out_pkt))
            draw_progress_bar(((offset+N230_FLASH_COMM_SECTOR_SIZE)*100)/N230_FLASH_COMM_FPGA_IMG_MAX_SIZE)
        print('\n[ERASE] DONE')

    def wipe_user_data(self, noprompt):
        confirm_msg = ('-------------------------------------------------------------------\n'
                       'WARNING!!! You are about to erase all the user data from the flash \n'
                       '-------------------------------------------------------------------\n'
                       'This will cause the device to lose the following:\n'
                       ' * IP Address (Will default to 192.168.10.2)\n'
                       ' * Subnet Mask (Will default to 255.255.255.2)\n'
                       ' * MAC Address\n'
                       ' * Serial Number\n'
                       ' * Hardware Revision\n'
                       ' * ...and other identification info\n'
                       'Are you sure you want to proceed?')
        if not noprompt:
            if raw_input("%s (y/N) " % confirm_msg).lower() == 'y':
                wipe_ok = True
            else:
                print '[WIPE] Aborted by user'
                wipe_ok = False
        else:
            print '[WARNING] Wiping user data without prompt a as requested'
            wipe_ok = True

        if wipe_ok:
            print '[WIPE] Erasing all user data from flash...'
            flags = N230_FLASH_COMM_FLAGS_ACK|N230_FLASH_COMM_CMD_WRITE_NV_DATA
            out_pkt = pack_flash_transaction(flags, 0, N230_FLASH_COMM_PAYLOAD_SIZE, [0xFF]*N230_FLASH_COMM_PAYLOAD_SIZE)
            (flags, real_offset, size, data) = unpack_flash_transaction(self.send_and_recv(out_pkt))
            print('[WIPE] DONE. Please power-cycle the device.')

    def print_status(self):
        header = self.read_bitfile_header_from_flash();
        if header['valid']:
            print('[STATUS] Detected a valid .bit header in the flash (Product = %s, Datestamp = %s%s)' % \
                  (header['product'], header['timestamp'], ', Safe-Image' if header['safe-image'] else ''))
        else:
            print('[STATUS] No .bit header detected. Either the flash is uninitialized or the image is corrupt.')


########################################################################
# command line options
########################################################################
def get_options():
    parser = optparse.OptionParser()
    parser.add_option("--addr", type="string",                 help="N230 device address", default='')
    parser.add_option("--status", action="store_true",         help="Print out the status of the burned image", default=False)
    parser.add_option("--erase", action="store_true",          help="Erase FPGA bitstream from flash", default=False)
    parser.add_option("--burn", type="string",                 help="Path to FPGA bitstream (.bit) to burn to flash", default=None)
    parser.add_option("--extract", type="string",              help="Destination bitfile to dump contents of the extracted image", default=None)
    parser.add_option("--safe_image", action="store_true",     help="Operate on the safe image. WARNING: This could be dangerous", default=False)
    parser.add_option("--wipe_user_data", action="store_true", help="Erase all user data like IP, MAC, S/N, etc from flash", default=False)
    parser.add_option("--no_prompt", action="store_true",      help="Suppress all warning prompts", default=False)
    (options, args) = parser.parse_args()
    return options

########################################################################
# main
########################################################################
if __name__=='__main__':
    options = get_options()

    if not options.addr: raise Exception('No address specified')

    ctrl_sock = ctrl_socket(addr=options.addr)
    
    # Initialize safe image selector first
    if options.safe_image:
        ctrl_sock.set_safe_image(options.no_prompt)

    if options.status:
        ctrl_sock.print_status()

    # Order of operations:
    # 1. Extract (if specified)
    # 2. Erase (if specified)
    # 3. Burn (if specified)
    
    if options.extract is not None:
        file_path = options.extract
        ctrl_sock.print_status()
        ctrl_sock.extract_fpga_from_flash(file_path)

    if options.erase:
        ctrl_sock.erase_fpga_from_flash()
        ctrl_sock.print_status()
    
    if options.burn is not None:
        file_path = options.burn
        extension = os.path.splitext(file_path)[1]
        if (extension.lower() == '.bit'):
            ctrl_sock.burn_fpga_to_flash(file_path, options.no_prompt)
            ctrl_sock.print_status()
        else:
            raise Exception("Unsupported FPGA bitfile format. You must use a .bit file.")

    if options.wipe_user_data:
        ctrl_sock.wipe_user_data(options.no_prompt)
