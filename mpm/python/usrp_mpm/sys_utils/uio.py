#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Access to UIO mapped memory.
"""

import os
from contextlib import contextmanager
from builtins import object
import pyudev
import usrp_mpm.libpyusrp_periphs as lib
from usrp_mpm.mpmlog import get_logger

UIO_SYSFS_BASE_DIR = '/sys/class/uio'
UIO_DEV_BASE_DIR = '/dev'


@contextmanager
def open_uio(label=None, path=None, length=None, read_only=True, offset=None):
    """Convenience function for creating a UIO object.
    Use this like you would open() for a file"""
    uio_obj = UIO(label, path, length, read_only, offset)
    uio_obj.open()
    yield uio_obj
    uio_obj.close()


def get_all_uio_devs():
    """
    Return a list of all uio devices. Will look something like
    ['uio0', 'uio1', ...].
    """
    try:
        context = pyudev.Context()
        paths = [os.path.split(device.device_node)[-1]
                 for device in context.list_devices(subsystem="uio")]
        return paths
    except OSError:
        # Typically means UIO devices
        return []


def get_uio_map_info(uio_dev, map_num):
    """
    Returns all the map info for a given UIO device and map number.
    Example: If uio_dev is 'uio0', and map_num is 0, it will list all files
    in /sys/class/uio/uio0/maps/map0/ and create a dictionary with filenames
    as keys and content as value.

    Numbers are casted to numbers automatically. Strings remain strings.
    """
    map_info = {}
    map_info_path = os.path.join(
        UIO_SYSFS_BASE_DIR, uio_dev, 'maps', 'map{0}'.format(map_num)
    )
    for info_file in os.listdir(map_info_path):
        map_info_value = open(os.path.join(map_info_path, info_file), 'r').read().strip()
        try:
            map_info[info_file] = int(map_info_value, 0)
        except ValueError:
            map_info[info_file] = map_info_value
    return map_info


def find_uio_device(label, logger=None):
    """
    Given a label, returns a tuple (uio_device, map_info).
    uio_device is something like '/dev/uio0'. map_info is a dictionary with
    information regarding the UIO device read from the map info sysfs dir.
    Note: We assume a single map (map0) for all UIO devices here.
    """
    uio_devices = get_all_uio_devs()
    if logger:
        logger.trace("Found the following UIO devices: `{0}'".format(','.join(uio_devices)))
    for uio_device in uio_devices:
        map0_info = get_uio_map_info(uio_device, 0)
        logger.trace("{0} has map info: {1}".format(uio_device, map0_info))
        if map0_info.get('name') == label:
            if logger:
                logger.trace("Device matches label: `{0}'".format(uio_device))
            return os.path.join(UIO_DEV_BASE_DIR, uio_device), map0_info
    if logger:
        logger.warning("Found no matching UIO device for label `{0}'".format(label))
    return None, None


class UIO(object):
    """
    Provides peek/poke interfaces for uio-mapped memory.

    This object will not, by default, open the associated UIO device. To
    actually open the device, you have two options:
    - Use the instantiation of this class as a context manager (using a `with`
      statement), like this:

    >>> with UIO(path="/dev/uio0") as uio0:
    >>>     uio0.peek32(addr)
    >>>     uio0.poke32(addr, value)

    - Manually call open() and close():

    >>> uio0 = UIO(path="/dev/uio0")
    >>> uio0.open()
    >>> uio0.peek32(addr)
    >>> uio0.poke32(addr, value)
    >>> uio0.close()

    Arguments:
    label -- Label of the UIO device. The label is set in the device tree
             overlay
    path -- Path to UIO device, e.g. '/dev/uio0'. This is ignored if 'label' is
            provided.
    length -- Number of bytes in the address space (is passed to mmap.mmap).
              This is usually automatically determined. No need to set it.
              Unless you really know what you're doing.
    read_only -- Boolean; True == ro, False == rw
    offset -- Passed to mmap.mmap.
              This is usually automatically determined. No need to set it.
              Unless you really know what you're doing.
    """
    def __init__(self, label=None, path=None, length=None, read_only=True, offset=None):
        self.log = get_logger('UIO')
        if label is None:
            self._path = path
            self.log.trace("Using UIO device `{0}'".format(path))
            uio_device = os.path.split(path)[-1]
            self.log.trace("Getting map info for UIO device `{0}'".format(uio_device))
            map_info = get_uio_map_info(uio_device, 0)
            # Python can't tell the size of a uio device by itself
            assert length is not None
        else:
            self.log.trace("Using UIO device by label `{0}'".format(label))
            self._path, map_info = find_uio_device(label, self.log)
        offset = offset or map_info['offset'] # If we ever support multiple maps, check if this is correct...
        assert offset == 0 # ...and then remove this line
        length = length or map_info['size']
        self.log.trace("UIO device is being opened read-{0}.".format("only" if read_only else "write"))
        if self._path is None:
            self.log.error("Could not find a UIO device for label {0}".format(label))
            raise RuntimeError("Could not find a UIO device for label {0}".format(label))
        self._read_only = read_only
        # Our UIO objects are managed in C++ land, which gives us more granular control over
        # opening and closing
        self._uio = lib.types.mmap_regs_iface(self._path, length, offset, self._read_only, False)
        # Reference counter for safely __enter__ and __exit__-ing
        self._ref_count = 0

    def __enter__(self):
        return self.open()

    def __exit__(self, exc_type, exc_value, traceback):
        self.close()
        return exc_type is None

    def open(self):
        """Actually open the UIO device.

        You need to call this before doing peeks and pokes. See also close().

        If you're using the UIO object as a context manager, it will open the
        file automatically.
        """
        if self._ref_count == 0:
            self._uio.open()
        self._ref_count += 1
        return self

    def close(self):
        """Close a UIO device.

        UIO devices can be problematic with regards to file descriptor leakage,
        so it is recommended to close a UIO device when it is no longer needed.
        """
        self._ref_count -= 1
        if self._ref_count == 0:
            self._uio.close()

    def peek32(self, addr):
        """
        Returns the 32-bit value starting at address addr as an integer
        """
        return self._uio.peek32(addr)

    def poke32(self, addr, val):
        """
        Writes the 32-bit value val to address starting at addr.
        Will throw if read_only was set to True.
        A value that exceeds 32 bits will be truncated to 32 bits.
        """
        assert not self._read_only
        return self._uio.poke32(addr, val)
