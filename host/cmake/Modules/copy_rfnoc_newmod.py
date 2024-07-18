# /usr/bin/env python3
"""Image Builder Helper: Create the template for RFNoC OOT modules.

Copyright 2024 Ettus Research, a National Instruments Brand

SPDX-License-Identifier: GPL-3.0-or-later
"""

import argparse
import importlib
import os
import sys

from ruamel.yaml import YAML


def parse_args():
    """Create argument parser and return args."""
    arg_parser = argparse.ArgumentParser(description="Create the template for RFNoC OOT modules.")
    arg_parser.add_argument("--source", required=True, help="Source directory")
    arg_parser.add_argument("--dest", required=True, help="Destination directory")
    arg_parser.add_argument("--module-dir", required=True, help="Module directory")
    return arg_parser.parse_args()


def main():
    """Do the copying."""
    args = parse_args()
    # Load the step configuratio as a Python object
    yaml = YAML(typ="safe", pure=True)
    step_yml = os.path.join(os.path.dirname(os.path.realpath(__file__)), "create_newmod.yml")
    with open(step_yml, "r", encoding="utf-8") as f:
        cmd = yaml.load(f)
    # Load the step executor from within the tree
    sys.path.insert(0, os.path.normpath(args.module_dir))
    importlib.import_module("rfnoc_utils.log").init_logging(color_mode="off", log_level="info")
    executor_vars = {
        "SOURCE_DIR": args.source,
        "DEST_DIR": args.dest,
    }
    # Now we can run the steps
    executor = importlib.import_module("rfnoc_utils.step_executor").StepExecutor(
        executor_vars, args, cmd
    )
    executor.run(cmd["steps"])
    return 0


if __name__ == "__main__":
    sys.exit(main())
