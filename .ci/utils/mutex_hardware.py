# mutex_hardware uses redis get a lock on hardware
# to prevent other Azure Pipeline agents from use.
# It also provides helper functions to get devices
# into a state where it can be used for testing.

import argparse
import os
import pathlib
import shlex
import subprocess
import sys
import time

from fabric import Connection
from pottery import Redlock
from redis import Redis


def jtag_x3xx(jtag_args, redis_server):
    remote_working_dir = "pipeline_fpga"
    vivado_program_jtag = "/opt/Xilinx/Vivado_Lab/2020.1/bin/vivado_lab -mode batch -source {}/viv_hardware_utils.tcl -nolog -nojournal -tclargs program".format(
        remote_working_dir)
    jtag_server, jtag_serial, fpga_path = jtag_args.split(",")
    print("Waiting on jtag mutex for {}".format(jtag_server), flush=True)
    with Redlock(key="hw_jtag_{}".format(jtag_server),
                 masters=redis_server, auto_release_time=1000 * 60 * 5):
        print("Got jtag mutex for {}".format(jtag_server), flush=True)
        with Connection(host=jtag_server) as jtag_host:
            jtag_host.run("mkdir -p " + remote_working_dir)
            jtag_host.run("rm -rf {}/*".format(remote_working_dir))
            jtag_host.put(
                os.path.join(pathlib.Path(
                    __file__).parent.absolute(), "jtag/viv_hardware_utils.tcl"),
                remote=remote_working_dir)
            jtag_host.put(fpga_path, remote=remote_working_dir)
            jtag_host.run(vivado_program_jtag + " " +
                          os.path.join(remote_working_dir, os.path.basename(fpga_path)) +
                          " " + jtag_serial)
        print("Waiting 15 seconds for device to come back up and for Vivado to close", flush=True)
        time.sleep(15)


def main(args):
    redis_server = {Redis.from_url(
        "redis://{}:6379/0".format(args.redis_server))}
    print("Waiting to acquire mutex for {}".format(args.dut_name), flush=True)
    with Redlock(key=args.dut_name, masters=redis_server, auto_release_time=1000 * 60 * args.dut_timeout):
        print("Got mutex for {}".format(args.dut_name), flush=True)
        if(args.jtag_x3xx != None):
            jtag_x3xx(args.jtag_x3xx, redis_server)
        for command in args.test_commands:
            result = subprocess.run(shlex.split(command))
            if(result.returncode != 0):
                sys.exit(result.returncode)
        sys.exit(0)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    # jtag_x3xx will flash the fpga for a given jtag_serial using
    # Vivado on jtag_server. It uses SSH to control jtag_server.
    # Provide fpga_path as a local path and it will be copied
    # to jtag_server.
    parser.add_argument("--jtag_x3xx", type=str,
                        help="user@jtag_server,jtag_serial,fpga_path")
    parser.add_argument("--dut_timeout", type=int, default=30,
                        help="Dut mutex timeout in minutes")
    parser.add_argument("redis_server", type=str,
                        help="Redis server for mutex")
    parser.add_argument("dut_name", type=str,
                        help="Unique identifier for device under test")
    # test_commands allows for any number of shell commands
    # to execute. Call into mutex_hardware with an unlimited
    # number of commands in string format as the last positional arguments.
    parser.add_argument("test_commands", type=str,
                        nargs="+", help="Commands to run")
    args = parser.parse_args()
    main(args)
