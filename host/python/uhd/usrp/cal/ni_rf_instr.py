#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
RFSA/RFSG-type measurement devices
"""
import os
import platform
if platform.system() == 'Windows':
    from ctypes import WinError, CDLL, byref, POINTER
    from ctypes import c_bool, c_int, c_int32, c_double, c_char_p

# pylint: disable=unused-argument
def _check(result, func, arguments):
    """
    Test result for error. Throw WinError, if result is not 0 using result as
    error index. This is a convenient default implementation for WIN API calls
    """
    assert os.name == "nt" # throwing WinError works on Windows only.
    if result != 0:
        error = WinError(result,
                         "External library call failed with {}".format(result))
        raise error

def _wrap(lib, func_name, res_type, arg_types, check_error=True):
    """
    Ease usage of API functions imported using ctypes library.
    :param lib: CDLL loaded by ctypes
    :param func_name: Function import to uses
    :param res_type: result type of call (as ctype)
    :param arg_types: argument list of call (as ctype)
    :param check_error: error checker (default uses _check)
    :return: Python function
    """
    func = lib.__getattr__(func_name)
    func.restype = res_type
    func.argtypes = arg_types
    if check_error:
        func.errcheck = _check
    return func

def get_modinst_devices(driver):
    """
    Creates a list of instruments matching the current driver. Set
    self._driver before calling this method
    :return: list of nimodinst device objects
    :throws: RuntimeError if called on non-Windows systems.
    """
    if platform.system() != 'Windows':
        raise RuntimeError('Cannot connect to ModInst '
                           'devices on non-Windows systems.')
    # make sure NI ModInst package is only loaded when required
    # pylint: disable=import-outside-toplevel
    import nimodinst
    session = nimodinst.Session(driver)
    return session.devices

def get_modinst_device(driver, name):
    """
    Get exact on device for the current driver. Use (optional) name to
    narrow the list if more than one device is installed for the current
    driver.
    :param name: name of devices to get
    :return: NI ModInst device class
    :throws: RuntimeError if no or more than one device was found.
    """
    devices = [device for device in get_modinst_devices(driver)
               if not name or name == device.device_name]
    if len(devices) > 1:
        print("Found %s devices:" % driver)
        for device in devices:
            print("* %s" % device.device_name)
        raise RuntimeError("Found more than one measurement device. "
                           "Please limit the query!")
    if not devices:
        raise RuntimeError("No measurement device found!")
    return devices[0]


class RFSADevice:
    """
    Class to measure power using devices from the NI-RFSA family. This class
    make use of the RFmx SpecAn TXP measurement. So this library must be
    installed as well in addition to the RFSA driver.
    """
    RFMXSPECAN_ATTR_REFERENCE_LEVEL = 0x00100002

    RFMXSPECAN_VAL_TXP_RBW_FILTER_TYPE_GAUSSIAN = 1
    RFMXSPECAN_VAL_TXP_AVERAGING_ENABLED_TRUE = 1
    RFMXSPECAN_VAL_TXP_AVERAGING_TYPE_RMS = 0

    def __init__(self, options):
        self.session = c_int(0)

        self._generate_specan_calls()

        device = get_modinst_device("NI-RFSA", options.get("name", None))

        self._RFmxSpecAn_Initialize(device.device_name.encode(),
                                    "".encode(), byref(self.session), 0)

    def __del__(self):
        if self.session.value > 0:
            self._RFmxSpecAn_Close(self.session, 0)

    def _generate_specan_calls(self):
        """
        Generate needed Python function in class to do power measurements.
        """
        lib = CDLL("niRFmxSpecAn.dll")
        # disable invalid name error to use function names as declared in C-DLL
        # pylint: disable=invalid-name
        self._RFmxSpecAn_Initialize = _wrap(
            lib,
            "RFmxSpecAn_Initialize",
            c_int,
            [c_char_p, c_char_p, POINTER(c_int), c_int]
        )
        self._RFmxSpecAn_Close = _wrap(
            lib,
            "RFmxSpecAn_Close",
            c_int,
            [c_int, c_int]
        )
        self._RFmxSpecAn_SetSelectedPorts = _wrap(
            lib,
            "RFmxSpecAn_SetSelectedPorts",
            c_int,
            [c_int, c_char_p, c_char_p]
        )
        self._RFmxSpecAn_CfgRF = _wrap(
            lib,
            "RFmxSpecAn_CfgRF",
            c_int,
            [c_int, c_char_p, c_double, c_double, c_double]
        )
        self._RFmxSpecAn_TXPCfgRBWFilter = _wrap(
            lib,
            "RFmxSpecAn_TXPCfgRBWFilter",
            c_int,
            [c_int, c_char_p, c_double, c_int32, c_double]
        )
        self._RFmxSpecAn_TXPCfgMeasurementInterval = _wrap(
            lib,
            "RFmxSpecAn_TXPCfgMeasurementInterval",
            c_int32,
            [c_int, c_char_p, c_double]
        )
        self._RFmxSpecAn_TXPCfgAveraging = _wrap(
            lib,
            "RFmxSpecAn_TXPCfgAveraging",
            c_int32,
            [c_int, c_char_p, c_int32, c_int32, c_int32,]
        )
        self._RFmxSpecAn_TXPRead = _wrap(
            lib,
            "RFmxSpecAn_TXPRead",
            c_int,
            [c_int, c_char_p, c_double, POINTER(c_double),
             POINTER(c_double), POINTER(c_double), POINTER(c_double)]
        )
        self._RFmxSpecAn_GetAttributeF64 = _wrap(
            lib,
            "RFmxSpecAn_GetAttributeF64",
            c_int,
            [c_int, c_char_p, c_int, POINTER(c_double)]
        )
        self._RFmxSpecAn_SetAttributeF64 = _wrap(
            lib,
            "RFmxSpecAn_SetAttributeF64",
            c_int,
            [c_int, c_char_p, c_int, c_double]
        )


    def init_power_meter(self):
        """
        Initialize power meter (derived from "RFmxSpecAn TXP(Basic)" example)
        """
        rbw = c_double(100E3)
        filt = c_int32(RFSADevice.RFMXSPECAN_VAL_TXP_RBW_FILTER_TYPE_GAUSSIAN)
        rrc_alpha = c_double(0)
        meas_int = c_double(1E-3)
        avg = c_int32(RFSADevice.RFMXSPECAN_VAL_TXP_AVERAGING_ENABLED_TRUE)
        avg_count = c_int32(10)
        avg_type = c_int32(RFSADevice.RFMXSPECAN_VAL_TXP_AVERAGING_TYPE_RMS)
        self._RFmxSpecAn_SetSelectedPorts(self.session, b"", b"")
        self._RFmxSpecAn_TXPCfgRBWFilter(self.session, b"", rbw, filt, rrc_alpha)
        self._RFmxSpecAn_TXPCfgMeasurementInterval(self.session, b"", meas_int)
        self._RFmxSpecAn_TXPCfgAveraging(self.session, b"", avg, avg_count, avg_type)

    def set_frequency(self, freq):
        """
        Set measurement frequency and reset ref level.
        """
        freq = c_double(freq)
        ref_level = c_double(-20) # start with a low ref level
        ext_att = c_double(0) # calibration script takes responsibility for
                              # external attenuation, so no need to set it here
        self._RFmxSpecAn_CfgRF(self.session, b"", freq, ref_level, ext_att)

    def _get_attribute(self, attr_id):
        """
        Wrapper to ease read RFSA double attributes.
        """
        result = c_double(0)
        self._RFmxSpecAn_GetAttributeF64(self.session, b"", attr_id, byref(result))
        return result.value

    def _set_attribute(self, attr_id, value):
        """
        Wrapper to ease write RFSA double attributes.
        """
        attr = c_double(value)
        self._RFmxSpecAn_SetAttributeF64(self.session, b"", attr_id, attr)

    def get_reference_level(self):
        """
        Return reference level used for measurements
        """
        return self._get_attribute(RFSADevice.RFMXSPECAN_ATTR_REFERENCE_LEVEL)

    def set_reference_level(self, value):
        """
        Set reference level used for measurements
        """
        self._set_attribute(RFSADevice.RFMXSPECAN_ATTR_REFERENCE_LEVEL, value)

    def get_power_dbm(self):
        """
        Measure power at current frequency. Reference level starts at -20dBm
        (see set_frequency) and is increased in 5dB steps until measurement
        succeeds. Returns the measured averaged power.
        """
        timeout = c_double(1.0)
        mean_val = c_double(0)
        peak2avg_val = c_double(0)
        min_val = c_double(0)
        max_val = c_double(0)
        while True:
            try:
                self._RFmxSpecAn_TXPRead(self.session, b"", timeout,
                                         byref(mean_val), byref(peak2avg_val),
                                         byref(min_val), byref(max_val))
                return mean_val.value
            except OSError as ex:
                # increase ref level on ADC or DSA overload
                if getattr(ex, 'winerror') in [
                    0x0005B10A, # DSP overflow (RFSA)
                    0x0005B10B, # ADC overload (RFSA)
                    0x3FFA9001, # ADC overload (mxl)
                    ]:
                    self.set_reference_level(self.get_reference_level() + 5)
                else:
                    raise ex


class RFSGDevice:
    """
    Class to generate CW tone using devices from the NI-RFSG family
    """
    NIRFSG_ATTR_POWER_LEVEL = 1250002
    NIRFSG_VAL_CW = 1000

    def __init__(self, options):
        super().__init__()
        self.session = c_int(0)

        self._generate_rfsg_calls()

        device = get_modinst_device("NI-RFSG", options.get("name", None))

        id_query = c_bool(False)
        reset = c_bool(True)
        self._niRFSG_init(device.device_name.encode(), id_query, reset,
                          byref(self.session))
        self._niRFSG_ConfigureGenerationMode(self.session,
                                             RFSGDevice.NIRFSG_VAL_CW)

    def __del__(self):
        if self.session.value > 0:
            self._niRFSG_close(self.session, 0)

    def _generate_rfsg_calls(self):
        """
        Generate needed Python function in class to do power measurements.
        For RFSGs the library has different names depending on the platform
        used. So platform is checked to select proper library name.
        """
        lib_name = "niRFSG_64.dll" if platform.architecture()[0] == '64bit' \
              else "niRFSG.dll"
        lib = CDLL(lib_name)
        # disable invalid name error to use function names as declared in C-DLL
        # pylint: disable=invalid-name
        self._niRFSG_init = _wrap(
            lib,
            "niRFSG_init",
            c_int,
            [c_char_p, c_bool, c_bool, POINTER(c_int)]
        )
        self._niRFSG_close = _wrap(
            lib,
            "niRFSG_close",
            c_int,
            [c_int, c_int]
        )

        self._niRFSG_SetAttributeViReal64 = _wrap(
            lib,
            "niRFSG_SetAttributeViReal64",
            c_int,
            [c_int, c_char_p, c_int, c_double]
        )

        self._niRFSG_GetAttributeViReal64 = _wrap(
            lib,
            "niRFSG_GetAttributeViReal64",
            c_int,
            [c_int, c_char_p, c_int, POINTER(c_double)]
        )

        self._niRFSG_ConfigureRF = _wrap(
            lib,
            "niRFSG_ConfigureRF",
            c_int,
            [c_int, c_double, c_double]
        )

        self._niRFSG_ConfigureGenerationMode = _wrap(
            lib,
            "niRFSG_ConfigureGenerationMode",
            c_int,
            [c_int, c_int]
        )

        self._niRFSG_Initiate = _wrap(
            lib,
            "niRFSG_Initiate",
            c_int,
            [c_int]
        )

        self._niRFSG_Abort = _wrap(
            lib,
            "niRFSG_Abort",
            c_int,
            [c_int]
        )

    def enable(self, enb=True):
        """
        Switch tone generation on of off
        """
        if enb:
            print("[SigGen] Starting tone generator.")
            self._niRFSG_Initiate(self.session)
        else:
            print("[SigGen] Stopping tone generator.")
            self._niRFSG_Abort(self.session)

    def set_frequency(self, freq):
        """
        Tune tone generator to new center frequency.
        """
        print("[SigGen] Tuning signal to {:.3f} MHz.".format(freq/1e6))
        self._niRFSG_ConfigureRF(self.session, freq, self.get_power())

    def _get_attribute(self, attr_id):
        """
        Wrapper to ease reading RFSG double attributes.
        """
        result = c_double(0)
        self._niRFSG_GetAttributeViReal64(self.session, b"", attr_id, byref(result))
        return result.value

    def _set_attribute(self, attr_id, value):
        """
        Wrapper to ease writing RFSG double attributes.
        """
        attr = c_double(value)
        self._niRFSG_SetAttributeViReal64(self.session, b"", attr_id, attr)

    def set_power(self, power_dbm):
        """
        Set power of output signal and return actual power.
        """
        self._set_attribute(RFSGDevice.NIRFSG_ATTR_POWER_LEVEL, power_dbm)
        return self.get_power()

    def get_power(self):
        """
        Get power of output signal.
        """
        return self._get_attribute(RFSGDevice.NIRFSG_ATTR_POWER_LEVEL)
