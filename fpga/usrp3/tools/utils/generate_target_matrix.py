#!/usr/bin/env python3
#
# Copyright 2025 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
"""
Generate a target matrix for parallel FPGA image builds.

Given an RFNoC image-core YAML file, this utility creates one pipeline target
for each requested build. Each target receives a unique image-core name,
artifact name, and build seed, and is emitted as YAML suitable for use as a
pipeline target matrix.

The generated matrix can be used with any pipeline that includes
``#file:stages-uhd-pipeline.yml``. The primary consumer is ``fpga/.ci/fpga-pipeline.yml``.
"""
import argparse
from pathlib import Path
import sys

from ruamel.yaml import YAML


# The input filename is expected to end with _rfnoc_image_core.yml.
def generate_output(args):
    yaml = YAML(typ="safe")
    with open(args.yaml_file, "r") as file:
        data = yaml.load(file)

    image_core_name = data.get("image_core_name", None)
    artifact_name = Path(args.yaml_file).stem.replace("_rfnoc_image_core", "")

    result = {}
    for i in range(0, args.numJobs):
        artifact_name_with_index = f"{artifact_name.upper()}_{i}"
        result[artifact_name_with_index] = {
            "image_core": Path(args.yaml_file).name,
            "image_core_name": f"{image_core_name}_{i}",
            "artifact_name": artifact_name_with_index,
            "timeout": 720,
            "max_attempts": 1,
            "build_seed": hash(str(i)) & 0xFFFFFFFF,  # Ensure a 32-bit integer
        }

    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Generate a target matrix for parallel FPGA image builds."
    )
    parser.add_argument("yaml_file", type=str, help="Path to the rfnoc_image_core YAML file.")
    parser.add_argument("-n", "--numJobs", type=int, default=10, help="Number of builds to run.")

    args = parser.parse_args()
    output = generate_output(args)

    yaml = YAML()
    yaml.default_flow_style = False
    yaml.dump(output, sys.stdout)
