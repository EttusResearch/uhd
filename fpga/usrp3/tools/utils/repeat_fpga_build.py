#!/usr/bin/env python3
#
# Copyright 2023 Ettus Research, a National Instrument Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

"""
Repeatedly runs the requested build until it builds successfully and meets
timing, up to a maximum number of tries. Builds will be retried when they
fail with timing errors or other errors that might not reoccur. Builds will
stop if a an unrecognized error occurs.
"""

import sys
import os
import signal
import time
import argparse
import subprocess
import threading
import logging
import re
import random

from enum import Enum


# Delay between build status checks
SLEEP_DELAY = 3


class Status(Enum):
    # Build status constants. Values can be returned to shell as error codes.
    SUCCESS = 0
    ERROR_TRANSIENT = 1
    ERROR_UNKNOWN = 2
    CANCELLED = 3
    TIMEOUT = 4
    NONE = 5

    def __str__(self):
        """Returns a pretty string for each enum."""
        status_string = {
            self.SUCCESS: "Success",
            self.ERROR_TRANSIENT: "Error (Transient)",
            self.ERROR_UNKNOWN: "Error (Unknown)",
            self.CANCELLED: "Cancelled",
            self.TIMEOUT: "Timeout",
            self.NONE: "Not Started",
        }
        return status_string[self]


class Worker:
    """Information for a single worker."""

    def __init__(self):
        self.status = None
        self.thread = None
        self.process = None
        self.start_time = None


class Workers:
    """A set of workers and methods for adding and checking the status of workers."""

    def __init__(self):
        self.thread_lock = threading.Lock()
        self.worker_list = []

    def new(self):
        """Create a new worker and returns its index."""
        with self.thread_lock:
            new_worker = Worker()
            self.worker_list.append(new_worker)
            num = len(self.worker_list) - 1
        return num

    def set(self, index, status=None, thread=None, process=None, start_time=None):
        """Set the status of a worker.

        Args:
            index:
                The index for the worker to update.
            status:
                If not None, set status to Status.SUCCESS, Status.ERROR_UNKNOWN, etc.
            thread:
                If not None, set thread to a Thread object.
            process:
                If not None, set process to a Popen object.
            start_time:
                If not None, set start_time to time worker started, in seconds.
        """
        with self.thread_lock:
            if status is not None:
                self.worker_list[index].status = status
            if thread is not None:
                self.worker_list[index].thread = thread
            if process is not None:
                self.worker_list[index].process = process
            if start_time is not None:
                self.worker_list[index].start_time = start_time

    def status(self, index):
        """Returns the status of the indicated worker."""
        with self.thread_lock:
            return self.worker_list[index].status

    def num_workers(self):
        """Return the total number of workers."""
        with self.thread_lock:
            return len(self.worker_list)

    def num_finished(self):
        """Return the number of workers that have finished."""
        with self.thread_lock:
            return sum(int(w.status is not None) for w in self.worker_list)

    def num_error_unknown(self):
        """Return the number of workers that finished with an unknown error."""
        with self.thread_lock:
            return sum(int(w.status == Status.ERROR_UNKNOWN) for w in self.worker_list)

    def any_success(self):
        """Returns True if any worker finished successfully."""
        with self.thread_lock:
            return any(
                not w.thread.is_alive() and w.status == Status.SUCCESS
                for w in self.worker_list
            )

    def check_timeout(self, timeout):
        """Check the timeout status of each worker. Kill it if timeout has
        expired.

        Args:
            timeout: The timeout in seconds.
        """
        with self.thread_lock:
            for index, worker in enumerate(self.worker_list):
                if time.time() - worker.start_time > timeout and worker.status == None:
                    try:
                        self.worker_list[index].status = Status.TIMEOUT
                        os.killpg(os.getpgid(worker.process.pid), signal.SIGTERM)
                    except ProcessLookupError:
                        # Process is already dead
                        pass

    def kill_jobs(self):
        """Kill any running jobs by sending SIGTERM to each process group."""
        with self.thread_lock:
            for index, worker in enumerate(self.worker_list):
                if worker.status == None:
                    try:
                        self.worker_list[index].status = Status.CANCELLED
                        os.killpg(os.getpgid(worker.process.pid), signal.SIGTERM)
                    except ProcessLookupError:
                        # Process is already dead
                        pass


def parse_args():
    """Parse the command line arguments.

    Returns:
        Populated namespace containing the arguments and their values.
    """
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    parser.add_argument(
        "--target",
        "-t",
        type=str,
        help="FPGA make target to build (e.g., X310_XG).",
    )
    parser.add_argument(
        "--image-core",
        "-y",
        type=str,
        help="For using the image builder instead of make, use this to specify "
        "the image core YAML.",
    )
    parser.add_argument(
        "--image-core-name",
        type=str,
        help="Specifies the image core name to pass to rfnoc_image_builder using the -n argument.",
    )
    parser.add_argument(
        "--fpga-dir",
        "-F",
        type=str,
        default=None,
        required=False,
        help="Specifies the FPGA directory to pass to rfnoc_image_builder using the -F argument.",
    )
    parser.add_argument(
        "--build-dir",
        "-B",
        type=str,
        default=None,
        required=False,
        help="Specifies the build directory to pass to rfnoc_image_builder using the -B argument.",
    )
    parser.add_argument(
        "--vivado-path",
        "-P",
        help="Path to the base install for Xilinx Vivado if not in default "
        "location (e.g., /tools/Xilinx/Vivado).",
        default=None,
    )
    parser.add_argument(
        "--ignore-warnings",
        "-W",
        help="Run build even when there are warnings from the RFNoC image builder",
        action="store_true",
    )
    parser.add_argument(
        "--num",
        "-n",
        type=int,
        default=4,
        required=False,
        help="Number of times to attempt the build.",
    )
    parser.add_argument(
        "--persistent",
        "-p",
        action="store_true",
        default=False,
        help=(
            "Continue retrying builds regardless of which error occurs, "
            "up to the specified number of attempts."
        ),
    )
    parser.add_argument(
        "--seed",
        "-s",
        type=int,
        default=0,
        required=False,
        help="Initial seed value to use.",
    )
    parser.add_argument(
        "--fpga-jobs",
        "-j",
        type=int,
        default=1,
        required=False,
        help="Number of parallel FPGA build jobs to run.",
    )
    parser.add_argument(
        "--ip-jobs",
        "-J",
        type=int,
        default=4,
        required=False,
        help="Number of parallel IP build jobs to run.",
    )
    parser.add_argument(
        "--timeout",
        "-T",
        type=float,
        default=0,
        help=("Set a timeout in hours for each build attempt. Use 0 for no timeout."),
    )
    parser.add_argument(
        "--all",
        "-a",
        action="store_true",
        default=False,
        help=(
            "Complete all builds, even after a successful build is completed, "
            "unless an unknown error occurs."
        ),
    )
    parser.add_argument(
        "--index",
        "-i",
        type=int,
        required=False,
        default=1,
        help=(
            "Starting index for the number to be appended to each parallel "
            "build's directory name."
        ),
    )
    return parser.parse_args()


def rfnoc_image_builder_cmd(
    build_seed,
    target,
    image_core,
    image_core_name,
    fpga_dir,
    build_dir,
    vivado_path,
    ignore_warnings,
    build_num=None,
):
    """
    Build the RFNoC image builder command for this build
    """
    cmd = ""
    if build_seed is not None:
        cmd += f"BUILD_SEED={build_seed} "
    cmd += f"rfnoc_image_builder --yaml-config {image_core} "
    if target:
        cmd += f"--target {target} "
    if image_core_name:
        cmd += f"--image-core-name {image_core_name}"
        cmd += " " if build_num == None else f"_{build_num:02} "
    if build_dir:
        cmd += f"--build-dir {build_dir}"
        cmd += " " if build_num == None else f"/{image_core_name}_{build_num:02} "
    if fpga_dir:
        cmd += f"--fpga-dir {fpga_dir} "
    if vivado_path:
        cmd += f"--vivado-path {vivado_path} "
    if ignore_warnings:
        cmd += "--ignore-warnings "
    return cmd


def run_fpga_build(
    workers,
    worker_num,
    build_num,
    build_seed,
    target,
    image_core,
    image_core_name,
    fpga_dir,
    build_dir,
    vivado_path,
    ignore_warnings,
    parallel_builds,
):
    """Performs one iteration of an FPGA build.

    Args:
        workers: Workers object containing the state of all workers
        woker_num: The number ID for this worker
        build_num: Build number to be displayed for this build
        build_seed: 32-bit signed integer to seed the FPGA build
        target: --target argument to be passed
        image_core: --image-core argument to be passed
        image_core_name: --image-core-name argument to be passed
        fpga_dir: --fpga-dir argument to be passed
        build_dir: --build-dir argument to be passed
        vivado_path: --vivado-path argument to be passed
        ignore_warnings: --ignore-warnings argument to be passed
        parallel_builds: Indicates if we are doing builds in parallel

    Returns:
        Status.SUCCESS: The build succeeded
        Status.*: The build failed for the indicated reason
    """
    logging.info(f"Starting FPGA build {build_num} with seed {build_seed}")
    output = ""
    # Give each build a unique directory number if doing parallel builds
    cmd_build_num = build_num if parallel_builds else None
    cmd = rfnoc_image_builder_cmd(
        build_seed,
        target,
        image_core,
        image_core_name,
        fpga_dir,
        build_dir,
        vivado_path,
        ignore_warnings,
        cmd_build_num,
    )
    logging.info(f"Running FPGA build command: {cmd}")
    my_env = os.environ.copy()
    with subprocess.Popen(
        cmd,
        shell=True,
        env=my_env,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        bufsize=1,
        universal_newlines=True,
        start_new_session=True,
    ) as proc:
        workers.set(worker_num, process=proc)
        prefix = f"[{build_num:02}] "
        for line in proc.stdout:
            print(prefix + line, end="")
            output += line
    if proc.returncode == 0:
        logging.info(f"Finished FPGA build {build_num} (SUCCESS)")
        status = Status.SUCCESS
    elif workers.status(worker_num) in (Status.CANCELLED, Status.TIMEOUT):
        status = workers.status(worker_num)
        logging.info((f"Finished FPGA build {build_num} ({str(status)})"))
    else:
        # Regular expressions for error strings to search for that would tell
        # us we should try again.
        transient_errors = [
            # Standard timing error:
            "The design did not satisfy timing constraints",
            # Known issue fixed in Vivado 2021.2:
            (
                "Router encountered a fatal exception of type .*"
                "Trying to tool lock on already tool locked arc"
            ),
        ]
        logging.info(f"Finished FPGA build {build_num} (FAILURE)")
        status = Status.ERROR_UNKNOWN
        for error_string in transient_errors:
            if re.search(error_string, output):
                status = Status.ERROR_TRANSIENT
                break
    workers.set(worker_num, status=status)
    return status


def run_ip_build(
    ip_jobs,
    target,
    image_core,
    image_core_name,
    fpga_dir,
    build_dir,
    vivado_path,
    ignore_warnings,
):
    """Performs the IP build.

    This is done separately so that when we do parallel FPGA builds, they don't
    separately launch IP builds that would clobber each other.

    Args:
        ip_jobs: Number or parallel jobs to use for IP build
        target: --target argument to be passed
        image_core: --image-core argument to be passed
        image_core_name: --image-core-name argument to be passed
        fpga_dir: --fpga-dir argument to be passed
        build_dir: --build-dir argument to be passed
        vivado_path: --vivado-path argument to be passed
        ignore_warnings: --ignore-warnings argument to be passed

    Returns:
        0: The IP build succeeded
        non-zero: The IP build failed
    """
    cmd = rfnoc_image_builder_cmd(
        None,
        target,
        image_core,
        image_core_name,
        fpga_dir,
        build_dir,
        vivado_path,
        ignore_warnings,
        None,
    )
    cmd += f" --ip-only --jobs {ip_jobs}"
    logging.info(f"Running IP build with command: {cmd}")
    output = ""
    with subprocess.Popen(
        cmd,
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        bufsize=1,
        universal_newlines=True,
    ) as proc:
        for line in proc.stdout:
            print(line, end="")
            output += line
    if proc.returncode != 0:
        logging.error("IP build failed! Consult output for details.")
        return Status.ERROR_UNKNOWN
    return Status.SUCCESS


def next_build_seed(previous_seed):
    """Determines the next seed to use based on the previous seed. This creates
    a reproducible sequence of values with a specific initial value.

    Args:
        previous_seed: The previous integer seed from which to determine the
            new seed.

    Returns:
        The next build seed value in the range of a 32-bit signed integer.
    """
    random.seed(previous_seed)
    return random.randint(-0x80000000, 0x7FFFFFFF)


def main():
    """Run the requested builds.

    Returns:
        The status of the build as an integer, based on the Status enum (0 if
        successful, non-zero if the build failed).
    """
    logging.basicConfig(format="[REPEAT BUILD][%(levelname)s] %(message)s")
    logging.root.setLevel(logging.INFO)
    args = parse_args()
    if not args.target and not args.image_core:
        logging.error("Either --target or --image-core must be provided!")
        return Status.NONE.value
    build_seed = args.seed
    workers = Workers()

    if args.fpga_jobs > 1 and not args.image_core_name:
        logging.error(
            "You must provide --image-core-name when doing parallel FPGA "
            "builds. Each build will use this name followed by a build number."
        )
        return Status.NONE.value

    # Build all the IP first, so it's ready to go for any parallel FPGA builds
    status = run_ip_build(
        args.ip_jobs,
        args.target,
        args.image_core,
        args.image_core_name,
        args.fpga_dir,
        args.build_dir,
        args.vivado_path,
        args.ignore_warnings,
    )
    if status != Status.SUCCESS:
        return status.value

    # Run the FPGA builds next
    status = Status.NONE
    try:
        while True:
            # See if we can start another build job
            if (
                threading.active_count() < args.fpga_jobs + 1
                and workers.num_workers() < args.num
            ):
                worker_num = workers.new()
                build_num = worker_num + args.index
                thread = threading.Thread(
                    target=run_fpga_build,
                    args=(
                        workers,
                        worker_num,
                        build_num,
                        build_seed,
                        args.target,
                        args.image_core,
                        args.image_core_name,
                        args.fpga_dir,
                        args.build_dir,
                        args.vivado_path,
                        args.ignore_warnings,
                        args.fpga_jobs > 1,
                    ),
                )
                workers.set(worker_num, thread=thread, start_time=time.time())
                thread.start()
                build_seed = next_build_seed(build_seed)
            else:
                # We've maxed out the number of workers/jobs, so check the
                # status periodically.
                time.sleep(SLEEP_DELAY)
                if args.timeout > 0:
                    workers.check_timeout(args.timeout * 3600)
                if workers.any_success():
                    if not args.all or workers.num_finished() == args.num:
                        break
                elif workers.num_finished() == args.num:
                    break

        # We're done, so kill any remaining jobs that haven't finished
        if threading.active_count() > 1:
            logging.info("Stopping remaining jobs . . .")
            workers.kill_jobs()
            while threading.active_count() > 1:
                time.sleep(0.1)

        # Print the results before returning
        logging.info(f"Build Summary:")
        for worker_num in range(workers.num_workers()):
            logging.info(
                f" - Build attempt {args.index + worker_num}: {str(workers.status(worker_num))}"
            )
        if workers.any_success():
            status = Status.SUCCESS
        elif workers.num_finished() == args.num:
            status = Status.ERROR_TRANSIENT
            logging.error("Reached maximum number of FPGA build attempts")
        elif workers.num_error_unknown() != 0 and args.stop_on_error:
            status = Status.ERROR_UNKNOWN
            logging.error("Stopped due to unexpected FPGA build error")

    except KeyboardInterrupt:
        logging.info("Received SIGINT. Aborting . . .")
        workers.kill_jobs()
        while threading.active_count() > 1:
            time.sleep(0.1)
        # Return normal Bash value for SIGINT (128+2)
        return 130

    return status.value


if __name__ == "__main__":
    sys.exit(main())
