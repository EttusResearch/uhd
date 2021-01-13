#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Utilities for udev lookups
"""

import os
import glob
import pyudev
from pathlib import Path

DT_BASE = "/proc/device-tree"

def get_eeprom_paths_by_symbol(symbol_name_glob):
    """
    Searches for EEPROM file locaction of symbol names under
    /proc/device-tree/__symbols__.
    args: symbol_name_glob: symbol name(s) to search (allows glob expressions)
    returns: dictionary with name found under __symbols__ as key
             and corresponding nvmem file path as value.
             The dictionary keys are sorted alphabetically.
    raises: FileNotFoundExcepiton: in case a symbol file could not be read.
    """
    symbol_base = os.path.join(DT_BASE, "__symbols__")
    context = pyudev.Context()
    devices = context.list_devices(subsystem="nvmem")
    of_nodes = [os.path.join(dev.sys_path, "of_node") for dev in devices]

    def read_symbol_file(symbol_file):
        with open(symbol_file) as f:
            symbol_path = f.read()
            # remove leading slash and trailing terminating char before
            # building the full path for symbol
            symbol_path = os.path.join(DT_BASE, symbol_path[1:-1])
            return symbol_path

    def find_device_path(path):
        for dev in of_nodes:
            if not os.path.exists(dev):
                # not all nvmem devices have an of_node
                continue
            elif os.path.samefile(dev, path):
                return os.path.join(os.path.dirname(dev), "nvmem")
        return None

    eeproms = glob.glob(os.path.join(symbol_base, symbol_name_glob))
    paths = {os.path.basename(eeprom): read_symbol_file(eeprom)
             for eeprom in eeproms}
    return {name: find_device_path(path)
            for name, path in sorted(paths.items())}


def get_device_from_dt_symbol(symbol, subsystem=None, context=None):
    """
    Return the device associated with the device tree symbol, which usually
    is a label on a specific node of interest
    """
    symfile = Path(DT_BASE) / '__symbols__' / symbol
    fullname = symfile.read_text()
    if context is None:
        context = pyudev.Context()
    devices = list(context.list_devices(OF_FULLNAME=fullname, subsystem=subsystem))
    if not devices:
        return None
    return devices[0]


def get_eeprom_paths(address):
    """
    Return list of EEPROM device paths for a given I2C address.
    If no device paths are found, an empty list is returned.
    """
    context = pyudev.Context()
    parent = pyudev.Device.from_name(context, "platform", address)
    paths = [d.device_node if d.device_node is not None else d.sys_path
             for d in context.list_devices(parent=parent, subsystem="nvmem")]
    if len(paths) == 0:
        return []
    # We need to sort this so 9-0050 comes before 10-0050 (etc.)
    maxlen = max((len(os.path.split(p)[1]) for p in paths))
    paths = sorted(
        paths,
        key=lambda x: "{:>0{maxlen}}".format(os.path.split(x)[1], maxlen=maxlen)
    )
    return [os.path.join(x, 'nvmem') for x in paths]

def get_spidev_nodes(spi_master):
    """
    Return list of spidev device paths for a given SPI master. If no valid paths
    can be found, an empty list is returned.
    """
    context = pyudev.Context()
    parent = pyudev.Device.from_name(context, "platform", spi_master)
    return [
        device.device_node
        for device in context.list_devices(parent=parent, subsystem="spidev")
    ]

def get_device_from_symbol(symbol, subsystems):
    """
    Find the first device and return its name where the parent device the DT
    symbol name matches and the hierarchy of subsystems (e.g. ['spi', 'spidev'])
    match
    """
    assert isinstance(subsystems,list)
    context = pyudev.Context()
    device = get_device_from_dt_symbol(symbol, subsystem=subsystems.pop(0), context=context)
    if device is None:
        return None
    for subsystem in subsystems:
        devices = list(context.list_devices(parent=device, subsystem=subsystem))
        if not devices:
            return None
        device = devices[0]
    return device.properties.get('DEVNAME')

def dt_symbol_get_spidev(symbol):
    """
    Return spidev associated with the given device tree symbol
    """
    return get_device_from_symbol(symbol, ['spi', 'spidev'])
