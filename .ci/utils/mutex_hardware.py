# mutex_hardware uses redis get a lock on hardware
# to prevent other Azure Pipeline agents from use.
# It also provides helper functions to get devices
# into a state where it can be used for testing.

import argparse
import labgrid
import os
import pathlib
import shlex
import subprocess
import sys
import time

from fabric import Connection
from pottery import Redlock
from redis import Redis

bitfile_name = "usrp_{}_fpga_{}.bit"

def jtag_x3xx(dev_type, dev_model, jtag_server, jtag_serial, fpga_folder, fpga, redis_server):
    remote_working_dir = "pipeline_fpga"
    vivado_program_jtag = "/opt/Xilinx/Vivado_Lab/2020.1/bin/vivado_lab -mode batch -source {}/viv_hardware_utils.tcl -nolog -nojournal -tclargs program".format(
        remote_working_dir)
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
            fpga_path = os.path.join(fpga_folder, bitfile_name.format(dev_model, fpga))
            jtag_host.put(fpga_path, remote=remote_working_dir)
            jtag_host.run(vivado_program_jtag + " " +
                          os.path.join(remote_working_dir, os.path.basename(fpga_path)) +
                          " " + jtag_serial)
        print("Waiting 15 seconds for device to come back up and for Vivado to close", flush=True)
        time.sleep(15)

def set_sfp_addrs(mgmt_addr, sfp_addrs):
    with Connection(host=mgmt_addr,user='root',connect_kwargs={"password":"", "timeout":120, "banner_timeout":120, "auth_timeout":120}) as dut:
        for idx, sfp_addr in enumerate(sfp_addrs):
            dut.run(f"ip link set sfp{idx} down")
            dut.run(f"ip addr add {sfp_addr}/24 dev sfp{idx}")
            dut.run(f"ip link set sfp{idx} up")
    time.sleep(30)

def flash_sdimage(dev_model, sdimage_path, labgrid_device_yaml, mgmt_addr, sfp_addrs):
    subprocess.run(shlex.split(f"labgrid-client -c {labgrid_device_yaml} release --kick"))
    subprocess.run(shlex.split(f"labgrid-client -c {labgrid_device_yaml} acquire"))
    env = labgrid.Environment(labgrid_device_yaml)
    target = env.get_target()

    print("Powering down DUT", flush=True)
    cp = target.get_driver(labgrid.protocol.ConsoleProtocol, name="scu_serial_driver")
    cp.write("\napshutdown\n".encode())

    print("Switching SDMux to Host", flush=True)
    sdmux = target.get_driver(labgrid.driver.USBSDMuxDriver)
    sdmux.set_mode('host')

    print(f"Writing SDMux using {sdimage_path}", flush=True)
    massstore = target.get_driver(labgrid.driver.USBStorageDriver)
    # This uses bmaptool in --nobmap mode to write the sdimg,
    # since it automatically decompresses the image. Do not
    # include the .bmap file since it can cause corruption with mender.
    # See: https://github.com/mendersoftware/meta-mender/pull/1076
    massstore.write_image(sdimage_path, mode=labgrid.driver.usbstoragedriver.Mode.BMAPTOOL)
    time.sleep(30)

    print("Switching SDMux to DUT", flush=True)
    sdmux.set_mode('dut')
    time.sleep(30)

    print("Powering on DUT", flush=True)
    cp.write("\npowerbtn\n".encode())

    print("Waiting 2 minutes for device to boot", flush=True)
    time.sleep(120)
    known_hosts_path = os.path.expanduser("~/.ssh/known_hosts")
    subprocess.run(shlex.split(f"ssh-keygen -f \"{known_hosts_path}\" -R \"{mgmt_addr}\""))

    if sfp_addrs:
        set_sfp_addrs(mgmt_addr, sfp_addrs)

    subprocess.run(shlex.split(f"labgrid-client -c {labgrid_device_yaml} release"))

def main(args):
    redis_server = {Redis.from_url(
        "redis://{}:6379/0".format(args.redis_server))}
    print("Waiting to acquire mutex for {}".format(args.dut_name), flush=True)
    with Redlock(key=args.dut_name, masters=redis_server, auto_release_time=1000 * 60 * args.dut_timeout):
        print("Got mutex for {}".format(args.dut_name), flush=True)

        if args.sdimage:
            dev_type, dev_model, sdimage_path, labgrid_device_yaml, mgmt_addr = args.sdimage.split(',')
            if args.sfp_addrs:
                sfp_addrs = args.sfp_addrs.split(',')
            else:
                sfp_addrs = None
            flash_sdimage(dev_model, sdimage_path, labgrid_device_yaml, mgmt_addr, sfp_addrs)

        if args.fpgas:
            working_dir = os.getcwd()
            return_code = 0
            for fpga in args.fpgas.split(','):
                os.mkdir(os.path.join(working_dir, fpga))
                os.chdir(os.path.join(working_dir, fpga))

                if args.jtag_x3xx:
                    dev_type, dev_model, jtag_server, jtag_serial, fpga_folder = args.jtag_x3xx.split(',')
                    jtag_x3xx(dev_type, dev_model, jtag_server, jtag_serial, fpga_folder, fpga, redis_server)

                if dev_type and dev_type in ["n3xx", "e3xx"]:
                    subprocess.run(shlex.split(f"uhd_image_loader --args=mgmt_addr={mgmt_addr},type={dev_type},fpga={fpga}"))
                    if sfp_addrs:
                        set_sfp_addrs(mgmt_addr, sfp_addrs)

                for command in args.test_commands:
                    result = subprocess.run(shlex.split(command))
                    if(return_code == 0):
                        return_code = result.returncode
            sys.exit(return_code)
        else:
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
                        help="dev_type,dev_model,user@jtag_server,jtag_serial,fpga_folder")
    parser.add_argument("--sdimage", type=str,
                        help="dev_type,dev_model,sdimg_path,labgrid_device_yaml,mgmt_addr")
    parser.add_argument("--sfp_addrs", type=str,
                        help="sfp0ip,sfp1ip,...")
    parser.add_argument("--fpgas", type=str,
                        help="Comma delimited list of FPGAs to test")
    parser.add_argument("--dut_timeout", type=int, default=60,
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
