#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Access to GPIOs mapped into the PS via sysfs
"""

import os
from builtins import object
import pyudev
from usrp_mpm.mpmlog import get_logger

GPIO_SYSFS_BASE_DIR = '/sys/class/gpio'
GPIO_SYSFS_VALUEFILE = 'value'

def get_all_gpio_devs(parent_dev=None):
    """
    Returns a list of all GPIO chips available through sysfs. Will look
    something like ['gpiochip882', 'gpiochip123', ...]

    If there are multiple devices with the same label (example: daughterboards
    may have a single label for a shared component), a parent device needs to
    be provided to disambiguate.

    Arguments:
    parent_dev -- A parent udev device. If this is provided, only GPIO devices
                  which are a child of the parent device are returned.

    Example:
    >>> parent_dev = pyudev.Devices.from_sys_path(
            pyudev.Context(), '/sys/class/i2c-adapter/i2c-10')
    >>> get_all_gpio_devs(parent_dev)
    """
    try:
        context = pyudev.Context()
        gpios = [device.sys_name
                 for device in context.list_devices(
                     subsystem="gpio").match_parent(parent_dev)
                 if device.device_number == 0
                ]
        return gpios
    except OSError:
        # Typically means GPIO not available, maybe no overlay
        return []

def get_gpio_map_info(gpio_dev):
    """
    Returns all the map info for a given GPIO device.
    Example: If pio_dev is 'gpio882', it will list all files
    in /sys/class/gpio/gpio882/ and create a dictionary with filenames
    as keys and content as value. Subdirs are skipped.

    Numbers are casted to numbers automatically. Strings remain strings.
    """
    map_info = {}
    map_info_path = os.path.join(
        GPIO_SYSFS_BASE_DIR, gpio_dev,
    )
    for info_file in os.listdir(map_info_path):
        if not os.path.isfile(os.path.join(map_info_path, info_file)):
            continue
        map_info_value = open(os.path.join(map_info_path, info_file), 'r').read().strip()
        try:
            map_info[info_file] = int(map_info_value, 0)
        except ValueError:
            map_info[info_file] = map_info_value
    # Manually add GPIO number
    context = pyudev.Context()
    map_info['sys_number'] = int(
        pyudev.Devices.from_name(context, subsystem="gpio", sys_name=gpio_dev).sys_number
    )
    return map_info

def get_map_data(gpio_dev, path, logger=None):
    """
    Returns the value of the file found at the given path within a gpio_dev.
    Example: If gpio_dev is 'gpio882' and path is 'device/of_node/name'
    the value found at '/sys/class/gpio/gpio882/device/of_node/name' will
    be returned.

    Numbers are casted to numbers automatically. Strings remain strings.
    """
    map_path = os.path.join(GPIO_SYSFS_BASE_DIR, path, gpio_dev)
    if not os.path.isfile(map_path):
        if logger:
            logger.trace("Couldn't find a device tree file to match: `{0}'".format(map_path))
        return None
    map_info_value = open(map_path, 'r').read().strip().rstrip('\x00')
    if logger:
        logger.trace("File at `{0}' has value `{1}'".format(map_path, map_info_value))
    try:
        map_info_value = int(map_info_value, 0)
    except ValueError:
        pass
    # Manually add GPIO number
    return map_info_value

def find_gpio_device(identifiers, parent_dev=None, logger=None):
    """
    Given identifiers, returns a tuple (uio_device, map_info).
    identifiers is a dictionary of paths as keys and expected values as values
    (e.g. {"label": "tca6404"}).
    uio_device is something like 'gpio882'.
    map_info is a dictionary with information regarding the GPIO device read
    from the map info sysfs dir.
    """
    def id_dict_compare(identifiers, gpio_dev, logger=None):
        """
        Checks that a gpio_dev matches the path value pairs specified in
        identifiers.
        Returns True if the gpio_dev matches all path value pairs, otherwise
        returns False
        """
        for k in identifiers:
            if get_map_data(k, gpio_dev, logger) != identifiers[k]:
                return False
        return True

    gpio_devices = get_all_gpio_devs(parent_dev)
    if logger:
        logger.trace("Found the following UIO devices: `{0}'".format(','.join(gpio_devices)))
    for gpio_device in gpio_devices:
        map_info = get_gpio_map_info(gpio_device)
        if logger:
            logger.trace("{0} has map info: {1}".format(gpio_device, map_info))
        if id_dict_compare(identifiers, gpio_device, logger):
            if logger:
                logger.trace("Device matches identifiers: `{0}'".format(gpio_device))
            return gpio_device, map_info
    if logger:
        logger.warning("Found no matching gpio device for identifiers `{0}'".format(identifiers))
    return None, None

class SysFSGPIO(object):
    """
    API for accessing GPIOs mapped into userland via sysfs
    """

    def __init__(self, identifiers, use_mask, ddr, init_value=0, parent_dev=None):
        assert (use_mask & ddr) == ddr
        self.log = get_logger("SysFSGPIO")
        self._identifiers = identifiers
        self._use_mask = use_mask
        self._ddr = ddr
        self._init_value = init_value
        self._out_value = 0
        self.log.trace("Generating SysFSGPIO object for identifiers `{}'..."
                       .format(identifiers))
        self._gpio_dev, self._map_info = \
                find_gpio_device(identifiers, parent_dev, self.log)
        if self._gpio_dev is None:
            error_msg = \
                "Could not find GPIO device with identifiers `{}'.".format(identifiers)
            self.log.error(error_msg)
            raise RuntimeError(error_msg)
        self.log.trace("GPIO base number is {}"
                       .format(self._map_info.get("sys_number")))
        self._base_gpio = self._map_info.get("sys_number")
        self.init(self._map_info['ngpio'],
                  self._base_gpio,
                  self._use_mask,
                  self._ddr,
                  self._init_value)

    def init(self, n_gpio, base, use_mask, ddr, init_value=0):
        """
        Guarantees that all the devices are created accordingly

        E.g., if use_mask & 0x1 is True, it makes sure that 'gpioXXX' is exported.
        Also sets the DDRs.
        """
        gpio_list = [x for x in range(n_gpio) if (1<<x) & use_mask]
        self.log.trace("Initializing {} GPIOs...".format(len(gpio_list)))
        for gpio_idx in gpio_list:
            gpio_num = base + gpio_idx
            ddr_out = ddr & (1<<gpio_idx)
            ini_v = init_value & (1<<gpio_idx)
            gpio_path = os.path.join(GPIO_SYSFS_BASE_DIR, 'gpio{}'.format(gpio_num))
            if not os.path.exists(gpio_path):
                self.log.trace("Creating GPIO path `{}'...".format(gpio_path))
                open(os.path.join(GPIO_SYSFS_BASE_DIR, 'export'), 'w').write('{}'.format(gpio_num))
            ddr_str = 'out' if ddr_out else 'in'
            ddr_str = 'high' if ini_v else ddr_str
            if ini_v and ddr_out:
                self._out_value |= 1 << gpio_idx
            self.log.trace("On GPIO path `{}', setting DDR mode to {}.".format(gpio_path, ddr_str))
            open(os.path.join(GPIO_SYSFS_BASE_DIR, gpio_path, 'direction'), 'w').write(ddr_str)

    def set(self, gpio_idx, value=None):
        """
        Assert a GPIO at given index.

        Note: The GPIO must be in the valid range, and it's DDR value must be
        high (for "out").
        """
        if value is None:
            value = 1
        assert (1<<gpio_idx) & self._use_mask
        assert (1<<gpio_idx) & self._ddr
        assert int(value) in [0, 1]
        value = int(value)
        gpio_num = self._base_gpio + gpio_idx
        gpio_path = os.path.join(GPIO_SYSFS_BASE_DIR, 'gpio{}'.format(gpio_num))
        value_path = os.path.join(gpio_path, GPIO_SYSFS_VALUEFILE)
        self.log.trace("Writing value `{}' to `{}'...".format(value, value_path))
        assert os.path.exists(value_path)
        open(value_path, 'w').write('{}'.format(value))
        if value:
            self._out_value |= 1 << gpio_idx
        else:
            self._out_value &= ~(1 << gpio_idx)

    def reset(self, gpio_idx):
        """
        Deassert a GPIO at given index.

        Note: The GPIO must be in the valid range, and it's DDR value must be
        high (for "out").
        """
        self.set(gpio_idx, value=0)

    def get(self, gpio_idx):
        """
        Read back a GPIO at given index.

        Note: The GPIO must be in the valid range. If it's DDR value is
        low (for "in") then the register value is read from a local variable.
        """
        assert (1<<gpio_idx) & self._use_mask
        if (1<<gpio_idx) & (~self._ddr):
            gpio_num = self._base_gpio + gpio_idx
            gpio_path = os.path.join(GPIO_SYSFS_BASE_DIR, 'gpio{}'.format(gpio_num))
            value_path = os.path.join(gpio_path, GPIO_SYSFS_VALUEFILE)
            assert os.path.exists(value_path)
            read_value = int(open(value_path, 'r').read().strip())
            self.log.trace("Reading value {} from `{}'...".format(read_value, value_path))
        else:
            read_value = 1 if self._out_value & (1 << gpio_idx) else 0
            self.log.trace("Reading value {} from local var".format(read_value))
        return read_value


class GPIOBank:
    """
    Usability / convenience wrapper for GPIO banks accessed by SysFSGPIO

    This class is useful when:
    - Accessing a GPIO bank that is hanging off of a SysFSGPIO node
    - All pins in this bank are consecutive pins on the SysFSGPIO node
    - The pins can be at an offset (i.e., do not have to start at pin 0 of the
      SysFSGPIO node)
    - The pins might have to be written/read as a whole

    For example, assume that a device has 8 GPIO pins, starting at pin 10. The
    user wants to read all pins at once as a single byte:

    >>> gpio_bank = GPIOBank(
            label_dict, # See SysFSGPIO for this parameter
            10,         # Pin offset
            0xFF,       # 8 pins. Must be consecutive ones!
            0x00)       # All pins are readable
    >>> if gpio_bank.get_all() == 3:
        print("Pins 0 and 1 are high!")
    """
    def __init__(self, uio_identifiers, offset, usemask, ddr):
        self._gpiosize = bin(usemask).count("1")
        # Make sure the pins are all one:
        assert (1 << self._gpiosize) == usemask+1
        self._offset = offset
        self._ddr = ddr
        self._usemask = usemask
        self._gpios = SysFSGPIO(
            uio_identifiers,
            self._usemask << self._offset,
            self._ddr << self._offset
        )

    def set(self, index, value=None):
        """
        Set a pin by index
        """
        assert index in range(self._gpiosize)
        self._gpios.set(self._offset + index, value)

    def reset_all(self):
        """
        Clear all pins
        """
        for i in range(self._gpiosize):
            if self._ddr & i:
                self._gpios.reset(self._offset+i)

    def reset(self, index):
        """
        Clear a pin by index

        Read back a pin by index. See also SysFSGPIO.reset(). The DDR value for
        this pin must be high.
        """
        assert index in range(self._gpiosize)
        self._gpios.reset(self._offset + index)

    def get_all(self):
        """
        Read back all pins. Pins with a DDR value of 1 ("output") are left zero.
        """
        result = 0
        for i in range(self._gpiosize):
            if not (1<<i) & self._ddr:
                value = self._gpios.get(self._offset + i)
                result = (result << 1) | value
        return result

    def get(self, index):
        """
        Read back a pin by index. See also SysFSGPIO.get()
        """
        assert index in range(self._gpiosize)
        return self._gpios.get(self._offset + index)
