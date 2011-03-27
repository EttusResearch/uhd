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

# TODO: make it autodetect UHD devices
# TODO: you should probably watch sequence numbers
# TODO: validate images in 1) size and 2) header content so you can't write a Justin Bieber MP3 to Flash

import optparse
import math
import os
import re
import struct
import socket
import sys
import time

########################################################################
# constants
########################################################################
UDP_FW_UPDATE_PORT = 49154
UDP_MAX_XFER_BYTES = 1024
UDP_TIMEOUT = 3
UDP_POLL_INTERVAL = 0.10 #in seconds

USRP2_FW_PROTO_VERSION = 7 #should be unused after r6

#from bootloader_utils.h

FPGA_IMAGE_SIZE_BYTES = 1572864
FW_IMAGE_SIZE_BYTES = 31744
SAFE_FPGA_IMAGE_LOCATION_ADDR = 0x00000000
SAFE_FW_IMAGE_LOCATION_ADDR = 0x003F0000
PROD_FPGA_IMAGE_LOCATION_ADDR = 0x00180000
PROD_FW_IMAGE_LOCATION_ADDR = 0x00300000

FLASH_DATA_PACKET_SIZE = 256

#see fw_common.h
FLASH_ARGS_FMT = '!LLLLL256s'
FLASH_INFO_FMT = '!LLLLL256x'
FLASH_IP_FMT =   '!LLLL260x'

class update_id_t:
  USRP2_FW_UPDATE_ID_WAT = ord(' ')
  USRP2_FW_UPDATE_ID_OHAI_LOL = ord('a')
  USRP2_FW_UPDATE_ID_OHAI_OMG = ord('A')
  USRP2_FW_UPDATE_ID_WATS_TEH_FLASH_INFO_LOL = ord('f')
  USRP2_FW_UPDATE_ID_HERES_TEH_FLASH_INFO_OMG = ord('F')
  USRP2_FW_UPDATE_ID_ERASE_TEH_FLASHES_LOL = ord('e')
  USRP2_FW_UPDATE_ID_ERASING_TEH_FLASHES_OMG = ord('E')
  USRP2_FW_UPDATE_ID_R_U_DONE_ERASING_LOL = ord('d')
  USRP2_FW_UPDATE_ID_IM_DONE_ERASING_OMG = ord('D')
  USRP2_FW_UPDATE_ID_NOPE_NOT_DONE_ERASING_OMG = ord('B')
  USRP2_FW_UPDATE_ID_WRITE_TEH_FLASHES_LOL = ord('w')
  USRP2_FW_UPDATE_ID_WROTE_TEH_FLASHES_OMG = ord('W')
  USRP2_FW_UPDATE_ID_READ_TEH_FLASHES_LOL = ord('r')
  USRP2_FW_UPDATE_ID_KK_READ_TEH_FLASHES_OMG = ord('R')
  USRP2_FW_UPDATE_ID_RESET_MAH_COMPUTORZ_LOL = ord('s')
  USRP2_FW_UPDATE_ID_RESETTIN_TEH_COMPUTORZ_OMG = ord('S')
  USRP2_FW_UPDATE_ID_KTHXBAI = ord('~')

_seq = -1
def seq():
    global _seq
    _seq = _seq+1
    return _seq

########################################################################
# helper functions
########################################################################
def unpack_flash_args_fmt(s):
    return struct.unpack(FLASH_ARGS_FMT, s) #(proto_ver, pktid, seq, flash_addr, length, data)

def unpack_flash_info_fmt(s):
    return struct.unpack(FLASH_INFO_FMT, s) #(proto_ver, pktid, seq, sector_size_bytes, memory_size_bytes)

def unpack_flash_ip_fmt(s):
    return struct.unpack(FLASH_IP_FMT, s) #(proto_ver, pktid, seq, ip_addr)

def pack_flash_args_fmt(proto_ver, pktid, seq, flash_addr, length, data):
    return struct.pack(FLASH_ARGS_FMT, proto_ver, pktid, seq, flash_addr, length, data)

def pack_flash_info_fmt(proto_ver, pktid, seq, sector_size_bytes, memory_size_bytes):
    return struct.pack(FLASH_INFO_FMT, proto_ver, pktid, seq, sector_size_bytes, memory_size_bytes)

def is_valid_fpga_image(fpga_image):
    for i in range(0,63):
      if ord(fpga_image[i]) == 0xFF:
        continue
      if ord(fpga_image[i]) == 0xAA and ord(fpga_image[i+1]) == 0x99:
        return 1

    return 0

def is_valid_fw_image(fw_image):
    for i in range(0,4):
      if ord(fw_image[i]) != 0x0B:
          return 0;

    return 1

########################################################################
# Burner class, holds a socket and send/recv routines
########################################################################
class burner_socket(object):
    def __init__(self, addr):
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self._sock.settimeout(UDP_TIMEOUT)
        self._sock.connect((addr, UDP_FW_UPDATE_PORT))
        self.set_callbacks(lambda *a: None, lambda *a: None)
        self.init_update() #check that the device is there

    def set_callbacks(self, progress_cb, status_cb):
        self._progress_cb = progress_cb
        self._status_cb = status_cb

    def send_and_recv(self, pkt):
        self._sock.send(pkt)
        return self._sock.recv(UDP_MAX_XFER_BYTES)

    #just here to validate comms
    def init_update(self):
        out_pkt = pack_flash_args_fmt(USRP2_FW_PROTO_VERSION, update_id_t.USRP2_FW_UPDATE_ID_OHAI_LOL, seq(), 0, 0, "")
        try: in_pkt = self.send_and_recv(out_pkt)
        except socket.timeout: raise Exception, "No response from device"
        (proto_ver, pktid, rxseq, ip_addr) = unpack_flash_ip_fmt(in_pkt)
        if pktid == update_id_t.USRP2_FW_UPDATE_ID_OHAI_OMG:
            print "USRP-N2XX found."
        else:
            raise Exception, "Invalid reply received from device."

        #  print "Incoming:\n\tVer: %i\n\tID: %c\n\tSeq: %i\n\tIP: %i\n" % (proto_ver, chr(pktid), rxseq, ip_addr)

    def get_flash_info(self):
        out_pkt = pack_flash_args_fmt(USRP2_FW_PROTO_VERSION, update_id_t.USRP2_FW_UPDATE_ID_WATS_TEH_FLASH_INFO_LOL, seq(), 0, 0, "")
        in_pkt = self.send_and_recv(out_pkt)

        (proto_ver, pktid, rxseq, sector_size_bytes, memory_size_bytes) = unpack_flash_info_fmt(in_pkt)

        if pktid != update_id_t.USRP2_FW_UPDATE_ID_HERES_TEH_FLASH_INFO_OMG:
            raise Exception, "Invalid reply %c from device." % (chr(pktid))

        return (memory_size_bytes, sector_size_bytes)

    def burn_fw(self, fw, fpga, reset, safe):
        (flash_size, sector_size) = self.get_flash_info()

        print "Flash size: %i\nSector size: %i\n\n" % (flash_size, sector_size)

        if fpga:
            if safe: image_location = SAFE_FPGA_IMAGE_LOCATION_ADDR
            else:    image_location = PROD_FPGA_IMAGE_LOCATION_ADDR

            fpga_file = open(fpga, 'rb')
            fpga_image = fpga_file.read()

            if len(fpga_image) > FPGA_IMAGE_SIZE_BYTES:
                print "Error: FPGA image file too large."
                return 0

            if not is_valid_fpga_image(fpga_image):
                print "Error: Invalid FPGA image file."
                return 0

            print "Begin FPGA write: this should take about 1 minute..."
            start_time = time.time()
            self.erase_image(image_location, FPGA_IMAGE_SIZE_BYTES)
            self.write_image(fpga_image, image_location)
            self.verify_image(fpga_image, image_location)
            print "Time elapsed: %f seconds"%(time.time() - start_time)
            print "\n\n"

        if fw:
            if safe: image_location = SAFE_FW_IMAGE_LOCATION_ADDR
            else:    image_location = PROD_FW_IMAGE_LOCATION_ADDR

            fw_file = open(fw, 'rb')
            fw_image = fw_file.read()

            if len(fw_image) > FW_IMAGE_SIZE_BYTES:
                print "Error: Firmware image file too large."
                return 0

            if not is_valid_fw_image(fw_image):
                print "Error: Invalid firmware image file."
                return 0

            print "Begin firmware write: this should take about 1 second..."
            start_time = time.time()
            self.erase_image(image_location, FW_IMAGE_SIZE_BYTES)
            self.write_image(fw_image, image_location)
            self.verify_image(fw_image, image_location)
            print "Time elapsed: %f seconds"%(time.time() - start_time)
            print "\n\n"

        if reset: self.reset_usrp()

    def write_image(self, image, addr):
        print "Writing image"
        self._status_cb("Writing")
        writedata = image
        #we split the image into smaller (256B) bits and send them down the wire
        while writedata:
            out_pkt = pack_flash_args_fmt(USRP2_FW_PROTO_VERSION, update_id_t.USRP2_FW_UPDATE_ID_WRITE_TEH_FLASHES_LOL, seq(), addr, FLASH_DATA_PACKET_SIZE, writedata[:FLASH_DATA_PACKET_SIZE])
            in_pkt = self.send_and_recv(out_pkt)

            (proto_ver, pktid, rxseq, flash_addr, rxlength, data) = unpack_flash_args_fmt(in_pkt)

            if pktid != update_id_t.USRP2_FW_UPDATE_ID_WROTE_TEH_FLASHES_OMG:
              raise Exception, "Invalid reply %c from device." % (chr(pktid))

            writedata = writedata[FLASH_DATA_PACKET_SIZE:]
            addr += FLASH_DATA_PACKET_SIZE
            self._progress_cb(float(len(image)-len(writedata))/len(image))

    def verify_image(self, image, addr):
        print "Verifying data"
        self._status_cb("Verifying")
        readsize = len(image)
        readdata = str()
        while readsize > 0:
            if readsize < FLASH_DATA_PACKET_SIZE: thisreadsize = readsize
            else: thisreadsize = FLASH_DATA_PACKET_SIZE
            out_pkt = pack_flash_args_fmt(USRP2_FW_PROTO_VERSION, update_id_t.USRP2_FW_UPDATE_ID_READ_TEH_FLASHES_LOL, seq(), addr, thisreadsize, "")
            in_pkt = self.send_and_recv(out_pkt)

            (proto_ver, pktid, rxseq, flash_addr, rxlength, data) = unpack_flash_args_fmt(in_pkt)

            if pktid != update_id_t.USRP2_FW_UPDATE_ID_KK_READ_TEH_FLASHES_OMG:
              raise Exception, "Invalid reply %c from device." % (chr(pktid))

            readdata += data[:thisreadsize]
            readsize -= FLASH_DATA_PACKET_SIZE
            addr += FLASH_DATA_PACKET_SIZE
            self._progress_cb(float(len(readdata))/len(image))

        print "Read back %i bytes" % len(readdata)
        #  print readdata

        #  for i in range(256, 512):
        #    print "out: %i in: %i" % (ord(image[i]), ord(readdata[i]))

        if readdata != image:
            raise Exception, "Verify failed. Image did not write correctly."
        else:
            print "Success."

    def read_image(self, image, size, addr):
        print "Reading image"
        readsize = size
        readdata = str()
        while readsize > 0:
            if readsize < FLASH_DATA_PACKET_SIZE: thisreadsize = readsize
            else: thisreadsize = FLASH_DATA_PACKET_SIZE
            out_pkt = pack_flash_args_fmt(USRP2_FW_PROTO_VERSION, update_id_t.USRP2_FW_UPDATE_ID_READ_TEH_FLASHES_LOL, seq(), addr, thisreadsize, "")
            in_pkt = self.send_and_recv(out_pkt)

            (proto_ver, pktid, rxseq, flash_addr, rxlength, data) = unpack_flash_args_fmt(in_pkt)

            if pktid != update_id_t.USRP2_FW_UPDATE_ID_KK_READ_TEH_FLASHES_OMG:
              raise Exception, "Invalid reply %c from device." % (chr(pktid))

            readdata += data[:thisreadsize]
            readsize -= FLASH_DATA_PACKET_SIZE
            addr += FLASH_DATA_PACKET_SIZE

        print "Read back %i bytes" % len(readdata)

        #write to disk
        f = open(image, 'w')
        f.write(readdata)
        f.close()

    def reset_usrp(self):
        out_pkt = pack_flash_args_fmt(USRP2_FW_PROTO_VERSION, update_id_t.USRP2_FW_UPDATE_ID_RESET_MAH_COMPUTORZ_LOL, seq(), 0, 0, "")
        try: in_pkt = self.send_and_recv(out_pkt)
        except socket.timeout: return

        (proto_ver, pktid, rxseq, flash_addr, rxlength, data) = unpack_flash_args_fmt(in_pkt)
        if pktid == update_id_t.USRP2_FW_UPDATE_ID_RESETTIN_TEH_COMPUTORZ_OMG:
            raise Exception, "Device failed to reset."

    def erase_image(self, addr, length):
        self._status_cb("Erasing")
        #get flash info first
        out_pkt = pack_flash_args_fmt(USRP2_FW_PROTO_VERSION, update_id_t.USRP2_FW_UPDATE_ID_ERASE_TEH_FLASHES_LOL, seq(), addr, length, "")
        in_pkt = self.send_and_recv(out_pkt)

        (proto_ver, pktid, rxseq, flash_addr, rxlength, data) = unpack_flash_args_fmt(in_pkt)

        if pktid != update_id_t.USRP2_FW_UPDATE_ID_ERASING_TEH_FLASHES_OMG:
            raise Exception, "Invalid reply %c from device." % (chr(pktid))

        print "Erasing %i bytes at %i" % (length, addr)
        start_time = time.time()

        #now wait for it to finish
        while(True):
            out_pkt = pack_flash_args_fmt(USRP2_FW_PROTO_VERSION, update_id_t.USRP2_FW_UPDATE_ID_R_U_DONE_ERASING_LOL, seq(), 0, 0, "")
            in_pkt = self.send_and_recv(out_pkt)

            (proto_ver, pktid, rxseq, flash_addr, rxlength, data) = unpack_flash_args_fmt(in_pkt)

            if pktid == update_id_t.USRP2_FW_UPDATE_ID_IM_DONE_ERASING_OMG: break
            elif pktid != update_id_t.USRP2_FW_UPDATE_ID_NOPE_NOT_DONE_ERASING_OMG:
                raise Exception, "Invalid reply %c from device." % (chr(pktid))
            time.sleep(0.01) #decrease network overhead by waiting a bit before polling
            self._progress_cb(min(1.0, (time.time() - start_time)/(length/80e3)))


########################################################################
# command line options
########################################################################
def get_options():
    parser = optparse.OptionParser()
    parser.add_option("--addr", type="string",                 help="USRP-N2XX device address",       default='')
    parser.add_option("--fw",   type="string",                 help="firmware image path (optional)", default='')
    parser.add_option("--fpga", type="string",                 help="fpga image path (optional)",     default='')
    parser.add_option("--reset", action="store_true",          help="reset the device after writing", default=False)
    parser.add_option("--read", action="store_true",           help="read to file instead of write from file", default=False)
    parser.add_option("--overwrite-safe", action="store_true", help="never ever use this option", default=False)
    (options, args) = parser.parse_args()

    return options

########################################################################
# main
########################################################################
if __name__=='__main__':
    options = get_options()
    if not options.addr: raise Exception, 'no address specified'

    if not options.fpga and not options.fw and not options.reset: raise Exception, 'Must specify either a firmware image or FPGA image, and/or reset.'

    if options.overwrite_safe and not options.read:
        print("Are you REALLY, REALLY sure you want to overwrite the safe image? This is ALMOST ALWAYS a terrible idea.")
        print("If your image is faulty, your USRP2+ will become a brick until reprogrammed via JTAG.")
        response = raw_input("""Type "yes" to continue, or anything else to quit: """)
        if response != "yes": sys.exit(0)

    burner = burner_socket(addr=options.addr)

    if options.read:
        if options.fw:
            file = options.fw
            if os.path.isfile(file):
                response = raw_input("File already exists -- overwrite? (y/n) ")
                if response != "y": sys.exit(0)
            size = FW_IMAGE_SIZE_BYTES
            addr = SAFE_FW_IMAGE_LOCATION_ADDR if options.overwrite_safe else PROD_FW_IMAGE_LOCATION_ADDR
            burner.read_image(file, size, addr)

        if options.fpga:
            file = options.fpga
            if os.path.isfile(file):
                response = raw_input("File already exists -- overwrite? (y/n) ")
                if response != "y": sys.exit(0)
            size = FPGA_IMAGE_SIZE_BYTES
            addr = SAFE_FPGA_IMAGE_LOCATION_ADDR if options.overwrite_safe else PROD_FPGA_IMAGE_LOCATION_ADDR
            burner.read_image(file, size, addr)

    else: burner.burn_fw(fw=options.fw, fpga=options.fpga, reset=options.reset, safe=options.overwrite_safe)
