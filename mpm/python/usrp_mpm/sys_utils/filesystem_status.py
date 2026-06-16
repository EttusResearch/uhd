#
# Copyright 2020 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Utilities for checking the filesystem status
"""

import hashlib
import pathlib
import subprocess
import time


def get_uhd_version(filesystem_root="/"):
    file = pathlib.Path(filesystem_root, "usr/bin/uhd_config_info")
    versionstring = subprocess.check_output([file, "--version"]).decode("utf-8").splitlines()[0]
    return versionstring


def get_mender_artifact(filesystem_root="/", parse_manually=False):
    def parse_artifact(output):
        for line in output.splitlines():
            if line.startswith("artifact_name="):
                return line[14:]
        return None

    if filesystem_root != "/":
        parse_manually = True
    if parse_manually:
        file = pathlib.Path(filesystem_root, "etc/mender/artifact_info")
        if not file.exists():
            return None
        return parse_artifact(file.read_text())
    try:
        # start_new_session=True isolates the child from the parent's process
        # group (equivalent to setsid()). Unlike preexec_fn, it is fork-safe
        # because it runs after exec, avoiding deadlocks on inherited locks.
        result = subprocess.run(
            ["/usr/bin/mender", "show-artifact"],
            capture_output=True,
            start_new_session=True,
        )
        return result.stdout.decode("utf-8").splitlines()[0]
    except Exception:
        return None


def get_fs_version(filesystem_root="/"):
    file = pathlib.Path(filesystem_root, "etc/version")
    if not file.exists():
        return None
    return file.read_text().splitlines()[0]


def get_opkg_status_date(date_only=False, filesystem_root="/"):
    if date_only:
        tformat = "%Y-%m-%d"
    else:
        tformat = "%Y-%m-%d %H:%M:%S"
    file = pathlib.Path(filesystem_root, "var/lib/opkg/status")
    if not file.exists():
        return None
    return time.strftime(tformat, time.gmtime(file.stat().st_mtime))


def get_opkg_status_md5sum(filesystem_root="/"):
    file = pathlib.Path(filesystem_root, "var/lib/opkg/status")
    if not file.exists():
        return None
    return hashlib.md5(file.read_bytes()).hexdigest()
