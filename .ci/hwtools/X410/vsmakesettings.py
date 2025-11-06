#!/usr/bin/env python3
#
# Copyright 2025 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: MIT
#

from hwtools.api import vapi

def do_options():
    vapi.set_project_name(name="X410_FPGA", pretty_name="USRP X410 FPGA")

def do_files():
    # x400 related files
    vapi.find_files_and_add_to_fileset(base="repo", path="fpga/usrp3/top/x400", tree=False)
    vapi.find_files_and_add_to_fileset(base="repo", path="fpga/usrp3/top/x400/cpld/common")
    vapi.find_files_and_add_to_fileset(base="repo", path="fpga/usrp3/top/x400/cpld/x410")
    vapi.find_files_and_add_to_fileset(base="repo", path="fpga/usrp3/top/x400/dboards", tree=False)
    vapi.find_files_and_add_to_fileset(base="repo", path="fpga/usrp3/top/x400/dboards/zbx")
    vapi.find_files_and_add_to_fileset(base="repo", path="fpga/usrp3/top/x400/rf/common")
    vapi.find_files_and_add_to_fileset(base="repo", path="fpga/usrp3/top/x400/rf/100m")
    vapi.find_files_and_add_to_fileset(base="repo", path="fpga/usrp3/top/x400/rf/200m")
    vapi.find_files_and_add_to_fileset(base="repo", path="fpga/usrp3/top/x400/rf/400m")
    vapi.find_files_and_add_to_fileset(base="repo", path="fpga/usrp3/top/x400/ip/x4xx_ps_rfdc_bd/common/regmap")
    vapi.find_files_and_add_to_fileset(base="repo", path="fpga/usrp3/top/x400/ip/x4xx_ps_rfdc_bd/x410_ps_rfdc_bd/regmap")

    # library
    vapi.find_files_and_add_to_fileset(base="repo", path="fpga/usrp3/lib/rfnoc")

    # regmap files
    vapi.find_files_and_add_to_fileset(base="repo", path="fpga/usrp3/top/x400/regmap", tree=False)
    vapi.find_files_and_add_to_fileset(base="repo", path="fpga/usrp3/top/x400/regmap/x410", tree=False)
    vapi.find_files_and_add_to_fileset(base="repo", path="fpga/usrp3/top/x400/cpld/regmap", tree=False)
    vapi.find_files_and_add_to_fileset(base="repo", path="fpga/usrp3/top/x400/cpld/regmap/x410", tree=False)

def do_preparse():
    pass

def do_precompile():
    pass

def do_presynth():
    pass

def do_finish():
    vapi.run_user_function(function="update_xmlparse_files", base="self", path="../common.py")

def vhook_warn_filter(msgstr):
    return False
