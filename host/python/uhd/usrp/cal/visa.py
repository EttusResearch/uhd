#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
VISA-type measurement devices
"""

import re
import sys
import inspect
import importlib

class VISADevice:
    """
    Parent class for VISA/SCPI devices (that can be accessed via PyVISA)
    """
    # Must contain a dictionary res_ids. See class below as an example

    def __init__(self, resource):
        self.res = resource
        # You can query the exact model etc. like this:
        # id = self._res.query("*IDN?").strip(

    def init_power_meter(self):
        """
        Initialize this device to measure power.
        """
        raise NotImplementedError()

    def init_signal_generator(self):
        """
        Initialize this device to generate signals.
        """
        raise NotImplementedError()

    def set_frequency(self, freq):
        """
        Set frequency
        """
        raise NotImplementedError()

    def get_power_dbm(self):
        """
        Return the received/measured power in dBm (power meter) or return  the
        output power it's currently set to (signal generator).
        """
        raise NotImplementedError()


class USBPowerMeter(VISADevice):
    """
    USB-based power measurement devices
    """
    # The keys of res_ids are regular expressions, which have to match the
    # resource ID of the VISA device. The values are a name for the device, which
    # is used for informing the user which driver was found.
    #
    # A class can match any number of resource IDs. If the commands used depend
    # on the specific ID, it can be queried in the appropriate init function
    # using the *IDN? command which is understood by all VISA devices.
    res_ids = {
        r'USB\d+::2733::376::\d+::0::INSTR': 'R&S NRP-6A',
    }

    def init_power_meter(self):
        """
        Enable the sensor to read power
        """
        self.res.timeout = 5000
        self.res.write("SENS:AVER:COUN 20")
        self.res.write("SENS:AVER:COUN:AUTO ON")
        self.res.write('UNIT:POW DBM')
        self.res.write('SENS:FUNC "POW:AVG"')

    def init_signal_generator(self):
        """
        This class is for power meters, so no bueno
        """
        raise RuntimeError("This device cannot be used for signal generation!")

    def set_frequency(self, freq):
        """
        Set frequency
        """
        self.res.write('SENS:FREQ {}'.format(freq))

    def get_power_dbm(self):
        """
        Return measured power in dBm
        """
        self.res.write('INIT:IMM')
        return float(self.res.query('FETCH?'))

###############################################################################
# The dispatch function
###############################################################################
def get_visa_device(resource, key, opt_dict):
    """
    Return the VISA device object
    """
    def match_res(obj):
        """
        Check if a class obj matches the requested key
        """
        for pattern, res_id in getattr(obj, 'res_ids', {}).items():
            if re.match(pattern, key):
                print("Found VISA device: {}".format(res_id))
                return True
        return False
    # Import additional modules if requested
    members = inspect.getmembers(sys.modules[__name__])
    if 'import' in opt_dict:
        try:
            print("Loading external module: {}".format(opt_dict.get('import')))
            external_module = importlib.import_module(opt_dict.get('import'))
            members += inspect.getmembers(external_module)
        except (ModuleNotFoundError, ImportError):
            print("WARNING: Could not import module '{}'"
                  .format(opt_dict.get('import')))
    # Now browse classes and find one that matches
    for _, obj in members:
        try:
            if issubclass(obj, VISADevice) and match_res(obj):
                return obj(resource)
        except TypeError:
            continue
    raise RuntimeError("No VISA device class found for key: {}".format(key))
