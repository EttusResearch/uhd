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

import platform
import tempfile
import subprocess
import urllib
import optparse
import math
import os
import re

########################################################################
# constants
########################################################################
SECTOR_SIZE = 512                 # bytes
MAX_FILE_SIZE =  1 * (2**20)      # maximum number of bytes we'll burn to a slot

FPGA_OFFSET = 0                   # offset in flash to fpga image
FIRMWARE_OFFSET = 1 * (2**20)     # offset in flash to firmware image

MAX_SD_CARD_SIZE = 2048e6         # bytes (any bigger is sdhc)

########################################################################
# helper functions
########################################################################
def command(*args):
    p = subprocess.Popen(
        args,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    ret = p.wait()
    verbose = p.stdout.read()
    if ret != 0: raise Exception, verbose
    return verbose

def get_dd_path():
    if platform.system() == 'Windows':
        dd_path = os.path.join(tempfile.gettempdir(), 'dd.exe')
        if not os.path.exists(dd_path):
            print 'Downloading dd.exe to %s'%dd_path
            dd_bin = urllib.urlopen('http://www.ettus.com/downloads/dd.exe').read()
            open(dd_path, 'wb').write(dd_bin)
        return dd_path
    return 'dd'

def int_ceil_div(num, den):
    return int(math.ceil(float(num)/float(den)))

def get_tmp_file():
    tmp = tempfile.mkstemp()
    os.close(tmp[0])
    return tmp[1]

########################################################################
# list possible devices
########################################################################
def get_raw_device_hints():
    ####################################################################
    # Platform Windows: parse the output of dd.exe --list
    ####################################################################
    if platform.system() == 'Windows':
        def extract_info_value(info, key):
            return info.split(key)[-1].split()[0]
        def get_info_list(output):
            in_info = False
            for line in output.splitlines():
                if line.startswith('\\\\'): in_info = True; info = ''
                elif in_info and not line.strip(): in_info = False; yield info
                if in_info: info += '\n'+line.strip()
        def is_info_valid(info):
            try:
                assert 'link to' in info
                #handles two spellings of remov(e)able:
                assert 'remov' in info.lower()
                if 'size is' in info: assert int(extract_info_value(info, 'size is')) <= MAX_SD_CARD_SIZE
                return True
            except: return False
        def extract_info_name(info):
            for key in ('Mounted on', 'link to'):
                if key in info: return extract_info_value(info, key)
            return info.splitlines()[0].strip()

        return sorted(set(map(extract_info_name, filter(is_info_valid, get_info_list(command(get_dd_path(), '--list'))))))

    ####################################################################
    # Platform Linux: parse procfs /proc/partitions
    ####################################################################
    if platform.system() == 'Linux':
        devs = list()
        try: output = open('/proc/partitions', 'r').read()
        except: return devs
        for line in output.splitlines():
            try:
                major, minor, blocks, name = line.split()
                assert not name[-1].isdigit() or int(minor) == 0
                assert int(blocks)*1024 <= MAX_SD_CARD_SIZE
            except: continue
            devs.append(os.path.join('/dev', name))

        return sorted(set(devs))

    ####################################################################
    # Platform Mac OS X: parse diskutil list and info commands
    ####################################################################
    if platform.system() == 'Darwin':
        devs = map(lambda d: d.split()[0], filter(lambda l: l.startswith('/dev'), command('diskutil', 'list').splitlines()))
        def output_to_info(output):
            return dict([map(str.strip, pair.lower().split(':')) for pair in filter(lambda l: ':' in l, output.splitlines())])
        def is_dev_valid(dev):
            info = output_to_info(command('diskutil', 'info', dev))
            try:
                if info.has_key('internal'): assert info['internal'] == 'no'
                if info.has_key('ejectable'): assert info['ejectable'] == 'yes'
                if info.has_key('total size'):
                    size_match = re.match('^.*\((\d+)\s*bytes\).*$', info['total size'])
                    if size_match: assert int(size_match.groups()[0]) <= MAX_SD_CARD_SIZE
                return True
            except: return False

        return sorted(set(filter(is_dev_valid, devs)))

    ####################################################################
    # Platform Others:
    ####################################################################
    return ()

########################################################################
# write and verify with dd
########################################################################
def verify_image(image_file, device_file, offset):
    #create a temporary file to store the readback image
    tmp_file = get_tmp_file()

    #read the image data
    img_data = open(image_file, 'rb').read()
    count = int_ceil_div(len(img_data), SECTOR_SIZE)

    #execute a dd subprocess
    verbose = command(
        get_dd_path(),
        "of=%s"%tmp_file,
        "if=%s"%device_file,
        "skip=%d"%(offset/SECTOR_SIZE),
        "bs=%d"%SECTOR_SIZE,
        "count=%d"%count,
    )

    #verfy the data
    tmp_data = open(tmp_file, 'rb').read(len(img_data))
    if img_data != tmp_data: return 'Verification Failed:\n%s'%verbose
    return 'Verification Passed:\n%s'%verbose

def write_image(image_file, device_file, offset):
    #create a temporary file to store the padded image
    tmp_file = get_tmp_file()

    #write the padded image data
    img_data = open(image_file, 'rb').read()
    count = int_ceil_div(len(img_data), SECTOR_SIZE)
    pad_len = SECTOR_SIZE*count - len(img_data)
    pad_str = ''.join([chr(0)]*pad_len) #zero-padding
    open(tmp_file, 'wb').write(img_data + pad_str)

    #execute a dd subprocess
    verbose = command(
        get_dd_path(),
        "if=%s"%tmp_file,
        "of=%s"%device_file,
        "seek=%d"%(offset/SECTOR_SIZE),
        "bs=%d"%SECTOR_SIZE,
        "count=%d"%count,
    )

    try: #exec the sync command (only works on linux)
        if platform.system() == 'Linux': command('sync')
    except: pass

    return verbose

def write_and_verify(image_file, device_file, offset):
    if os.path.getsize(image_file) > MAX_FILE_SIZE:
        raise Exception, 'Image file larger than %d bytes!'%MAX_FILE_SIZE
    return '%s\n%s'%(
        write_image(
            image_file=image_file,
            device_file=device_file,
            offset=offset,
        ), verify_image(
            image_file=image_file,
            device_file=device_file,
            offset=offset,
        ),
    )

def burn_sd_card(dev, fw, fpga):
    verbose = ''
    if fw: verbose += 'Burn firmware image:\n%s\n'%write_and_verify(
        image_file=fw, device_file=dev, offset=FIRMWARE_OFFSET
    )
    if fpga: verbose += 'Burn fpga image:\n%s\n'%write_and_verify(
        image_file=fpga, device_file=dev, offset=FPGA_OFFSET
    )
    return verbose

########################################################################
# command line options
########################################################################
def get_options():
    parser = optparse.OptionParser()
    parser.add_option("--dev",  type="string",       help="raw device path",                default='')
    parser.add_option("--fw",   type="string",       help="firmware image path (optional)", default='')
    parser.add_option("--fpga", type="string",       help="fpga image path (optional)",     default='')
    parser.add_option("--list", action="store_true", help="list possible raw devices",      default=False)
    (options, args) = parser.parse_args()

    if options.list:
        print 'Possible raw devices:'
        print '  ' + '\n  '.join(get_raw_device_hints())
        exit()

    return options

########################################################################
# main
########################################################################
if __name__=='__main__':
    options = get_options()
    if not options.dev: raise Exception, 'no raw device path specified'
    print burn_sd_card(dev=options.dev, fw=options.fw, fpga=options.fpga)
