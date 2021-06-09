#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Switch Device Classes for UHD Power Calibration
"""

import sys
import inspect
import importlib
from .ni_rf_instr import get_modinst_device

# pylint: disable=too-few-public-methods
class SwitchBase:
    """
    Base class to connect ports of measurement equipment and USRP.
    """
    key = ""

    def connect(self, chan, antenna):
        """
        Connect a port of the USRP (DUT) denoted by chan and antenna to the
        measurement device.
        :param chan: channel to connect
        :param antenna: antenna to connect
        """
        raise NotImplementedError()


class ManualSwitch(SwitchBase):
    """
    ManualSwitch is the fallback implementation if no other switch can be
    found. It asks the user to change cable setup and halts the calibration
    until the user confirms the configuration. If `mode=auto` is given in
    options connect call assumes there is no need to pause for connecting
    measurement device with DUT (e.g. only one path is measured).
    """
    def __init__(self, direction, options=None):
        self.direction = direction
        self.mode = options.get('mode', '')

    def connect(self, chan, antenna):
        """
        Connect a port of the USRP (DUT) denoted by chan and antenna to the
        measurement device. In this manual mode the user is responsible to
        ensure correct wiring. The script waits until the user confirms the
        setup.
        :param chan: channel to connect
        :param antenna: antenna to connect
        """
        if self.mode == 'auto':
            return # no need to wait for manual connection
        dev_type = "signal generator" if self.direction == 'rx' else "power meter"
        input(f"[{self.direction}] Connect your {dev_type} to device channel {chan}, "
              f"antenna {antenna}. Then, hit Enter.")

class NISwitch(SwitchBase):
    """
    Use NI switch devices to automatically connect measurement devices with
    DUT.
    """
    key = "niswitch"

    def __init__(self, options):
        # connections stores the connected ports for each chan/antenna
        # combination. During connect call connection is checked for an
        # appropriate key. If there is no such key the next channel of the
        # current port switch is used.
        # To get a working setup
        self.connections = {}

        device = get_modinst_device("NI-SWITCH", options.get("name", ""))
        # pylint: disable=import-outside-toplevel
        # disable import warning. We do not want to make niswitch a mandatory
        # package for users who do not use this class
        import niswitch
        self.session = niswitch.Session(device.device_name)
        self.port = options.get("port", "comA")

    def connect(self, chan, antenna):
        """
        Connect a port of the USRP (DUT) denoted by chan and antenna to the
        measurement device.
        The NI switch connects the channel of the switch (named "chXY") to the
        port of the switch ("comX") which was given during initialization.
        The connection is made consecutively, meaning the first requested
        configuration of channel and antenna is connected to chX1, the second
        to chX2 and so on. X is the identifier of the port given in
        initialization. The user has to make sure that the SMA port of the DUT
        are cabled in the right order. The calibration loops over all
        (configured) channels and for each channel over the (configured)
        antennas. The order of channels and antennas can be changed via script
        parameter. The loop order is fixed.
        :param chan: channel to connect
        :param antenna: antenna to connect
        """
        key = (chan, antenna)
        if key not in self.connections:
            # connection not known yet, generate next connection pair
            # first item is port used by this instance
            # second item is next channel, channels are named chXY where
            # X is the letter of the current port (derived from port name) and
            # Y is a number starting at 1
            switch_channel = "ch%d%s" % (len(self.connections) + 1,
                                         self.port[-1:])
            self.connections[key] = (self.port, switch_channel)

        connection = self.connections[key]
        print("Connecting %s-%s at switch to measure %d-%s of DUT" %
              (connection[0], connection[1], chan, antenna))
        self.session.disconnect_all()
        self.session.connect(connection[0], connection[1])

###############################################################################
# The dispatch function
###############################################################################
def get_switch(direction, dev_key, options):
    """
    Return the measurement device object
    """
    opt_dict = {
        k[0]: k[1] if len(k) > 1 else None for k in [x.split("=", 1) for x in options]
    }
    members = inspect.getmembers(sys.modules[__name__])
    if 'import' in opt_dict:
        try:
            print("Loading external module: {}".format(opt_dict.get('import')))
            external_module = importlib.import_module(opt_dict.get('import'))
            members += inspect.getmembers(external_module)
        except (ModuleNotFoundError, ImportError):
            print("WARNING: Could not import module '{}'"
                  .format(opt_dict.get('import')))
    for _, obj in members:
        try:
            if issubclass(obj, SwitchBase) and dev_key == getattr(obj, 'key', ''):
                return obj(opt_dict)
        except TypeError:
            continue
    return ManualSwitch(direction, opt_dict)
