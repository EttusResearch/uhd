#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
UHD Power Cal: USRP Calbration Utility Objects
"""

import time
import inspect
import sys
import numpy
import uhd

from . import database
from .tone_gen import ToneGenerator

NUM_SAMPS_PER_EST = int(1e6)
# Limits for the power estimation algorithm. For good estimates, we want the
# signal to be at -6 dBFS, but not outside of an upper or lower limit.
PWR_EST_LLIM = -20
PWR_EST_IDEAL_LEVEL = -6
PWR_EST_ULIM = -3
SIGPWR_LOCK_MAX_ITER = 4

# The default distance between frequencies at which we measure
DEFAULT_FREQ_STEP = 10e6 # Hz
DEFAULT_SAMP_RATE = 5e6

def get_streamer(usrp, direction, chan):
    """
    Create an appropriate streamer object for this channel
    """
    stream_args = uhd.usrp.StreamArgs('fc32', 'sc16')
    stream_args.channels = [chan]
    return usrp.get_rx_stream(stream_args) if direction == 'rx' \
           else usrp.get_tx_stream(stream_args)

def get_default_gains(direction, gain_range, gain_step):
    """
    Create a equidistant gain range for calibration
    """
    assert direction in ('rx', 'tx')
    result = numpy.arange(0, gain_range.stop() + gain_step, gain_step)
    # reverse measurement points in RX so we can break cal loop once signal is too low
    if direction == 'rx':
        result = numpy.flip(result)
    return result

def get_usrp_power(streamer, num_samps=NUM_SAMPS_PER_EST, chan=0):
    """
    Return the measured input power in dBFS

    The return value is a list of dBFS power values, one per channel.
    """
    recv_buffer = numpy.zeros(
        (streamer.get_num_channels(), num_samps), dtype=numpy.complex64)
    metadata = uhd.types.RXMetadata()
    stream_cmd = uhd.types.StreamCMD(uhd.types.StreamMode.num_done)
    stream_cmd.num_samps = num_samps
    stream_cmd.stream_now = True
    streamer.issue_stream_cmd(stream_cmd)
    # Pass in long timeout, so we can rx the entire buffer in one go
    samps_recvd = streamer.recv(recv_buffer, metadata, 5.0)
    if samps_recvd != num_samps:
        raise RuntimeError(
            "ERROR! get_usrp_power(): Did not receive the correct number of samples!")
    return uhd.dsp.signals.get_power_dbfs(recv_buffer[chan])


def subtract_power(p1_db, p2_db):
    """
    Return the power of p1 subtracted from p2, where both are given in logarithmic
    units.
    """
    return 10 * numpy.log10(10**(p1_db/10) - 10**(p2_db/10))

###############################################################################
# Base Class
###############################################################################
class USRPCalibratorBase:
    """
    Base class for device calibration. Every USRP that can do power calibration
    needs to implement this.
    """
    # These should be overriden to device-specific values
    max_input_power = -20 # -20 happens to be a safe value for all devices
    min_detectable_signal = -70
    default_rate = DEFAULT_SAMP_RATE
    lo_offset = 0.0
    min_freq = None
    max_freq = None
    tune_settling_time = 0

    def __init__(self, usrp, meas_dev, direction, **kwargs):
        self._usrp = usrp
        self._meas_dev = meas_dev
        # This makes sure our measurement will not destroy the DUT
        self._meas_dev.max_output_power = self.max_input_power
        self._dir = direction
        self._desired_gain_step = kwargs.get('gain_step', 10)
        self._id = usrp.get_usrp_rx_info(0).get('mboard_id')
        self._mb_serial = usrp.get_usrp_rx_info(0).get('mboard_serial')
        # Littler helper to print stuff with a device ID prefix.
        self.log = lambda *args, **kwargs: print("[{}]".format(self._id), *args, **kwargs)
        # Channel, antenna, and streamer will get updated in update_port()
        self._chan = None
        self._ant = ""
        self._streamer = None
        # These dictionaries store the results that get written out as well as
        # the noise floor for reference
        self.results = {} # This must be of the form results[freq][gain] = power
        self._noise = {}
        # The tone generator object is only needed for Tx measurements, and is
        # initialized conditionaly in init()
        self._tone_gen = None
        # The gains can be overridden by the device if non-equidistant gains are
        # desired. However, gains must be increasing order for Tx measurements,
        # and in decreasing order for Rx measurements.
        self._gains = get_default_gains(
            direction,
            getattr(self._usrp, 'get_{}_gain_range'.format(self._dir))(),
            self._desired_gain_step)
        # You might want to override this, but it's not important. It will
        # become the 'name' argument for the power cal factory.
        self.cal_name = self._id + " Power Cal"
        # The child class can store temperature and ref gain here
        self.temp = None
        self.ref_gain = None

    def init(self, rate, tone_freq, amplitude):
        """
        Initialize device with finalized values. Not that __init__() needs to
        finish before the call site knows the rate, so we can't fold this into
        __init__().
        """
        if self._dir == 'tx':
            self._tone_gen = ToneGenerator(rate, tone_freq, amplitude)

    def update_port(self, chan, antenna):
        """
        Notify the device that we've switched channel and/or antenna.
        """
        self.log("Switching to channel {}, antenna {}.".format(chan, antenna))
        self._ant = antenna
        if chan != self._chan:
            # This will be an RX streamer for RX power cal, and a TX streamer
            # for TX power cal.
            self._streamer = get_streamer(self._usrp, self._dir, chan)
            if self._dir == 'tx':
                self._tone_gen.set_streamer(self._streamer)
        self._chan = chan

    def _get_frequencies(self, start_hint=None, stop_hint=None, step_hint=None):
        """
        Return an iterable of frequencies for testing.

        The default will check the hints against the device, but otherwise heed
        them.

        If a particular device needs to check specific frequencies, then
        override this.
        """
        step = step_hint or DEFAULT_FREQ_STEP
        start_min = self.min_freq or \
            getattr(self._usrp, 'get_{}_freq_range'.format(self._dir))(
                self._chan).start()
        if not start_hint and (start_min < step):
            start_min = step
        start_hint = start_hint or start_min
        start = max(start_hint, start_min)
        stop_max = self.max_freq or \
            getattr(self._usrp, 'get_{}_freq_range'.format(self._dir))(
                self._chan).stop()
        stop_hint = stop_hint or stop_max
        stop = min(stop_hint, stop_max)
        return numpy.arange(start, stop + step, step)

    def init_frequencies(self, start_hint, stop_hint, step_hint):
        """
        Return an iterable of frequencies for testing.

        The default will check the hints against the device, but otherwise heed
        them.

        Then it will measure the noise floor across frequency to get a good
        baseline measurement.
        """
        freqs = self._get_frequencies(start_hint, stop_hint, step_hint)
        if self._dir == 'tx':
            print("===== Measuring noise floor across frequency...")
            for freq in freqs:
                self._meas_dev.set_frequency(freq)
                self._noise[freq] = self._meas_dev.get_power()
                print("[TX] Noise floor: {:7.2f} MHz => {:+6.2f} dBm"
                      .format(freq/1e6, self._noise[freq]))
        else: # Rx
            print("===== Measuring noise floor across frequency and gain...")
            for freq in freqs:
                self._noise[freq] = {}
                tune_req = uhd.types.TuneRequest(freq)
                self._usrp.set_rx_freq(tune_req, self._chan)
                time.sleep(self.tune_settling_time)
                for gain in self._gains:
                    self._usrp.set_rx_gain(gain, self._chan)
                    self._noise[freq][gain] = get_usrp_power(self._streamer)
                    print("[RX] Noise floor: {:7.2f} MHz / {} dB => {:+6.2f} dBFS"
                          .format(freq/1e6, gain, self._noise[freq][gain]))
        return freqs

    def start(self):
        """
        Initialize the device for calibration
        """
        if self._dir == 'tx':
            self._tone_gen.start()
        else:
            self._meas_dev.enable(True)

    def stop(self, store=True):
        """
        Shut down the device after calibration
        """
        if self._dir == 'tx':
            self._tone_gen.stop()
        else:
            self._meas_dev.enable(False)
        if store:
            self.store()

    def run_rx_cal(self, freq):
        """
        Run the actual RX calibration for this frequency.
        """
        # Go to highest gain, lock in signal generator
        self._usrp.set_rx_gain(max(self._gains), self._chan)
        time.sleep(0.1) # Settling time of the USRP, highly conservative
        self.log("Locking in signal generator power...")
        self.log("Requesting input power: {:+.2f} dBm."
                 .format(self.min_detectable_signal))
        usrp_input_power = self._meas_dev.set_power(self.min_detectable_signal)
        recvd_power = get_usrp_power(self._streamer)
        self.log("Got input power: {:+.2f} dBm. Received power: {:.2f} dBFS. "
                 "Requesting new input power: {:+.2f} dBm."
                 .format(usrp_input_power,
                         recvd_power,
                         usrp_input_power + PWR_EST_IDEAL_LEVEL - recvd_power))
        usrp_input_power = self._meas_dev.set_power(
            usrp_input_power + PWR_EST_IDEAL_LEVEL - recvd_power)
        siggen_locked = False
        for _ in range(SIGPWR_LOCK_MAX_ITER):
            recvd_power = get_usrp_power(self._streamer)
            if PWR_EST_LLIM <= recvd_power <= PWR_EST_ULIM:
                siggen_locked = True
                break
            self.log("Receiving input power: {:+.2f} dBFS.".format(recvd_power))
            power_delta = PWR_EST_IDEAL_LEVEL - recvd_power
            # Update power output by the delta from the desired input value:
            self.log("Requesting input power: {:+.2f} dBm."
                     .format(usrp_input_power + power_delta))
            usrp_input_power = self._meas_dev.set_power(usrp_input_power + power_delta)
        if not siggen_locked:
            raise RuntimeError(
                "Unable to lock siggen within {} iterations! Last input power level: {:+6.2f} dBm."
                .format(SIGPWR_LOCK_MAX_ITER, usrp_input_power))
        self.log("Locked signal generator in at input power level: {:+6.2f} dBm."
                 .format(usrp_input_power))
        # Now iterate through gains
        results = {}
        # Gains are in decreasing order!
        last_gain = self._gains[0]
        for gain in self._gains:
            self._usrp.set_rx_gain(gain, self._chan) # Set the new gain
            self.log("Set gain to: {} dB. Got gain: {} dB."
                     .format(gain, self._usrp.get_rx_gain(self._chan)))
            time.sleep(0.1) # Settling time of the USRP, highly conservative
            gain_delta = last_gain - gain # This is our gain step
            if gain_delta:
                # If we decrease the device gain, we need to crank up the input
                # power
                usrp_input_power = self._meas_dev.set_power(
                    min(usrp_input_power + gain_delta, self.max_input_power))
                # usrp_input_power = self._meas_dev.set_power(usrp_input_power + gain_delta)
                self.log("New input power is: {:+.2f} dBm".format(usrp_input_power))
            recvd_power = get_usrp_power(self._streamer)
            self.log("Received power: {:.2f} dBFS".format(recvd_power))
            # It's possible that we lose the lock on the signal power, so allow
            # for a correction
            if not PWR_EST_LLIM <= recvd_power <= PWR_EST_ULIM:
                power_delta = PWR_EST_IDEAL_LEVEL - recvd_power
                self.log("Adapting input power to: {:+.2f} dBm."
                         .format(usrp_input_power + power_delta))
                usrp_input_power = self._meas_dev.set_power(usrp_input_power + power_delta)
                self.log("New input power is: {:+.2f} dBm".format(usrp_input_power))
                # And then of course, measure again
                recvd_power = get_usrp_power(self._streamer)
                self.log("Received power: {:.2f} dBFS".format(recvd_power))
            # Note: The noise power should be way down there, and really
            # shouldn't matter. We subtract it anyway for formal correctness.
            recvd_signal_power = subtract_power(recvd_power, self._noise[freq][gain])
            # A note on the following equation: 'recvd_signal_power' is in dBFS,
            # and usrp_input_power is in dBm. However, this is the reference
            # signal, so we need the power (in dBm) that corresponds to 0 dBFS.
            # The assumption is that digital gain is linear, so what we really
            # want is usrp_input_power - (recvd_signal_power - 0dBFS), and the
            # result of the equation is in dBm again. We omit the subtract-by-zero
            # since our variables don't have units.
            results[gain] = usrp_input_power - recvd_signal_power
            self.log(f"{gain:4.2f} dB => {results[gain]:+6.2f} dBm")
            # If we get too close to the noise floor, we stop
            if recvd_power - self._noise[freq][gain] <= 1.5:
                self.log("Can no longer detect input signal. Terminating.")
                break
            last_gain = gain
        self.results[freq] = results

    def run_tx_cal(self, freq):
        """
        Run the actual TX calibration for this frequency.
        """
        results = {}
        for gain in self._gains:
            self._usrp.set_tx_gain(gain, self._chan)
            time.sleep(0.1) # Settling time of the USRP, highly conservative
            results[gain] = self._meas_dev.get_power()
            self.log(f"{gain:4.2f} dB => {results[gain]:+6.2f} dBm")
        self.results[freq] = results

    def store(self):
        """
        Return the results object
        """
        chan_info = getattr(self._usrp, "get_usrp_{}_info".format(self._dir))(self._chan)
        cal_key = chan_info.get("{}_ref_power_key".format(self._dir))
        cal_serial = chan_info.get("{}_ref_power_serial".format(self._dir))
        cal_data = uhd.usrp.cal.PwrCal(self.cal_name, cal_serial, int(time.time()))
        if self.temp:
            cal_data.set_temperature(self.temp)
        if self.ref_gain:
            cal_data.set_ref_gain(self.ref_gain)
        for freq, results in self.results.items():
            max_power = max(results.values())
            min_power = min(results.values())
            cal_data.add_power_table(results, min_power, max_power, freq)
        database.write_cal_data(
            cal_key,
            cal_serial,
            cal_data.serialize())
        self.results = {}

class B200Calibrator(USRPCalibratorBase):
    """
    B200 calibration
    """
    mboard_ids = ('B200', 'B210', 'B200mini', 'B205mini')
    # Choosing 5 MHz: It is a small rate, but carries enough bandwidth to receive
    # a tone. By default, the auto MCR will kick in and set the MCR to 40 Msps,
    # thus engaging all halfbands.
    default_rate = 5e6
    # Choosing an LO offset of 10 MHz: At 5 Msps, the LO will never be within
    # our estimate. B200 generally has good DC offset / IQ balance performance,
    # but we still try and avoid DC as much as possible.
    lo_offset = 10e6

    def __init__(self, usrp, meas_dev, direction, **kwargs):
        super().__init__(usrp, meas_dev, direction, **kwargs)
        print("===== Reading temperature... ", end="")
        self.temp = self._usrp.get_rx_sensor("temp").to_int()
        print("{} C".format(self.temp))
        # TODO don't hard code
        self.ref_gain = 60

class X300Calibrator(USRPCalibratorBase):
    """
    X300/X310 calibration

    Notes / TODOs:
    - The X310 must avoid frequencies that are multiples of 200 MHz; here be
      harmonics (TODO)
    - TwinRX needs its own special method. It needs to be run once with one
      channel in the streamer, and once with two channels in the streamer. Or we
      come up with a clever way of enabling the other channel without modifying
      the streamer. A poke to the prop tree might do the trick.
    """
    mboard_ids = ('X300', 'X310', 'NI-2974')
    # Choosing 5 MHz: It is a small rate, but carries enough bandwidth to receive
    # a tone. It's 1/40 the master clock rate, which means it'll engage max
    # halfbands.
    default_rate = 5e6
    # Choosing an LO offset of 10 MHz: At 5 Msps, the LO will never be within
    # our estimate, so it doesn't matter if this device is DC offset / IQ balance
    # calibrated.
    lo_offset = 10e6

class X410Calibrator(USRPCalibratorBase):
    """
    X410/ZBX Calibration
    """
    mboard_ids = ('x410',)
    # Choosing 3.84 MHz: It is a small rate, but carries enough bandwidth to
    # receive a tone. It's 1/40 the default master clock rate (122.88e6), which
    # means it'll engage max halfbands.
    default_rate = 3.84e6
    min_freq = 1e6
    max_freq = 8e9
    # X410 non-timed tunes are currently very poke-intensive, so we give it some
    # time to clear the command queue
    tune_settling_time = .5

class E3XXCalibrator(USRPCalibratorBase):
    """
    E3XX calibration
    """
    mboard_ids = ('e320', 'e310', 'e31x', 'e3xx')
    # Choosing 4 MHz: It is a small rate, but carries enough bandwidth to receive
    # a tone. Compatible with the default MCR of 16 MHz.
    default_rate = 4e6
    # Choosing an LO offset of 8 MHz: At 4 Msps, the LO will never be within
    # our estimate. E3XX generally has good DC offset / IQ balance performance,
    # but we still try and avoid DC as much as possible.
    lo_offset = 8e6
    tune_settling_time = .01 # Probably not needed??

    def __init__(self, usrp, meas_dev, direction, **kwargs):
        super().__init__(usrp, meas_dev, direction, **kwargs)
        self._start_temp = None
        self._stop_temp = None

    def start(self):
        # query and remember starting temperature
        get_sensor_fn = getattr(self._usrp, 'get_{}_sensor'.format(self._dir))
        self._start_temp = get_sensor_fn("ad9361_temperature").to_real()
        super().start()

    def stop(self, store=True):
        # save average temperature over the calibration cycle
        get_sensor_fn = getattr(self._usrp, 'get_{}_sensor'.format(self._dir))
        self._stop_temp = get_sensor_fn("ad9361_temperature").to_real()
        print("===== Temperature range was {:.1f} to {:.1f} C"
              .format(self._start_temp, self._stop_temp))
        self.temp = round((self._start_temp + self._stop_temp) / 2)
        super().stop(store)


###############################################################################
# The dispatch function
###############################################################################
def get_usrp_calibrator(usrp, meas_dev, direction, **kwargs):
    """
    Return a USRP calibrator object.
    """
    usrp_type = \
        getattr(usrp, 'get_usrp_{}_info'.format(direction))().get('mboard_id')
    if usrp_type is None:
        raise RuntimeError("Could not determine USRP type!")
    print("=== Detected USRP type:", usrp_type)
    for _, obj in inspect.getmembers(sys.modules[__name__]):
        if (inspect.isclass(obj) and
            issubclass(obj, USRPCalibratorBase) and
            usrp_type in getattr(obj, 'mboard_ids', '')):
            return obj(usrp, meas_dev, direction, **kwargs)
    raise RuntimeError("No USRP calibrator object found for device type: {}"
                       .format(usrp_type))
