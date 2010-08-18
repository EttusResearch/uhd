#!/usr/bin/env python
#
# Copyright 2010 Ettus Research LLC
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

# TODO: make it work
# TODO: make it autodetect UHD devices
# TODO: you should probably watch sequence numbers

import optparse
import math
import os
import re
import struct
import socket
import sys

########################################################################
# constants
########################################################################
UDP_FW_UPDATE_PORT = 49154
UDP_MAX_XFER_BYTES = 1024
UDP_TIMEOUT = 3
UDP_POLL_INTERVAL = 0.10 #in seconds

USRP2_FW_PROTO_VERSION = 5

#from bootloader_utils.h
PROD_FPGA_IMAGE_LOCATION_ADDR = 0x00200000
PROD_FW_IMAGE_LOCATION_ADDR   = 0x00400000
SAFE_FW_IMAGE_LOCATION_ADDR   = 0x007F0000
SAFE_FPGA_IMAGE_LOCATION_ADDR = 0x00000000
FPGA_IMAGE_SIZE_BYTES         = 2097152
FW_IMAGE_SIZE_BYTES      = 31744

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

def send_and_recv(pkt, ip):
  update_socket = create_socket()

  try:
    update_socket.sendto(pkt, (ip, UDP_FW_UPDATE_PORT))
  except Exception, e: 
    print e
    sys.exit(1)

  try:
    (recv_pkt, recv_addr) = update_socket.recvfrom(UDP_MAX_XFER_BYTES)
  except Exception, e: 
    print e
    sys.exit(1)

  if recv_addr != (options.ip, UDP_FW_UPDATE_PORT):
    raise Exception, "Packet received from invalid IP %s, expected %s" % (recv_addr, options.ip)

  return recv_pkt

def create_socket():
  socket.setdefaulttimeout(UDP_TIMEOUT)
  update_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
  return update_socket

#just here to validate comms
def init_update(ip):
  out_pkt = pack_flash_args_fmt(USRP2_FW_PROTO_VERSION, update_id_t.USRP2_FW_UPDATE_ID_OHAI_LOL, seq(), 0, 0, "")
  in_pkt = send_and_recv(out_pkt, ip)
  (proto_ver, pktid, rxseq, ip_addr) = unpack_flash_ip_fmt(in_pkt)
  if pktid == update_id_t.USRP2_FW_UPDATE_ID_OHAI_OMG:
    print "USRP2P found."
  else:
    raise Exception, "Invalid reply received from device."

#  print "Incoming:\n\tVer: %i\n\tID: %c\n\tSeq: %i\n\tIP: %i\n" % (proto_ver, chr(pktid), rxseq, ip_addr)

def get_flash_info(ip):
  out_pkt = pack_flash_args_fmt(USRP2_FW_PROTO_VERSION, update_id_t.USRP2_FW_UPDATE_ID_WATS_TEH_FLASH_INFO_LOL, seq(), 0, 0, "")
  in_pkt = send_and_recv(out_pkt, ip)

  (proto_ver, pktid, rxseq, sector_size_bytes, memory_size_bytes) = unpack_flash_info_fmt(in_pkt)

  if pktid != update_id_t.USRP2_FW_UPDATE_ID_HERES_TEH_FLASH_INFO_OMG:
    raise Exception, "Invalid reply %c from device." % (chr(pktid))


  return (memory_size_bytes, sector_size_bytes)

def burn_fw(ip, fw, fpga):
  init_update(ip)
  (flash_size, sector_size) = get_flash_info(ip)

  print "Flash size: %i\nSector size: %i" % (flash_size, sector_size)

  if fpga: 
    fpga_file = open(fpga, 'rb')
    fpga_image = fpga_file.read()
    erase_image(ip, SAFE_FPGA_IMAGE_LOCATION_ADDR, FPGA_IMAGE_SIZE_BYTES)
    write_image(ip, fpga_image, SAFE_FPGA_IMAGE_LOCATION_ADDR) #TODO: DO NOT WRITE SAFE IMAGE! this is only here because the bootloader isn't finished yet
    verify_image(ip, fpga_image, SAFE_FPGA_IMAGE_LOCATION_ADDR)

  if fw:
    fw_file = open(fw, 'rb')
    fw_image = fw_file.read()
    erase_image(ip, PROD_FW_IMAGE_LOCATION_ADDR, FW_IMAGE_SIZE_BYTES)
    write_image(ip, fw_image, PROD_FW_IMAGE_LOCATION_ADDR)
    verify_image(ip, fw_image, PROD_FW_IMAGE_LOCATION_ADDR)

def write_image(ip, image, addr):
#we split the image into smaller (256B) bits and send them down the wire
  while image:
    out_pkt = pack_flash_args_fmt(USRP2_FW_PROTO_VERSION, update_id_t.USRP2_FW_UPDATE_ID_WRITE_TEH_FLASHES_LOL, seq(), addr, FLASH_DATA_PACKET_SIZE, image[:FLASH_DATA_PACKET_SIZE])
    in_pkt = send_and_recv(out_pkt, ip)

    (proto_ver, pktid, rxseq, flash_addr, rxlength, data) = unpack_flash_args_fmt(in_pkt)

    if pktid != update_id_t.USRP2_FW_UPDATE_ID_WROTE_TEH_FLASHES_OMG:
      raise Exception, "Invalid reply %c from device." % (chr(pktid))

    image = image[FLASH_DATA_PACKET_SIZE:]
    addr += FLASH_DATA_PACKET_SIZE

def verify_image(ip, image, addr):
  readsize = len(image)
  readdata = str()
  while readsize > 0:
    if readsize < FLASH_DATA_PACKET_SIZE: thisreadsize = readsize
    else: thisreadsize = FLASH_DATA_PACKET_SIZE
    out_pkt = pack_flash_args_fmt(USRP2_FW_PROTO_VERSION, update_id_t.USRP2_FW_UPDATE_ID_READ_TEH_FLASHES_LOL, seq(), addr, thisreadsize, "")
    in_pkt = send_and_recv(out_pkt, ip)
    
    (proto_ver, pktid, rxseq, flash_addr, rxlength, data) = unpack_flash_args_fmt(in_pkt)

    if pktid != update_id_t.USRP2_FW_UPDATE_ID_KK_READ_TEH_FLASHES_OMG:
      raise Exception, "Invalid reply %c from device." % (chr(pktid))

    readdata += data[:thisreadsize]
    readsize -= FLASH_DATA_PACKET_SIZE
    addr += FLASH_DATA_PACKET_SIZE

  print "Read back %i bytes" % len(readdata)
  print readdata

#  for i in range(256, 512):
#    print "out: %i in: %i" % (ord(image[i]), ord(readdata[i]))

  if readdata != image:
    print "Verify failed. Image did not write correctly."
  else:
    print "Success."

def erase_image(ip, addr, length):
  #get flash info first
  out_pkt = pack_flash_args_fmt(USRP2_FW_PROTO_VERSION, update_id_t.USRP2_FW_UPDATE_ID_ERASE_TEH_FLASHES_LOL, seq(), addr, length, "")
  in_pkt = send_and_recv(out_pkt, ip)

  (proto_ver, pktid, rxseq, flash_addr, rxlength, data) = unpack_flash_args_fmt(in_pkt)

  if pktid != update_id_t.USRP2_FW_UPDATE_ID_ERASING_TEH_FLASHES_OMG:
    raise Exception, "Invalid reply %c from device." % (chr(pktid))

  print "Erasing %i bytes at %i" % (length, addr)

  #now wait for it to finish
  while(1):
    out_pkt = pack_flash_args_fmt(USRP2_FW_PROTO_VERSION, update_id_t.USRP2_FW_UPDATE_ID_R_U_DONE_ERASING_LOL, seq(), 0, 0, "")
    in_pkt = send_and_recv(out_pkt, ip)

    (proto_ver, pktid, rxseq, flash_addr, rxlength, data) = unpack_flash_args_fmt(in_pkt)

    if pktid == update_id_t.USRP2_FW_UPDATE_ID_IM_DONE_ERASING_OMG: break
    elif pktid != update_id_t.USRP2_FW_UPDATE_ID_NOPE_NOT_DONE_ERASING_OMG:
      raise Exception, "Invalid reply %c from device." % (chr(pktid))

  print "\tFinished."

#def verify_image(ip, image, addr):


########################################################################
# command line options
########################################################################
def get_options():
    parser = optparse.OptionParser()
    parser.add_option("--ip",   type="string",       help="USRP2P firmware address",        default='')
    parser.add_option("--fw",   type="string",       help="firmware image path (optional)", default='')
    parser.add_option("--fpga", type="string",       help="fpga image path (optional)",     default='')
    (options, args) = parser.parse_args()

    return options

########################################################################
# main
########################################################################
if __name__=='__main__':
    options = get_options()
    if not options.ip: raise Exception, 'no ip address specified'

    if not options.fpga and not options.fw: raise Exception, 'Must specify either a firmware image or FPGA image.'
    burn_fw(ip=options.ip, fw=options.fw, fpga=options.fpga)
