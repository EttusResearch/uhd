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
import array
import os.path
import sys
import time
try:
    import fcntl
    N230_DEVICE_DISCOVERY_AVAILABLE = True
except:
    N230_DEVICE_DISCOVERY_AVAILABLE = False

########################################################################
# constants
########################################################################
N230_FW_COMMS_UDP_PORT          = 49152
N230_FW_COMMS_MAX_DATA_WORDS    = 16

N230_FW_COMMS_FLAGS_ACK         = 0x00000001
N230_FW_COMMS_FLAGS_ERROR_MASK  = 0xF0000000
N230_FW_COMMS_FLAGS_CMD_MASK    = 0x000000F0

N230_FW_COMMS_CMD_ECHO          = 0x00000000
N230_FW_COMMS_CMD_POKE32        = 0x00000010
N230_FW_COMMS_CMD_PEEK32        = 0x00000020
N230_FW_COMMS_CMD_BLOCK_POKE32  = 0x00000030
N230_FW_COMMS_CMD_BLOCK_PEEK32  = 0x00000040

N230_FW_COMMS_ERR_PKT_ERROR     = 0x80000000
N230_FW_COMMS_ERR_CMD_ERROR     = 0x40000000
N230_FW_COMMS_ERR_SIZE_ERROR    = 0x20000000

N230_FW_COMMS_ID                = 0x0001ACE3

N230_FW_LOADER_ADDR             = 0xfa00
N230_FW_LOADER_DATA             = 0xfa04
N230_FW_LOADER_NUM_WORDS        = 8192
N230_FW_LOADER_BOOT_DONE_ADDR   = 0xA004
N230_FW_LOADER_BOOT_TIMEOUT     = 5

N230_JESD204_TEST               = 0xA014
N230_FPGA_HASH_ADDR             = 0xA010
N230_FW_HASH_ADDR               = 0x10004
N230_ICAP_ADDR                  = 0xF800
#ICAP_DUMMY_WORD               = 0xFFFFFFFF
#ICAP_SYNC_WORD                = 0xAA995566
#ICAP_TYPE1_NOP                = 0x20000000
#ICAP_WRITE_WBSTAR             = 0x30020001
#ICAP_WBSTAR_ADDR              = 0x00000000
#ICAP_WRITE_CMD                = 0x30008001
#ICAP_IPROG_CMD                = 0x0000000F
# Bit reversed values per Xilinx UG470 - Bits reversed within bytes.
ICAP_DUMMY_WORD               = 0xFFFFFFFF
ICAP_SYNC_WORD                = 0x5599AA66
ICAP_TYPE1_NOP                = 0x04000000
ICAP_WRITE_WBSTAR             = 0x0C400080
ICAP_WBSTAR_ADDR              = 0x00000000
ICAP_WRITE_CMD                = 0x0C000180
ICAP_IPROG_CMD                = 0x000000F0


UDP_MAX_XFER_BYTES  = 256
UDP_TIMEOUT         = 3
FPGA_LOAD_TIMEOUT   = 10

_seq = -1
def seq():
    global _seq
    _seq = _seq+1
    return _seq

########################################################################
# helper functions
########################################################################

def pack_fw_command(flags, seq, num_words, addr, data_arr):
    if (num_words > N230_FW_COMMS_MAX_DATA_WORDS):
        raise Exception("Data size too large. Firmware supports a max 16 words per block." % (addr))
    buf = bytes()
    buf = struct.pack('!IIIII', N230_FW_COMMS_ID, flags, seq, num_words, addr)
    for i in range(N230_FW_COMMS_MAX_DATA_WORDS):
        if (i < num_words):
            buf += struct.pack('!I', data_arr[i])
        else:
            buf += struct.pack('!I', 0)
    return buf

def unpack_fw_command(buf, fmt=None):
    (id, flags, seq, num_words, addr) = struct.unpack_from('!IIIII', buf)
    fw_check_error(flags)
    data = []
    if fmt is None:
        fmt = 'I'
    for i in xrange(20, len(buf), 4):
        data.append(struct.unpack('!'+fmt, buf[i:i+4])[0])
    return (flags, seq, num_words, addr, data)

def fw_check_error(flags):
    if flags & N230_FW_COMMS_ERR_PKT_ERROR == N230_FW_COMMS_ERR_PKT_ERROR:
        raise Exception("The fiwmware operation returned a packet error")
    if flags & N230_FW_COMMS_ERR_CMD_ERROR == N230_FW_COMMS_ERR_CMD_ERROR:
        raise Exception("The fiwmware operation returned a command error")
    if flags & N230_FW_COMMS_ERR_SIZE_ERROR == N230_FW_COMMS_ERR_SIZE_ERROR:
        raise Exception("The fiwmware operation returned a size error")

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
# Discovery class
########################################################################
class discovery_socket(object):
    def __init__(self):
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self._sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        self._sock.settimeout(0.250)

    def get_bcast_addrs(self):
        max_possible = 128  # arbitrary. raise if needed.
        num_bytes = max_possible * 32
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        names = array.array('B', '\0' * num_bytes)
        outbytes = struct.unpack('iL', fcntl.ioctl(
            s.fileno(),
            0x8912,  # SIOCGIFCONF
            struct.pack('iL', num_bytes, names.buffer_info()[0])
        ))[0]
        namestr = names.tostring()
        lst = []
        for i in range(0, outbytes, 40):
            name = namestr[i:i+16].split('\0', 1)[0]
            ip   = map(ord, namestr[i+20:i+24])
            mask = map(ord, fcntl.ioctl(s.fileno(), 0x891B, struct.pack('256s', name))[20:24])
            bcast = []
            for i in range(len(ip)):
                bcast.append((ip[i] | (~mask[i])) & 0xFF) 
            if (name != 'lo'):
                lst.append(str(bcast[0]) + '.' + str(bcast[1]) + '.' + str(bcast[2]) + '.' + str(bcast[3]))
        return lst

    def discover(self):
        addrs = []
        for bcast_addr in self.get_bcast_addrs():
            out_pkt = pack_fw_command(N230_FW_COMMS_CMD_ECHO|N230_FW_COMMS_FLAGS_ACK, seq(), 0, 0, [0])
            self._sock.sendto(out_pkt, (bcast_addr, N230_FW_COMMS_UDP_PORT))
            while 1:
                try:
                    (in_pkt, addr_pair) = self._sock.recvfrom(UDP_MAX_XFER_BYTES)
                    if len(in_pkt) < 20:
                        continue
                    (flags, ack_seq, block_size, addr, data) = unpack_fw_command(in_pkt)
                    addrs.append(addr_pair[0])
                except socket.error:
                    break
        return addrs


########################################################################
# Communications class, holds a socket and send/recv routine
########################################################################
class ctrl_socket(object):
    def __init__(self, addr, port):
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self._sock.settimeout(UDP_TIMEOUT)
        self._sock.connect((addr, port))
        self.set_callbacks(lambda *a: None, lambda *a: None)
        self.peek(0)    #Dummy read

    def set_callbacks(self, progress_cb, status_cb):
        self._progress_cb = progress_cb
        self._status_cb = status_cb

    def send(self, pkt):
        self._sock.send(pkt)

    def recv(self):
        return self._sock.recv(UDP_MAX_XFER_BYTES)

    def send_and_recv(self, pkt):
        self.send(pkt)
        return self.recv()

    def peek(self, peek_addr, fmt=None):
        out_pkt = pack_fw_command(N230_FW_COMMS_CMD_PEEK32|N230_FW_COMMS_FLAGS_ACK, seq(), 1, peek_addr, [0])
        in_pkt = self.send_and_recv(out_pkt)
        (flags, ack_seq, block_size, addr, data) = unpack_fw_command(in_pkt, fmt)
        return data[0]

    def peek64(self, peek_addr, fmt=None):
        out_pkt = pack_fw_command(N230_FW_COMMS_CMD_BLOCK_PEEK32|N230_FW_COMMS_FLAGS_ACK, seq(), 2, peek_addr, [0,0])
        in_pkt = self.send_and_recv(out_pkt)
        (flags, ack_seq, block_size, addr, data) = unpack_fw_command(in_pkt, fmt)
        return (data[0] | (data[1] << 32)) 

    def poke(self, poke_addr, poke_data, ack=True):
        ack_flag = N230_FW_COMMS_FLAGS_ACK if ack else 0
        out_pkt = pack_fw_command(N230_FW_COMMS_CMD_POKE32|ack_flag, seq(), 1, poke_addr, [poke_data])
        self.send(out_pkt)
        if ack:
            in_pkt = self.recv()
            (flags, ack_seq, block_size, addr, data) = unpack_fw_command(in_pkt)

    def live_load_firmware_bin(self, bin_path):
        raise Exception("live_load_firmware_bin not implemented yet!")

    def live_load_firmware_coe(self, coe_path):
        with open(coe_path, 'r') as coe_file:
            print("Loading %s..." % coe_path)
            coe_lines = [line.strip(',;\n ') for line in coe_file]
            start_index = coe_lines.index("memory_initialization_vector=") + 1
            coe_words = coe_lines[start_index:]
            if len(coe_words) != N230_FW_LOADER_NUM_WORDS:
                raise Exception("invalid COE file. Must contain 8192 words!")
            self.poke(N230_FW_LOADER_ADDR, 0)    #Load start address
            for i in range(0, len(coe_words)):
                self.poke(N230_FW_LOADER_DATA, int(coe_words[i],16), (i%10==0) and (i<len(coe_words)-1))
                draw_progress_bar(((i+1)*100)/len(coe_words))
            print("\nRebooting...")
            out_pkt = pack_fw_command(N230_FW_COMMS_CMD_POKE32, seq(), 1, N230_FW_LOADER_BOOT_DONE_ADDR, [1])
            self._sock.send(out_pkt)
            self._sock.settimeout(1)
            out_pkt = pack_fw_command(N230_FW_COMMS_CMD_PEEK32|N230_FW_COMMS_FLAGS_ACK, seq(), 1, 0, [0])
            for i in range(N230_FW_LOADER_BOOT_TIMEOUT):
                try:
                    self._sock.send(out_pkt)
                    in_pkt = self._sock.recv(UDP_MAX_XFER_BYTES)
                    print("Firmware is alive!")
                    self._sock.settimeout(UDP_TIMEOUT)
                    return
                except socket.error:
                    pass
            print("Firmware boot FAILED!!!")
            self._sock.settimeout(UDP_TIMEOUT)

    def read_hash(self):
        fpga_hash = self.peek(N230_FPGA_HASH_ADDR)
        fpga_status = "clean" if (fpga_hash & 0xf0000000 == 0x0) else "modified"
        fw_hash = self.peek(N230_FW_HASH_ADDR)
        fw_status = "clean" if (fw_hash & 0xf0000000 == 0x0) else "modified"
        print("FPGA Version     : %x (%s)" % (fpga_hash & 0xfffffff, fpga_status))
        print("Firmware Version : %x (%s)" % (fw_hash & 0xfffffff, fw_status))

    def is_claimed(self):
        claimed = self.peek(0x10008)
        print("Claimed          : %s") % ('YES' if claimed else 'NO')

    def reset_fpga(self):
        print("Reseting USRP...")
        ctrl_sock.poke(N230_ICAP_ADDR,ICAP_DUMMY_WORD)
        ctrl_sock.poke(N230_ICAP_ADDR,ICAP_TYPE1_NOP)
        ctrl_sock.poke(N230_ICAP_ADDR,ICAP_SYNC_WORD)
        ctrl_sock.poke(N230_ICAP_ADDR,ICAP_TYPE1_NOP)
        ctrl_sock.poke(N230_ICAP_ADDR,ICAP_WRITE_WBSTAR)
        ctrl_sock.poke(N230_ICAP_ADDR,ICAP_WBSTAR_ADDR)
        ctrl_sock.poke(N230_ICAP_ADDR,ICAP_TYPE1_NOP)
        ctrl_sock.poke(N230_ICAP_ADDR,ICAP_WRITE_CMD)
        ctrl_sock.poke(N230_ICAP_ADDR,ICAP_IPROG_CMD, False)
        print("Waiting for FPGA to load...")
        self._sock.settimeout(1)
        out_pkt = pack_fw_command(N230_FW_COMMS_CMD_ECHO|N230_FW_COMMS_FLAGS_ACK, seq(), 1, 0, [0])
        for i in range(FPGA_LOAD_TIMEOUT):
            try:
                in_pkt = self.send_and_recv(out_pkt)
                (flags, ack_seq, block_size, addr, data) = unpack_fw_command(in_pkt)
                print("FPGA loaded successfully.")
                self._sock.settimeout(UDP_TIMEOUT)
                return
            except socket.error:
                pass
        print("FPGA load FAILED!!!")
        self._sock.settimeout(UDP_TIMEOUT)

    def jesd204_test_connector(self):
        print("Testing JESD204 connectors. Molex cable #79576-2102 must be connected")
        ctrl_sock.poke(N230_JESD204_TEST,0x1)
        while True:
            jesd204_test_status = ctrl_sock.peek(N230_JESD204_TEST)
            if (jesd204_test_status & 0x10000 == 0x10000):
                break
        ctrl_sock.poke(N230_JESD204_TEST,0x0)
        if (jesd204_test_status & 0xFFFF != 0x0):
            print("JESD204 loopback test Failed!: Returned status is %4x" % (jesd204_test_status & 0xFFFF))
        else:
            print("JESD204 loopback test Passed.")

########################################################################
# command line options
########################################################################
def get_options():
    parser = optparse.OptionParser()
    parser.add_option("--discover", action="store_true",help="Find all devices connected N230 devices", default=False)
    parser.add_option("--addr", type="string",      help="N230 device address", default='')
    parser.add_option("--peek", type="int",         help="Read from memory map", default=None)
    parser.add_option("--poke", type="int",         help="Write to memory map", default=None)
    parser.add_option("--data", type="int",         help="Data for poke", default=None)
    parser.add_option("--fw",   type="string",      help="Path to FW image to load", default=None)
    parser.add_option("--hash", action="store_true",help="Display FPGA git hash", default=False)
    parser.add_option("--reset", action="store_true",help="Reset and Reload USRP FPGA.", default=False)
    parser.add_option("--jesd204test", action="store_true",help="Test mini-SAS connectors with loopback cable..", default=False)

    (options, args) = parser.parse_args()
    return options

########################################################################
# main
########################################################################
if __name__=='__main__':
    options = get_options()

    if options.discover:
        if N230_DEVICE_DISCOVERY_AVAILABLE:
            disc_sock = discovery_socket()
            for addr in disc_sock.discover():
                print '==== FOUND ' + addr + ' ===='
                ctrl_sock = ctrl_socket(addr, N230_FW_COMMS_UDP_PORT)
                ctrl_sock.read_hash()
                ctrl_sock.is_claimed()
            sys.exit()
        else:
            raise Exception('Discovery is only supported on Linux.')

    if not options.addr: 
        raise Exception('No address specified')

    ctrl_sock = ctrl_socket(options.addr, N230_FW_COMMS_UDP_PORT)
   
    if options.fw is not None:
        file_path = options.fw
        extension = os.path.splitext(file_path)[1]
        if (extension.lower() == '.coe'):
            ctrl_sock.live_load_firmware_coe(file_path)
        elif (extension.lower() == '.bin'):
            ctrl_sock.live_load_firmware_bin(file_path)
        else:
            raise Exception("Unsupported firmware file format")

    if options.hash:
        ctrl_sock.read_hash()

    if options.peek is not None:
        addr = options.peek
        data = ctrl_sock.peek(addr)
        print("PEEK[0x%x (%d)] => 0x%x (%d)" % (addr,addr,data,data))

    if options.poke is not None and options.data is not None:
        addr = options.poke
        data = options.data
        ctrl_sock.poke(addr,data)
        print("POKE[0x%x (%d)] <= 0x%x (%d)" % (addr,addr,data,data))

    if options.reset:
        ctrl_sock.reset_fpga()

    if options.jesd204test:
        ctrl_sock.jesd204_test_connector()
