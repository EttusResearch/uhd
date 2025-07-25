"""mutex_hardware contains helper methods to acquire a lock on a device.

mutex_hardware uses redis to get a lock on hardware to prevent other Azure
Pipeline agents from use. It also provides helper functions to get devices
into a state where it can be used for testing.
"""

import argparse
import os
import pathlib
import shlex
import subprocess
import sys
import time

import labgrid
from fabric import Connection
from httpd import HTTPServer
from pottery import Redlock
from redis import Redis
from tftp import TFTPServer

bitfile_name = "usrp_{}_fpga_{}.bit"


class AcquiredPlace:
    """This class provides the default target from a given yaml configuration file.
    A context manager is used to automatically acquire and release the associated
    place. Instantiate the class using the 'with' statement as follows:

    with AcquiredPlace(config) as target:
        (...)
    """

    def __init__(self, config, force_release=False):
        self.config = config
        self.force_release = force_release
        self.env = labgrid.environment.Environment(self.config)

    def __enter__(self):
        """Acquire the place and return the target"""
        proc = subprocess.run(["labgrid-client", "-c", self.config, "acquire"])
        if (proc.returncode != 0) and self.force_release:
            proc = subprocess.run(["labgrid-client", "-c", self.config, "release", "--kick"])
            proc = subprocess.run(["labgrid-client", "-c", self.config, "acquire"])
        if proc.returncode != 0:
            sys.exit(proc.returncode)
        return self.env.get_target()

    def __exit__(self, *args):
        """Release the place"""
        subprocess.run(["labgrid-client", "-c", self.config, "release"])


def jtag_x3xx(
    dev_type, dev_model, jtag_server, jtag_serial, fpga_folder, fpga, redis_server, vivado_dir
):
    """Download the bitfile to the FPGA using jtag interface."""
    if dev_model not in ["x300", "x310"]:
        raise RuntimeError(f"{dev_type} not supported with jtag_x3xx")
    remote_working_dir = "pipeline_fpga"
    vivado_lab_path = os.path.join(vivado_dir, "bin/vivado_lab")
    vivado_program_jtag = "{} -mode batch -source {}/viv_hardware_utils.tcl -nolog -nojournal -tclargs program".format(
        vivado_lab_path, remote_working_dir
    )
    print("Waiting on jtag mutex for {}".format(jtag_server), flush=True)
    with Redlock(
        key="hw_jtag_{}".format(jtag_server), masters=redis_server, auto_release_time=1000 * 60 * 5
    ):
        print("Got jtag mutex for {}".format(jtag_server), flush=True)
        with Connection(host=jtag_server) as jtag_host:
            jtag_host.run("mkdir -p " + remote_working_dir)
            jtag_host.run("rm -rf {}/*".format(remote_working_dir))
            jtag_host.put(
                os.path.join(
                    pathlib.Path(__file__).parent.absolute(), "jtag/viv_hardware_utils.tcl"
                ),
                remote=remote_working_dir,
            )
            fpga_path = os.path.join(fpga_folder, bitfile_name.format(dev_model, fpga))
            jtag_host.put(fpga_path, remote=remote_working_dir)
            jtag_host.run(
                vivado_program_jtag
                + " "
                + os.path.join(remote_working_dir, os.path.basename(fpga_path))
                + " "
                + jtag_serial
            )
        print("Waiting 15 seconds for device to come back up and for Vivado to close", flush=True)
        time.sleep(15)


def set_sfp_addrs(mgmt_addr, sfp_addrs):
    """Configure IP addresses of the SFP adapters."""
    with Connection(
        host=mgmt_addr,
        user="root",
        connect_kwargs={"password": "", "timeout": 120, "banner_timeout": 120, "auth_timeout": 120},
    ) as dut:
        for idx, sfp_addr in enumerate(sfp_addrs):
            dut.run(f"ip link set sfp{idx} down")
            dut.run(f"ip addr flush dev sfp{idx}")
            dut.run(f"ip addr add {sfp_addr}/24 dev sfp{idx}")
            dut.run(f"ip link set sfp{idx} up")
    time.sleep(30)


def flash_sdimage_sdmux(
    dev_model, sdimage_path, labgrid_device_yaml, mgmt_addr, sfp_addrs, force_release=False
):
    """Flash image using sdmux.

    This method uses an sdmux (https://linux-automation.com/en/products/usb-sd-mux.html)
    to reimage the sd card.
    """
    if dev_model not in ["n300", "n310", "n320", "n321"]:
        raise RuntimeError(f"{dev_model} not supported with sdimage_sdmux")

    with AcquiredPlace(labgrid_device_yaml, force_release) as target:

        cp_scu = target.get_driver(labgrid.protocol.ConsoleProtocol, name="scu_serial_driver")
        cp_linux = target.get_driver(labgrid.protocol.ConsoleProtocol, name="linux_serial_driver")

        print("Powering down DUT", flush=True)
        cp_scu.write("\napshutdown\n".encode())

        print("Switching SDMux to Host", flush=True)
        sdmux = target.get_driver(labgrid.driver.USBSDMuxDriver)
        sdmux.set_mode("host")

        print(f"Writing SDMux using {sdimage_path}", flush=True)
        massstore = target.get_driver(labgrid.driver.USBStorageDriver)
        # This uses bmaptool in --nobmap mode to write the sdimg,
        # since it automatically decompresses the image. Do not
        # include the .bmap file since it can cause corruption with mender.
        # See: https://github.com/mendersoftware/meta-mender/pull/1076
        massstore.write_image(sdimage_path, mode=labgrid.driver.usbstoragedriver.Mode.BMAPTOOL)
        time.sleep(30)

        print("Switching SDMux to DUT", flush=True)
        sdmux.set_mode("dut")
        time.sleep(30)

        print("Powering on DUT", flush=True)
        cp_scu.write("\npowerbtn\n".encode())

        try:
            cp_linux.expect("Enter 'noautoboot' to enter prompt without timeout", timeout=5)
        except Exception:
            # Sometimes it requires multiple powerbtn calls to turn on device
            print("Device didn't power on with first attempt. Trying again...")
            cp_scu.write("\npowerbtn\n".encode())
            cp_linux.expect("Enter 'noautoboot' to enter prompt without timeout", timeout=5)

        print("Waiting 2 minutes for device to boot", flush=True)
        time.sleep(120)
        cp_linux.write("\n".encode())
        cp_linux.expect("login:", timeout=5)
        known_hosts_path = os.path.expanduser("~/.ssh/known_hosts")
        subprocess.run(shlex.split(f'ssh-keygen -f "{known_hosts_path}" -R "{mgmt_addr}"'))

        if sfp_addrs:
            set_sfp_addrs(mgmt_addr, sfp_addrs)


def flash_sdimage_tftp(
    dev_model, sdimage_path, initramfs_path, labgrid_device_yaml, sfp_addrs, force_release=False
):
    """Flash image using tftp.

    This method uses tftp to boot the device into a small Linux envionment to
    write to the device's sd card. This method is used on the E320 since it has
    a hardware incompatibility with sdmuxes.
    """
    if dev_model not in ["e320"]:
        raise RuntimeError(f"{dev_model} not supported with sdimage_tftp")

    if dev_model == "e320":
        dev_ram_address = "0x20000000"
        dev_bootm_config = "conf@zynq-ni-${mboard}.dtb"

    with AcquiredPlace(labgrid_device_yaml, force_release) as target:

        cp_scu = target.get_driver(labgrid.protocol.ConsoleProtocol, name="scu_serial_driver")
        cp_linux = target.get_driver(labgrid.protocol.ConsoleProtocol, name="linux_serial_driver")

        print("Powering down DUT", flush=True)
        cp_scu.write("\napshutdown\n".encode())
        time.sleep(10)

        print("Powering on DUT", flush=True)
        cp_scu.write("\npowerbtn\n".encode())
        # Sometimes it requires multiple powerbtn calls to turn on device
        try:
            cp_linux.expect("Enter 'noautoboot' to enter prompt without timeout", timeout=5)
        except Exception:
            print("Device didn't power on with first attempt. Trying again...", flush=True)
            cp_scu.write("\npowerbtn\n".encode())
            cp_linux.expect("Enter 'noautoboot' to enter prompt without timeout", timeout=5)

        print("Attempting to get into uboot console", flush=True)
        cp_linux.write("noautoboot".encode())
        # Handle if the watchdog triggers
        try:
            cp_linux.expect("Enter 'noautoboot' to enter prompt without timeout", timeout=30)
            cp_linux.write("noautoboot".encode())
        except Exception:
            pass
        cp_linux.expect("uboot>")
        print("Waiting for NIC to come up", flush=True)
        time.sleep(10)
        cp_linux.write(f"setenv autoload no; dhcp;\n".encode())
        cp_linux.expect("DHCP client bound to address")
        expect_index, expect_before, expect_match, expect_after = cp_linux.expect(
            r"\b\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}\b"
        )
        mgmt_addr = expect_match[0].decode()
        print(f"Dev got IP Address {mgmt_addr}")

        with TFTPServer(initramfs_path, mgmt_addr) as server:
            time.sleep(10)
            cp_linux.expect("uboot>")
            cp_linux.write(f"setenv tftpdstp {server.port}\n".encode())
            cp_linux.expect("uboot>")
            print("TFTPing initramfs image", flush=True)
            cp_linux.write(
                f"tftpboot {dev_ram_address} {server.ip}:{os.path.basename(initramfs_path)}\n".encode()
            )
            cp_linux.expect("uboot>", timeout=120)
            print("Booting into initramfs", flush=True)
            cp_linux.write(f"bootm {dev_ram_address}#{dev_bootm_config}\n".encode())
            cp_linux.expect("mender login:", timeout=120)
            print("Logging into Linux", flush=True)
            cp_linux.write("root\n".encode())
            cp_linux.expect("mender:~#")
            print("Waiting for NIC to DHCP", flush=True)
            time.sleep(10)

        with HTTPServer(os.path.dirname(sdimage_path), mgmt_addr) as server:
            print(f"Writing SD Card using {sdimage_path}", flush=True)
            print("Running bmaptool... This will take awhile", flush=True)
            cp_linux.write(
                f"bmaptool copy --nobmap {server.get_url(os.path.basename(sdimage_path))} /dev/mmcblk0\n".encode()
            )
            cp_linux.expect("mender:~#", timeout=1800)
            cp_linux.write("echo bmaptool exit code: $?\n".encode())
            cp_linux.expect("bmaptool exit code: 0", timeout=10)
            time.sleep(10)
            print("Rebooting into new image from sd card", flush=True)
            cp_linux.write("reboot\n".encode())

        print("Waiting 2 minutes for device to boot", flush=True)
        time.sleep(120)
        cp_linux.expect("login:", timeout=30)
        known_hosts_path = os.path.expanduser("~/.ssh/known_hosts")
        subprocess.run(
            args=shlex.split(f'ssh-keygen -f "{known_hosts_path}" -R "{mgmt_addr}"'), check=False
        )

        if sfp_addrs:
            set_sfp_addrs(mgmt_addr, sfp_addrs)

        return mgmt_addr


def get_redis_server(args):
    """Return the Redis object from the given URL (args.redis_server) or None if no URL was provided."""
    if args.redis_server:
        return {Redis.from_url("redis://{}:6379/0".format(args.redis_server))}
    else:
        return None


def main_mutexed(args):
    """Main entry point for mutex_hardware."""
    print("Waiting to acquire mutex for {}".format(args.dut_name), flush=True)
    with Redlock(
        key=args.dut_name,
        masters=get_redis_server(args),
        auto_release_time=1000 * 60 * args.dut_timeout,
    ):
        print("Got mutex for {}".format(args.dut_name), flush=True)
        main(args)


def main(args):
    """Main entry point for mutex_hardware with aquired mutex."""
    if args.sdimage_sdmux:
        dev_type, dev_model, sdimage_path, labgrid_device_yaml, mgmt_addr = (
            args.sdimage_sdmux.split(",")
        )
        if args.sfp_addrs:
            sfp_addrs = args.sfp_addrs.split(",")
        else:
            sfp_addrs = None
        flash_sdimage_sdmux(
            dev_model,
            sdimage_path,
            labgrid_device_yaml,
            mgmt_addr,
            sfp_addrs,
            force_release=args.force_release,
        )

    if args.sdimage_tftp:
        dev_type, dev_model, sdimage_path, initramfs_path, labgrid_device_yaml = (
            args.sdimage_tftp.split(",")
        )
        if args.sfp_addrs:
            sfp_addrs = args.sfp_addrs.split(",")
        else:
            sfp_addrs = None
        mgmt_addr = flash_sdimage_tftp(
            dev_model,
            sdimage_path,
            initramfs_path,
            labgrid_device_yaml,
            sfp_addrs,
            force_release=args.force_release,
        )

    if args.fpgas:
        working_dir = os.getcwd()
        return_code = 0
        for fpga in args.fpgas.split(","):
            os.mkdir(os.path.join(working_dir, fpga))
            os.chdir(os.path.join(working_dir, fpga))

            if args.jtag_x3xx:
                dev_type, dev_model, jtag_server, jtag_serial, fpga_folder, vivado_dir = (
                    args.jtag_x3xx.split(",")
                )
                jtag_x3xx(
                    dev_type,
                    dev_model,
                    jtag_server,
                    jtag_serial,
                    fpga_folder,
                    fpga,
                    get_redis_server(args),
                    vivado_dir,
                )

            if dev_type and dev_type in ["n3xx", "e3xx"]:
                subprocess.run(
                    shlex.split(
                        f"uhd_image_loader --args=mgmt_addr={mgmt_addr},type={dev_type},fpga={fpga}"
                    )
                )
                if sfp_addrs:
                    set_sfp_addrs(mgmt_addr, sfp_addrs)

            for command in args.test_commands:
                if args.working_dir:
                    result = subprocess.run(
                        shlex.split(command.format(fpga=fpga)), cwd=args.working_dir
                    )
                else:
                    result = subprocess.run(shlex.split(command.format(fpga=fpga)))
                if result.returncode != 0:
                    print(
                        "Command exited with return code {}".format(result.returncode), flush=True
                    )
                    print("Aborting execution (fpga={})".format(fpga), flush=True)
                    return_code = result.returncode
                    break
        sys.exit(return_code)
    else:
        return_code = 0
        for command in args.test_commands:
            if args.working_dir:
                result = subprocess.run(shlex.split(command), cwd=args.working_dir)
            else:
                result = subprocess.run(shlex.split(command))
            if result.returncode != 0:
                print("Command exited with return code {}".format(result.returncode), flush=True)
                print("Aborting execution", flush=True)
                return_code = result.returncode
                break
        sys.exit(return_code)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    group = parser.add_mutually_exclusive_group()
    # jtag_x3xx will flash the fpga for a given jtag_serial using
    # Vivado on jtag_server. It uses SSH to control jtag_server.
    # Provide fpga_path as a local path and it will be copied
    # to jtag_server.
    group.add_argument(
        "--jtag_x3xx", type=str, help="dev_type,dev_model,user@jtag_server,jtag_serial,fpga_folder"
    )
    group.add_argument(
        "--sdimage_sdmux",
        type=str,
        help="dev_type,dev_model,sdimg_path,labgrid_device_yaml,mgmt_addr",
    )
    group.add_argument(
        "--sdimage_tftp",
        type=str,
        help="dev_type,dev_model,sdimg_path,initramfs_path,labgrid_device_yaml",
    )
    parser.add_argument("--sfp_addrs", type=str, help="sfp0ip,sfp1ip,...")
    parser.add_argument("--fpgas", type=str, help="Comma delimited list of FPGAs to test")
    parser.add_argument("--dut_timeout", type=int, default=60, help="Dut mutex timeout in minutes")
    parser.add_argument(
        "--working_dir", type=str, help="Change working directory for commands to be run"
    )
    parser.add_argument(
        "--no-force-release",
        action="store_true",
        help="Don't force-release labgrid place if it was acquired before",
    )
    parser.add_argument("redis_server", type=str, help="Redis server for mutex")
    parser.add_argument("dut_name", type=str, help="Unique identifier for device under test")
    # test_commands allows for any number of shell commands
    # to execute. Call into mutex_hardware with an unlimited
    # number of commands in string format as the last positional arguments.
    parser.add_argument("test_commands", type=str, nargs="+", help="Commands to run")
    args = parser.parse_args()
    args.force_release = not args.no_force_release
    if args.redis_server:
        main_mutexed(args)
    else:
        main(args)
