#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Daughterboard flash implementation
"""

import subprocess
import time
from usrp_mpm.sys_utils.udev import get_device_from_symbol
from usrp_mpm.sys_utils.mount import Mount
from usrp_mpm.sys_utils import dtoverlay
from usrp_mpm.mpmlog import get_logger

class DBFlash():
    """
    Class for accessing (mounting) the daughterboard flash
    """

    def __init__(self, slot_idx, log=None):
        self.dt_symbol = 'db{}_flash'.format(slot_idx)
        self.overlay = self.dt_symbol
        if log is None:
            self.log = get_logger("DBFlash")
        else:
            self.log = log.getChild("DBFlash")
        self.mount = None
        self.mtd_devpath = None
        self.mtdblock_devpath = None
        self.initialized = False

    def init(self):
        """
        initialize (mount) the daughterboard flash
        """
        ret = False
        self.log.trace("Initializing daughterboard flash")
        dtoverlay.apply_overlay_safe(self.overlay)
        time.sleep(0.2)
        try:
            self.mtd_devpath = get_device_from_symbol(self.dt_symbol, ['*', 'mtd'])
            self.mtdblock_devpath = get_device_from_symbol(
                self.dt_symbol, ['*', 'block'])
        except FileNotFoundError:
            raise ValueError(
                "could not find MTD/-block device for device tree symbol {}".format(
                    self.dt_symbol))
        try:
            self.mount = Mount(self.mtdblock_devpath, '/mnt/' + self.dt_symbol,
                               ['-t', 'jffs2'], log=self.log)
            if not self.mount.ismounted():
                ret = self.mount.mount()
                if not ret:
                    raise RuntimeError()
            self.initialized = True
        except:
            self.log.warning("Failed to initialize daughterboard flash")
        return ret

    def get_mount_point(self):
        """
        returns the mount point (None when not mounted)
        """
        return self.mount.get_mount_point()

    def deinit(self):
        """
        deinitialize (unmount) the daughterboard flash
        """
        if self.initialized:
            self.log.trace("Deinitializing daughterboard flash")
            ret = self.mount.unmount()
            if not ret:
                self.log.warning("Failed to deinitialize daughterboard flash")
            else:
                dtoverlay.rm_overlay_safe(self.overlay)
                self.initialized = False
            return ret
        return False

    def clear_flash(self):
        """
        Clear the daughterboard flash

        Attention! This will erase all data in the flash which will cause loss
        of any data stored in the flash
        """
        self.log.info("Clearing daughterboard flash")
        proc = subprocess.run(['flash_erase', self.mtd_devpath, '0', '0'], check=True)
        self.log.trace(proc)
        return True
