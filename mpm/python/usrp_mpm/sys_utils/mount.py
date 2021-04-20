#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Utilities for creating a mount point
"""

import subprocess
import os
from usrp_mpm.mpmlog import get_logger

class Mount():
    """
    Class for creating a mount point
    """

    def __init__(self, devicepath, mountpoint, options=None, log=None):
        assert isinstance(devicepath, str)
        assert isinstance(mountpoint, str)
        assert isinstance(options, list)
        self.devicepath = devicepath
        self.mountpoint = mountpoint
        self.options = options
        if log is None:
            self.log = get_logger("Mount")
        else:
            self.log = log.getChild("Mount")
        self.log.trace("Early initialization: devicepath={}, mountpoint={}, options={}".format(
            devicepath, mountpoint, options))

    def ismounted(self):
        """
        Returns true if the mount point is mounted
        """
        assert self.devicepath is not None
        with open("/etc/mtab") as mtab_f:
            return any(
                line.split()[0] == self.devicepath and line.split()[1] == self.mountpoint \
                for line in mtab_f
            )

    def get_mount_point(self):
        """
        returns the mount point (None when not mounted)
        """
        if not self.ismounted():
            return None
        return self.mountpoint

    def prepare_mountpoint(self):
        """
        Creates the mount point directory (if not already existing)
        """
        if not os.path.exists(self.mountpoint):
            os.makedirs(self.mountpoint)
        return True

    def delete_mountpoint(self):
        """
        Deletes the mount point directory
        """
        os.removedirs(self.mountpoint)
        return True

    def mount(self):
        """
        Mounts the mount point
        """
        if self.ismounted():
            self.log.debug("{} was already mounted".format(self.mountpoint))
            return True
        self.prepare_mountpoint()
        self.log.debug("Mounting {}".format(self.mountpoint))
        cmd = ['mount']
        if self.options:
            cmd.extend(self.options)
        cmd.append(self.devicepath)
        cmd.append(self.mountpoint)
        proc = subprocess.run(cmd, check=True)
        self.log.trace(proc)
        return True

    def unmount(self):
        """
        Unmounts the mount point
        """
        if not self.ismounted():
            self.log.warning("{} was not mounted".format(self.mountpoint))
            return True
        self.log.debug("Unmounting {}".format(self.mountpoint))
        cmd = ['umount', self.mountpoint]
        proc = subprocess.run(cmd, check=True)
        self.log.trace(proc)
        self.delete_mountpoint()
        return True
