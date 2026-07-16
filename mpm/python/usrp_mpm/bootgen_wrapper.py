#!/usr/bin/env python3
#
# Copyright 2026 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""Utility functions provided by wrapping the "bootgen" tool.

verify: Verify the signature of a .bin file.
extract_bitstream: Extract the FPGA bitstream from a .bin file.
"""

import shutil
import subprocess
import tempfile
from pathlib import Path


def verify(input, arch):
    """Verify the signature of the .bin file."""
    proc = subprocess.run(["bootgen", "-arch", arch, "-verify", input], capture_output=True)
    if proc.returncode != 0:
        stderr = proc.stderr.decode().splitlines()[0]
        raise RuntimeError(
            f'FPGA bitstream (.bin file) verification using bootgen failed: "{stderr}"'
        )


def extract_bitstream(input, output, arch):
    """Extract the FPGA bitstream from the bootgen generated .bin file."""
    temp_dir = tempfile.TemporaryDirectory()
    proc = subprocess.run(
        ["bootgen", "-arch", arch, "-dump", input, "-dump_dir", temp_dir.name], capture_output=True
    )
    if proc.returncode != 0:
        if proc.stderr.decode().startswith("[ERROR]  : Boot Header not found"):
            # The file is already a FPGA bitstream in .bin file format, just copy it
            try:
                shutil.copyfile(input, output)
            except shutil.SameFileError:
                # in-place conversion is allowed
                pass
        else:
            stderr = proc.stderr.decode().splitlines()[0]
            raise RuntimeError(
                f'Could not extract bitstream from .bin file, bootgen returned "{stderr}"'
            )
    else:
        extracted_bins = list(Path(temp_dir.name).glob("*.bin"))
        num_bins = len(extracted_bins)
        if num_bins == 0:
            raise RuntimeError("Could not find extracted .bin file in bootgen dump output")
        elif num_bins > 1:
            raise RuntimeError(
                f"Invalid .bin file: found {num_bins} partitions but expected only one."
            )
        shutil.move(str(extracted_bins[0]), output)
