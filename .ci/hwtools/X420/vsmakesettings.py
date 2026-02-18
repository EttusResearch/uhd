#!/usr/bin/env python3
#
# Copyright 2025 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: MIT
#

import os
import sys

from hwtools.api import vapi, pathapi, pp

def do_options():
    vapi.set_project_name(name="X420_FPGA", pretty_name="USRP X420 FPGA")
    vapi.set_top_synth_entity("x4xx")
    vapi.set_top_sim_entity("x4xx")
    pathapi.set_workspace_base("repo")

def do_files():
    # x400 related files
    vapi.find_files_and_add_to_fileset(base="repo", path="fpga/usrp3/top/x400", tree=False)
    vapi.find_files_and_add_to_fileset(base="repo", path="fpga/usrp3/top/x400/cpld/common")
    vapi.find_files_and_add_to_fileset(base="repo", path="fpga/usrp3/top/x400/cpld/x410")
    vapi.find_files_and_add_to_fileset(base="repo", path="fpga/usrp3/top/x400/dboards", tree=False)
    vapi.find_files_and_add_to_fileset(base="repo", path="fpga/usrp3/top/x400/dboards/hbx")
    vapi.find_files_and_add_to_fileset(base="repo", path="fpga/usrp3/top/x400/rf/common")
    vapi.find_files_and_add_to_fileset(base="repo", path="fpga/usrp3/top/x400/rf/x420")
    vapi.find_files_and_add_to_fileset(base="repo", path="fpga/usrp3/top/x400/rf/1000m")
    vapi.find_files_and_add_to_fileset(base="repo", path="fpga/usrp3/top/x400/rf/full/rf_core_full.sv")
    vapi.find_files_and_add_to_fileset(base="repo", path="fpga/usrp3/top/x400/ip/x4xx_ps_rfdc_bd/common/regmap")
    vapi.find_files_and_add_to_fileset(base="repo", path="fpga/usrp3/top/x400/ip/x4xx_ps_rfdc_bd/x420_ps_rfdc_bd/regmap")
    # library
    vapi.find_files_and_add_to_fileset(base="repo", path="fpga/usrp3/lib/axi4s_sv")
    vapi.find_files_and_add_to_fileset(base="repo", path="fpga/usrp3/lib/control")
    vapi.find_files_and_add_to_fileset(base="repo", path="fpga/usrp3/lib/dsp")
    vapi.find_files_and_add_to_fileset(base="repo", path="fpga/usrp3/lib/fifo")
    vapi.find_files_and_add_to_fileset(base="repo", path="fpga/usrp3/lib/rfnoc")

    # regmap files
    vapi.find_files_and_add_to_fileset(base="repo", path="fpga/usrp3/top/x400/regmap", tree=False)
    vapi.find_files_and_add_to_fileset(base="repo", path="fpga/usrp3/top/x400/regmap/x420", tree=False)
    vapi.find_files_and_add_to_fileset(base="repo", path="fpga/usrp3/top/x400/cpld/regmap", tree=False)
    vapi.find_files_and_add_to_fileset(base="repo", path="fpga/usrp3/top/x400/cpld/regmap/x410", tree=False)

    # duplicate exclusions
    vapi.remove_from_fileset("build-.*")
    vapi.remove_from_fileset("lattice/impl1")
    vapi.remove_from_fileset("crossbar_tb/")
    vapi.remove_from_fileset("glbl.v")

    # simulation files
    vapi.find_files_and_add_to_fileset(base="repo", path="fpga/usrp3/sim/packages")
    vapi.find_files_and_add_to_fileset(base="repo", path="fpga/usrp3/sim/rfnoc")

    # check for hbx CPLD
    cpld_impl_path = "fpga/usrp3/top/x400/dboards/hbx/cpld/"
    cpld_source_path = pathapi.get_abs_path(base="repo", path=cpld_impl_path, checkexist=False)
    if not os.path.isdir(cpld_source_path):
        pp.printerr(f"HBX implementation is missing. {os.linesep}"
                    f"Please copy the HBX files to your workspace by executing the following command: {os.linesep}"
                    f"git restore --source HBX_SOURCE_BRANCH -- {cpld_source_path}")
        sys.exit(1)

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
