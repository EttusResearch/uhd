#!/usr/bin/env python
#
# Copyright 2010-2011,2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

import optparse
import math
import os
import re
import struct
import socket
import sys
import time
import platform
import subprocess

def print_image_loader_warning(fw, fpga, reset, safe, addr):
    # Newline + indent
    if platform.system() == "Windows":
        nl = " ^\n    "
    else:
        nl = " \\\n    "

    # Generate uhd_image_loader command based on given arguments
    uhd_image_loader = "uhd_image_loader --args=\"type=usrp2,addr={0}".format(addr)
    if reset:
        uhd_image_loader += ",reset"
    if safe:
        uhd_image_loader += ",overwrite-safe"
    uhd_image_loader += "\""

    if fw:
        uhd_image_loader += "{0}--fw-path=\"{1}\"".format(nl, fw)
    else:
        uhd_image_loader += "{0}--no-fw".format(nl)

    if fpga:
        uhd_image_loader += "{0}--fpga-path=\"{1}\"".format(nl, fpga)
    else:
        uhd_image_loader += "{0}--no-fpga".format(nl)

    print("")
    print("************************************************************************************************")
    print("ERROR: This utility has been removed in this version of UHD. Use this command:")
    print("")
    print(uhd_image_loader)
    print("")
    print("************************************************************************************************")
    print("")

########################################################################
# Burner class, holds a socket and send/recv routines
########################################################################
class burner_socket(object):
    def __init__(self, addr):
        self._addr = addr

    def burn_fw(self, fw, fpga, reset, safe, check_rev=True):
        " Just a dummy lol "
        print_image_loader_warning(fw, fpga, reset, safe, self._addr)

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
    parser.add_option("--dont-check-rev", action="store_true", help="disable revision checks", default=False)
    parser.add_option("--list", action="store_true",           help="list possible network devices", default=False)
    (options, args) = parser.parse_args()

    return options

########################################################################
# main
########################################################################
if __name__=='__main__':
    options = get_options()
    burner_socket(options.addr).burn_fw(fw=options.fw, fpga=options.fpga, reset=options.reset, safe=options.overwrite_safe, check_rev=not options.dont_check_rev)
