#!/usr/bin/env python3
"""Execute rfnoc_modtool and make sure it works."""
#
# Copyright 2025 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

import argparse
import importlib
import os
import pathlib
import subprocess
import sys
import tempfile

# Valid block YAML file for a simple block
SIMPLE_BLOCK_YAML = """
schema: rfnoc_modtool_args
module_name: mytestblock
version: "1.0"
rfnoc_version: "1.0"
chdr_width: 64
noc_id: 0x73570000

parameters:
  NUM_PORTS: 1

clocks:
  - name: rfnoc_chdr
    freq: "[]"
  - name: rfnoc_ctrl
    freq: "[]"
  - name: ce
    freq: "[]"

control:
  fpga_iface: ctrlport
  interface_direction: slave
  fifo_depth: 32
  clk_domain: ce
  ctrlport:
    byte_mode: False
    timed: False
    has_status: False

data:
  fpga_iface: axis_pyld_ctxt
  clk_domain: ce
  inputs:
    in:
      num_ports: NUM_PORTS
      item_width: 32
      nipc: 1
      context_fifo_depth: 2
      payload_fifo_depth: 32
      format: sc16
  outputs:
    out:
      num_ports: NUM_PORTS
      item_width: 16
      nipc: 1
      context_fifo_depth: 2
      payload_fifo_depth: 32
      format: s16
"""


def parse_args():
    """Create arg parser and run it."""
    # get path of current file
    this_path = os.path.abspath(os.path.join(__file__, os.pardir, os.pardir))
    parser = argparse.ArgumentParser()
    parser.add_argument("--base-dir", help="Base directory for running rfnoc-modtool")
    parser.add_argument(
        "--rfnoc-oot-name", help="Name of the OOT directory to create", default="test"
    )
    parser.add_argument("--uhd-repo-dir", help="Path to UHD repository", default=this_path)
    parser.add_argument("-j", help="Number of jobs to run", default=1)
    parser.add_argument("--skip-testbenches", help="Skip running HDL testbenches", action="store_true")
    return parser.parse_args()


def main():
    """Execute."""
    args = parse_args()
    base_dir = args.base_dir
    if not base_dir:
        base_dir = tempfile.mkdtemp()
    base_dir = os.path.abspath(base_dir)
    print(f"Switching to temporary directory: {base_dir}")
    os.chdir(base_dir)
    rfnoc_oot_name = args.rfnoc_oot_name
    rfnoc_oot_dir = f"rfnoc-{rfnoc_oot_name}"
    abs_rfnoc_oot_dir = os.path.abspath(os.path.join(base_dir, rfnoc_oot_dir))
    print(f"Creating OOT directory: {rfnoc_oot_dir}...")
    subprocess.run(["rfnoc_modtool", "create", rfnoc_oot_name], check=True, cwd=base_dir)
    assert pathlib.Path(
        os.path.join(base_dir, rfnoc_oot_dir)
    ).is_dir(), "Failed to create OOT directory!"
    print(f"Switching to OOT directory: {rfnoc_oot_dir}")
    os.chdir(rfnoc_oot_dir)
    simple_block_yaml = os.path.join(abs_rfnoc_oot_dir, "rfnoc", "blocks", "mytestblock.yml")
    print(f"Creating block YAML file: {simple_block_yaml}...")
    with open(simple_block_yaml, "w") as f:
        f.write(SIMPLE_BLOCK_YAML)
    print(f"Running rfnoc_modtool add...")
    subprocess.run(["rfnoc_modtool", "add", "mytestblock"], check=True, cwd=abs_rfnoc_oot_dir)
    print("Creating build directory...")
    build_dir = os.path.join(abs_rfnoc_oot_dir, "build")
    os.mkdir(build_dir)
    os.chdir(build_dir)
    cmake_cmd = ["cmake", "..", "-DUHD_FPGA_DIR=" + os.path.join(args.uhd_repo_dir, "fpga")]
    print("Running cmake command:", " ".join(cmake_cmd))
    subprocess.run(cmake_cmd, check=True, cwd=build_dir)
    make_cmd = ["make", "-j", str(args.j)]
    print("Running make command:", " ".join(make_cmd))
    subprocess.run(make_cmd, check=True, cwd=build_dir)
    print("Checking Python module...")
    importlib.invalidate_caches()
    sys.path.append(os.path.join(build_dir, "python"))
    m = importlib.import_module(f"rfnoc_{rfnoc_oot_name}")
    assert hasattr(
        m.rfnoc_test_python, f"mytestblock_block_control"
    ), "Failed to import Python module!"
    if not args.skip_testbenches:
        print("Running HDL testbenches...")
        testbench_cmd = ["make", "testbenches"]
        subprocess.run(testbench_cmd, check=True)


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)
