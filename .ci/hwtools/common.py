#!/usr/bin/env python3
#
# Copyright 2025 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: MIT
#

import shutil

from pathlib import Path

from hwtools.api import vapi, pathapi, cmd


def update_xmlparse_files():
    if vapi.is_xmlparse():
        # copy the register map to the correct location
        variant = Path.cwd().name
        project_name = f"{variant}_FPGA"
        src_file = pathapi.get_abs_path(
            f"{project_name}/xmlparse/pri1/{project_name}.htm", base="objects"
        )
        # search the file recusively in the current directory
        dest_file = pathapi.get_abs_path(
            f"fpga/usrp3/top/x400/doc/{variant}/{project_name}.htm", base="repo"
        )
        shutil.copy(src_file, dest_file)

        # get the directory of generated files and check each file in it
        xmlparse_hdl_dir = Path(pathapi.get_abs_path("pri1/hdl", base="xmlparse"))
        for gen_file in xmlparse_hdl_dir.iterdir():
            print(f"Checking {gen_file.name}")

            # check for matching files in the repo
            existing_files = vapi.get_files_from_fileset("/" + gen_file.name)
            if (len(existing_files) != 1):
                # if there is no match for vhd files this might be fine for the NI-USRP case
                if (len(existing_files) == 0) and (gen_file.name.endswith(".vhd")):
                    print(f"Skipped")
                    continue
                # print if there are multiple or no matches
                raise Exception(f"File {gen_file.name} match set was not unique. Possible targets: {existing_files}")

            # update the file in the repo
            # Remove lines containing "not implemented" from the copied file
            with open(gen_file, "r") as file:
                lines = file.readlines()
            with open(existing_files[0], "w") as file:
                for line in lines:
                    if "not implemented" not in line:
                        file.write(line)

            # revert the file if the year is the only change
            cmd.run("git", "diff", "--unified=0", existing_files[0], raise_on_err=False)
            lines = cmd.stdout.splitlines()
            if lines:
                # skip the unified header of 5 lines
                lines = lines[5:]
                remaining_lines = [line for line in lines if not "Copyright" in line]
                if not remaining_lines:
                    cmd.run("git", "checkout", existing_files[0], raise_on_err=False)
