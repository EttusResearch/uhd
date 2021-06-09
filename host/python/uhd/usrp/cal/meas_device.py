#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Measurement Device Class for UHD Power Calibration
"""

import sys
import time
import inspect
import importlib
import numpy
import uhd
from .tone_gen import ToneGenerator
from .visa import get_visa_device
from .ni_rf_instr import RFSADevice, RFSGDevice

###############################################################################
# Base Classes
###############################################################################
class PowerMeterBase:
    """
    Base class for measuring output power (Tx) of the USRP. That means the
    measurement device is receiving and the USRP (the DUT) is transmitting.
    """
    def __init__(self, options):
        self._options = options
        self.power_offset = 0

    def set_frequency(self, freq):
        """
        Set the frequency of the measurement device.
        """
        raise NotImplementedError()

    def get_power(self):
        """
        Return the current measured power in dBm.
        """
        return self._get_power() + self.power_offset

    def _get_power(self):
        """
        Return the current measured power in dBm.
        """
        raise NotImplementedError()

class SignalGeneratorBase:
    """
    Base class for measuring input power (Rx) of the USRP. That means the
    measurement device is transmitting and the USRP (the DUT) is receiving.
    """
    def __init__(self, options):
        self._options = options
        self.power_offset = 0
        # Make sure to set this before doing RX cal
        self.max_output_power = None

    def enable(self, enable=True):
        """
        Turn on the power generator. By default, it should be off, and only
        produce a signal when this was called with an argument value of 'True'.
        """
        raise NotImplementedError()

    def set_power(self, power_dbm):
        """
        Set the input power of the DUT. This will factor in the power offset,
        and set the measurement device to produce the power that will cause the
        DUT to receive power_dbm.

        This will coerce to the next possible power available and return the
        coerced value.
        """
        assert self.max_output_power is not None
        if power_dbm > self.max_output_power:
            print("[SigGen] WARNING! Trying to set power beyond safe levels. "
                  "Capping output power at {} dBm.".format(self.max_output_power))
            power_dbm = self.max_output_power
        return self._set_power(power_dbm + self.power_offset) - self.power_offset

    def get_power(self):
        """
        Return the input power of the DUT.  This will factor in the power offset,
        and will return the power level in dBm that is going into the DUT.
        Use this with set_power(), as not all power levels can be reached.
        """
        return self._get_power() - self.power_offset

    def set_frequency(self, freq):
        """
        Set the center frequency of the generated signal.
        """
        raise NotImplementedError()

    def _set_power(self, power_dbm):
        """
        Set the output power of the device in dBm.
        """
        raise NotImplementedError()

    def _get_power(self):
        """
        Return the output power of the measurement device.
        """
        raise NotImplementedError()

###############################################################################
# Manual Measurement: For masochists, or for small sample sets
###############################################################################
class ManualPowerMeter(PowerMeterBase):
    """
    Manual measurement: The script does nothing, it just asks the user to
    manually make changes and return values
    """
    key = 'manual'

    def set_frequency(self, freq):
        """
        Ask user to set frequency
        """
        input("[TX] Set your power meter to following frequency: "
              "{:.3f} MHz, then hit Enter.".format(freq/1e6))

    def _get_power(self):
        """
        Ask user for the power
        """
        num_tries = 5
        for _ in range(num_tries):
            try:
                return float(input("[TX] Please enter the measured power in dBm: ")) \
                       + self.power_offset
            except ValueError:
                continue
        raise ValueError("Invalid power value entered.")

class ManualPowerGenerator(SignalGeneratorBase):
    """
    Manual measurement: The script does nothing, it just asks the user to
    manually make changes and return values
    """
    key = 'manual'
    num_tries = 5

    def enable(self, enable=True):
        """
        Ask the user to turn the device on or off
        """
        input("[RX] Please {} your signal generator and hit Enter."
              .format("enable" if enable else "disable"))

    def _set_power(self, power_dbm):
        """
        Ask for a power, or the closest, and return that
        """
        new_power = input(
            "[RX] Set your signal generator to following output power: "
            "{:.2f} dBm, then hit Enter, or enter the closest available power: "
            .format(power_dbm))
        if not new_power:
            return power_dbm
        for _ in range(self.num_tries):
            try:
                return float(new_power)
            except ValueError:
                new_power = input(
                    "[RX] Set your signal generator to following output power: "
                    "{:.2f} dBm, then hit Enter, or enter the closest available power: "
                    .format(power_dbm))
                if not new_power:
                    return power_dbm
        raise ValueError("Invalid power value entered.")

    def _get_power(self):
        """
        Ask user for current power
        """
        for _ in range(self.num_tries):
            try:
                return float(input(
                    "[RX] Please enter the output power in dBm of your "
                    "signal generator: "))
            except ValueError:
                continue
        raise ValueError("Invalid power value entered.")

    # pylint: disable=no-self-use
    def set_frequency(self, freq):
        """
        Set the center frequency of the generated signal.
        """
        input("[RX] Set your signal generator to following frequency: {:.3f} MHz, then hit Enter."
              .format(freq/1e6))
    # pylint: enable=no-self-use

##############################################################################
# RFSA: Run through a NI-RFSA device, using RFmx library
###############################################################################
class RfsaPowerMeter(PowerMeterBase):
    """
    Power meter using RFmx TXP measurement on NI-RFSA devices.
    """
    key = 'rfsa'

    def __init__(self, options):
        super().__init__(options)
        self.device = RFSADevice(options)

    def set_frequency(self, freq):
        """
        Set the frequency of the measurement device.
        """
        self.device.set_frequency(freq)

    def _get_power(self):
        """
        Return the current measured power in dBm.
        """
        return self.device.get_power_dbm()

##############################################################################
# VISA: Run through a VISA device, using SCPI commands
###############################################################################
class VisaPowerMeter(PowerMeterBase):
    """
    VISA based Tx measurement device
    """
    DEFAULT_VISA_LIB = '@py' # pyvisa-py
    DEFAULT_VISA_QUERY = "?*::INSTR"

    key = 'visa'

    def __init__(self, options):
        super().__init__(options)
        # pylint: disable=import-outside-toplevel
        # We disable this warning because having pyvisa installed is not a
        # requirement, so we want to load it as late as possible, and only when
        # needed.
        import pyvisa
        # pylint: enable=import-outside-toplevel
        visa_lib = options.get('visa_lib', self.DEFAULT_VISA_LIB)
        visa_query = options.get('visa_query', self.DEFAULT_VISA_QUERY)
        self._rm = pyvisa.ResourceManager(visa_lib)
        resources = self._rm.list_resources(visa_query)
        if len(resources) > 1:
            print("Found VISA devices:")
            for resource in resources:
                print("*" + resource)
            raise RuntimeError(
                "Found more than one measurement device. Please limit the query!")
        if len(resources) == 0:
            raise RuntimeError("No measurement device found!")
        self._res = self._rm.open_resource(resources[0])
        self.visa = get_visa_device(self._res, resources[0], options)
        self.visa.init_power_meter()

    def set_frequency(self, freq):
        """
        Set frequency
        """
        self.visa.set_frequency(freq)

    def _get_power(self):
        """
        Get power
        """
        return self.visa.get_power_dbm()

###############################################################################
# USRP: Use a pre-calibrated USRP as a measurement device
###############################################################################
class USRPPowerGenerator(SignalGeneratorBase):
    """
    The power generator is actually a USRP. This only works if the USRP that is
    used for power/signal generation has been previously calbrated itself.
    """
    key = 'usrp'

    def __init__(self, options):
        super().__init__(options)
        usrp_args = options.get('args')
        if not usrp_args:
            raise RuntimeError(
                "Must specify args for USRP measurement device!")
        self._usrp = uhd.usrp.MultiUSRP(usrp_args)
        self._rate = float(options.get('rate', 5e6))
        self._lo_offset = float(options.get('lo_offset', 0))
        self._chan = int(options.get('chan', 0))
        self._amplitude = float(options.get('ampl', 1/numpy.sqrt(2)))
        self._pwr_dbfs = 20 * numpy.log10(self._amplitude)
        self._tone_freq = 0
        stream_args = uhd.usrp.StreamArgs('fc32', 'sc16')
        stream_args.channels = [self._chan]
        self._streamer = self._usrp.get_tx_stream(stream_args)
        print("==== Creating USRP tone generator. Power offset:", self.power_offset)
        self._tone_gen = ToneGenerator(self._rate, self._tone_freq, self._amplitude)
        self._tone_gen.set_streamer(self._streamer)

    def enable(self, enable=True):
        """
        Turn the tone generator on or off.
        """
        if enable:
            print("[SigGen] Starting tone generator.")
            self._tone_gen.start()
        else:
            print("[SigGen] Stopping tone generator.")
            self._tone_gen.stop()
        time.sleep(0.1) # Give it some time to spin down

    def set_frequency(self, freq):
        """
        Set the center frequency of the generated signal.
        """
        print("[SigGen] Channel {}: Tuning signal to {:.3f} MHz."
              .format(self._chan, freq/1e6))
        tune_req = uhd.types.TuneRequest(freq, self._lo_offset)
        self._usrp.set_tx_freq(tune_req, self._chan)

    def _set_power(self, power_dbm):
        """
        Set the output power of the device in dBm.
        """
        self._usrp.set_tx_power_reference(power_dbm - self._pwr_dbfs, self._chan)
        return self._get_power()

    def _get_power(self):
        """
        Return the output power of the measurement device.
        """
        return self._usrp.get_tx_power_reference(self._chan) + self._pwr_dbfs

###############################################################################
# RFSG: NI signal generator family
###############################################################################
class RFSGPowerGenerator(SignalGeneratorBase):
    """
    Power Generator using NI-RFSG devices.
    """
    key = 'rfsg'

    def __init__(self, options):
        super().__init__(options)
        self.device = RFSGDevice(options)

    def enable(self, enable=True):
        """
        Turn tone generation on and off
        """
        self.device.enable(enable)

    def set_frequency(self, freq):
        """
        Set the center frequency of the generated signal.
        """
        self.device.set_frequency(freq)

    def _set_power(self, power_dbm):
        """
        Set the output power of the device in dBm.
        """
        return self.device.set_power(power_dbm)

    def _get_power(self):
        """
        Get the output power of the device in dBm.
        """
        return self.device.get_power()

###############################################################################
# The dispatch function
###############################################################################
def get_meas_device(direction, dev_key, options):
    """
    Return the measurement device object
    """
    assert direction in ('tx', 'rx')
    base_class = SignalGeneratorBase if direction == 'rx' else PowerMeterBase
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
            if issubclass(obj, base_class) and dev_key == getattr(obj, 'key', ''):
                return obj(opt_dict)
        except TypeError:
            continue
    raise RuntimeError("No {} found for key: {}".format(
        "signal generator" if direction == "rx" else "power meter", dev_key))
