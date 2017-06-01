#
# Copyright 2017 Ettus Research (National Instruments)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
"""
Access to GPIOs mapped into the PS via sysfs
"""

import os
from builtins import object
import pyudev
from .mpmlog import get_logger

GPIO_SYSFS_BASE_DIR = '/sys/class/gpio'
GPIO_SYSFS_LABELFILE = 'label'
GPIO_SYSFS_VALUEFILE = 'value'

def get_all_gpio_devs():
    """
    Returns a list of all GPIO chips available through sysfs. Will look
    something like ['gpiochip882', 'gpiochip123', ...]
    """
    try:
        context = pyudev.Context()
        gpios = [device.sys_name
                 for device in context.list_devices(subsystem="gpio")
                 if os.path.exists(os.path.join( # udev probably has better ways to do this
                     GPIO_SYSFS_BASE_DIR,
                     device.sys_name,
                     GPIO_SYSFS_LABELFILE
                 ))
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

def find_gpio_device(label, logger=None):
    """
    Given a label, returns a tuple (uio_device, map_info).
    uio_device is something like 'gpio882'. map_info is a dictionary with
    information regarding the GPIO device read from the map info sysfs dir.
    """
    gpio_devices = get_all_gpio_devs()
    if logger:
        logger.trace("Found the following UIO devices: `{0}'".format(','.join(gpio_devices)))
    for gpio_device in gpio_devices:
        map_info = get_gpio_map_info(gpio_device)
        if logger:
            logger.trace("{0} has map info: {1}".format(gpio_device, map_info))
        if map_info.get('label') == label:
            if logger:
                logger.trace("Device matches label: `{0}'".format(gpio_device))
            return gpio_device, map_info
    if logger:
        logger.warning("Found no matching gpio device for label `{0}'".format(label))
    return None, None

class SysFSGPIO(object):
    """
    API for accessing GPIOs mapped into userland via sysfs
    """

    def __init__(self, label, use_mask, ddr):
        assert (use_mask & ddr) == ddr
        self.log = get_logger("SysFSGPIO")
        self._label = label
        self._use_mask = use_mask
        self._ddr = ddr
        self.log.trace("Generating SysFSGPIO object for label `{}'...".format(label))
        self._gpio_dev, self._map_info = find_gpio_device(label, self.log)
        if self._gpio_dev is None:
            self.log.error("Could not find GPIO device with label `{}'.".format(label))
        self.log.trace("GPIO base number is {}".format(self._map_info.get("sys_number")))
        self._base_gpio = self._map_info.get("sys_number")
        self.init(self._map_info['ngpio'], self._base_gpio, self._use_mask, self._ddr)

    def init(self, n_gpio, base, use_mask, ddr):
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
            gpio_path = os.path.join(GPIO_SYSFS_BASE_DIR, 'gpio{}'.format(gpio_num))
            if not os.path.exists(gpio_path):
                self.log.trace("Creating GPIO path `{}'...".format(gpio_path))
                open(os.path.join(GPIO_SYSFS_BASE_DIR, 'export'), 'w').write('{}'.format(gpio_num))
            ddr_str = 'out' if ddr_out else 'in'
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
        gpio_num = self._base_gpio + gpio_idx
        gpio_path = os.path.join(GPIO_SYSFS_BASE_DIR, 'gpio{}'.format(gpio_num))
        value_path = os.path.join(gpio_path, GPIO_SYSFS_VALUEFILE)
        self.log.trace("Writing value `{}' to `{}'...".format(value, value_path))
        assert os.path.exists(value_path)
        open(value_path, 'w').write('{}'.format(value))

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

        Note: The GPIO must be in the valid range, and it's DDR value must be
        low (for "in").
        """
        assert (1<<gpio_idx) & self._use_mask
        assert (1<<gpio_idx) & (~self._ddr)
        gpio_num = self._base_gpio + gpio_idx
        gpio_path = os.path.join(GPIO_SYSFS_BASE_DIR, 'gpio{}'.format(gpio_num))
        value_path = os.path.join(gpio_path, GPIO_SYSFS_VALUEFILE)
        self.log.trace("Writing value from `{}'...".format(value_path))
        assert os.path.exists(value_path)
        return int(open(value_path, 'r').read().strip())

