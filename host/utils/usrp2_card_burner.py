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

import platform
import tempfile
import subprocess
try:
    import urllib.request
except ImportError:
    import urllib
    urllib.request = urllib
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
    verbose = p.stdout.read().decode('utf-8')
    if ret != 0: raise Exception(verbose)
    return verbose

def get_dd_path():
    if platform.system() == 'Windows':
        dd_path = os.path.join(os.path.dirname(__file__), 'dd.exe')
        if os.path.exists(dd_path): return dd_path
        dd_path = os.path.join(tempfile.gettempdir(), 'dd.exe')
        if not os.path.exists(dd_path):
            print('Downloading dd.exe to %s'%dd_path)
            dd_bin = urllib.request.urlopen('http://files.ettus.com/dd.exe').read()
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
                if 'link to' not in info: return False
                #handles two spellings of remov(e)able:
                if 'remov' not in info.lower(): return False
                if 'size is' in info and int(extract_info_value(info, 'size is')) > MAX_SD_CARD_SIZE: return False
            except: return False
            return True
        def extract_info_name(info):
            for key in ('Mounted on', 'link to'):
                if key in info: return extract_info_value(info, key)
            return info.splitlines()[0].strip()

        return sorted(set(map(extract_info_name, list(filter(is_info_valid, get_info_list(command(get_dd_path(), '--list')))))))

    ####################################################################
    # Platform Linux: parse procfs /proc/partitions
    ####################################################################
    if platform.system() == 'Linux':
        devs = list()
        for line in command('cat', '/proc/partitions').splitlines():
            try:
                major, minor, blocks, name = line.split()
                if not name[-1].isdigit() and int(minor) == 0: continue
                if int(blocks)*1024 > MAX_SD_CARD_SIZE: continue
            except: continue
            devs.append(os.path.join('/dev', name))

        return sorted(set(devs))

    ####################################################################
    # Platform Mac OS X: parse diskutil list and info commands
    ####################################################################
    if platform.system() == 'Darwin':
        devs = [d.split()[0] for d in [l for l in command('diskutil', 'list').splitlines() if l.startswith('/dev')]]
        def output_to_info(output):
            return dict([list(map(lambda x: x.strip(), pair.lower().split(':'))) for pair in [l for l in output.splitlines() if ':' in l]])
        def is_dev_valid(dev):
            info = output_to_info(command('diskutil', 'info', dev))
            try:
                if 'internal' in info and info['internal'] == 'yes': return False
                if 'ejectable' in info and info['ejectable'] == 'no': return False
                if 'total size' in info:
                    size_match = re.match('^.*\((\d+)\s*bytes\).*$', info['total size'])
                    if size_match and int(size_match.groups()[0]) > MAX_SD_CARD_SIZE: return False
            except: return False
            return True

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
    padding = bytes(b'\x00')*pad_len #zero-padding
    open(tmp_file, 'wb').write(img_data + padding)

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
        raise Exception('Image file larger than %d bytes!'%MAX_FILE_SIZE)
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
    parser.add_option("--force", action="store_true", help="override safety check",         default=False)
    (options, args) = parser.parse_args()

    return options

########################################################################
# main
########################################################################
if __name__=='__main__':
    options = get_options()
    device_hints = get_raw_device_hints()
    show_listing = options.list

    if not show_listing and not options.force and options.dev and options.dev not in device_hints:
        print('The device "%s" was not in the list of possible raw devices.'%options.dev)
        print('The card burner application will now exit without burning your card.')
        print('To override this safety check, specify the --force option.\n')
        show_listing = True

    if show_listing:
        print('Possible raw devices:')
        print('  ' + '\n  '.join(device_hints))
        exit()

    if not options.dev: raise Exception('no raw device path specified')
    print(burn_sd_card(dev=options.dev, fw=options.fw, fpga=options.fpga))
