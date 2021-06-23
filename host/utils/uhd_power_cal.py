#!/usr/bin/env python3
#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Utility to run power calibrations with USRPs
"""

import sys
import time
import math
import pickle
import argparse
import uhd

def parse_args():
    """ Parse args and return args object """
    parser = argparse.ArgumentParser(
        description="Run power level calibration for supported USRPs",
    )
    parser.add_argument(
        '--args', default="",
        help="USRP Device Args",
    )
    parser.add_argument(
        '-d', '--dir', default='tx',
        help="Direction: Must be either rx or tx. From the perspective of the "
             "device: rx == receive power levels, tx == transmit power levels.",
        choices=['tx', 'rx'])
    parser.add_argument(
        '--start',
        help='Start Frequency. Defaults to lowest available frequency on device. '
             'Note that this is only a hint for the device object, which can choose '
             'to override this value.', type=float)
    parser.add_argument(
        '--stop',
        help='Stop Frequency. Defaults to highest available frequency on device. '
             'Note that this is only a hint for the device object, which can choose '
             'to override this value.', type=float)
    parser.add_argument(
        '--step',
        help='Frequency Step. Defaults are device-specific. '
             'Note that this is only a hint for the device object, which can choose '
             'to override this value. Devices can also measure at non-regular '
             'frequencies, e.g., to more accurately cover differences between bands.',
        type=float)
    parser.add_argument(
        '--gain-step', type=float, default=1,
        help='Gain Step (dB). Defaults to 1 dB. '
             'Note that this is only a hint for the device object, which can choose '
             'to override this value. Devices can also measure at non-regular '
             'gain intervals.')
    parser.add_argument(
        '--lo-offset', type=float,
        help='LO Offset. This gets applied to every tune request. Note that for '
             'TX measurements, there is also an offset applied by --tone-freq. '
             'The default value is device-dependent.')
    parser.add_argument(
        '--amplitude', type=float, default=1./math.sqrt(2),
        help='Amplitude of the tone that is generated for tx measurements. '
             'Default is 1/sqrt(2), or -3 dBFS.')
    parser.add_argument(
        '--attenuation', type=float, default=0.0,
        help='Amount of attenuation between measurement device and DUT. This will '
             'be accounted for by simple addition, it is treated like a measurement error. '
             'The argument is generally positive, e.g. 30 means 30 dB of attenuation.')
    parser.add_argument(
        '--tone-freq', type=float, default=1e6,
        help='Frequency of the tone that is generated for Tx measurements. This '
             'has the same effect as setting an LO offset, except in software.')
    parser.add_argument(
        '--antenna', default="*",
        help="Select antenna port. A value of '*' means that the calibration "
             "will be repeated on all appropriate antenna ports.")
    parser.add_argument(
        '--channels', default="*",
        help="Select channel. A value of '*' means that the calibration "
             "will be repeated on all appropriate channels.")
    parser.add_argument(
        '--meas-dev', default='manual',
        help='Type of measurement device that is used')
    parser.add_argument(
        '-o', '--meas-option', default=[], action='append',
        help='Options that are passed to the measurement device')
    parser.add_argument(
        '--switch', default='manual',
        help='Type of switch to be used to connect antennas')
    parser.add_argument(
        '--switch-option', default=[], action='append',
        help='Options that are passed to the switch')
    parser.add_argument(
        '-r', '--rate', type=float,
        help='Sampling rate at which the calibration is performed')
    parser.add_argument(
        '--store', metavar='filename.pickle',
        help='If provided, will store intermediate cal data. This can be analyzed '
        'separately, or loaded into the tool with --load.')
    parser.add_argument(
        '--load', metavar='filename.pickle',
        help='If provided, will load intermediate cal data instead of running a '
        'measurement.')
    return parser.parse_args()


def sanitize_args(usrp, args, default_rate):
    """
    Check the args against the USRP object.
    """
    assert usrp.get_num_mboards() == 1, \
        "Power calibration tools are designed for a single motherboard!"
    available_chans = getattr(usrp, 'get_{}_num_channels'.format(args.dir))()
    if args.channels == '*':
        # * means all channels
        channels = list(range(available_chans))
    else:
        try:
            channels = [int(c) for c in args.channels.split(',')]
        except ValueError:
            raise ValueError("Invalid channel list: {}".format(args.channels))
        for chan in channels:
            assert chan in range(available_chans), \
                "ERROR: Invalid channel: {}. Should be in 0..{}.".format(
                    chan, available_chans)
    print("=== Calibrating for channels:", ", ".join([str(x) for x in channels]))
    available_ants = getattr(usrp, 'get_{}_antennas'.format(args.dir))()
    if args.antenna == '*':
        invalid_antennas = ('CAL', 'LOCAL', 'CAL_LOOPBACK', 'TERMINATION')
        antennas = [x for x in available_ants if x not in invalid_antennas]
    else:
        try:
            antennas = args.antenna.split(',')
        except ValueError:
            raise ValueError("Invalid antenna list: {}".format(args.antenna))
        for ant in antennas:
            assert ant in available_ants, \
                "Invalid antenna: {}. Should be in {}.".format(
                    ant, available_ants)
    print("=== Calibrating for antennas:", ", ".join([str(x) for x in antennas]))
    rate = args.rate or default_rate
    getattr(usrp, 'set_{}_rate'.format(args.dir))(rate)
    actual_rate = getattr(usrp, 'get_{}_rate'.format(args.dir))()
    print("=== Requested sampling rate: {} Msps, actual rate: {} Msps"
          .format(rate/1e6, actual_rate/1e6))
    if args.dir == 'tx' and abs(args.tone_freq) > actual_rate:
        raise ValueError(
            "The TX tone frequency offset of {} kHz is greater than the sampling rate."
            .format(args.tone_freq / 1e3))
    return channels, antennas, actual_rate


def init_results(pickle_file):
    """
    Initialize results from pickle file, or empty dict
    """
    if pickle_file is None:
        return {}
    with open(pickle_file, 'rb') as results_file:
        return pickle.load(results_file)

class CalRunner:
    """
    Executor of the calibration routines.
    """
    def __init__(self, usrp, usrp_cal, meas_dev, args):
        self.usrp = usrp
        self.usrp_cal = usrp_cal
        self.meas_dev = meas_dev
        self.dir = args.dir
        self.tone_offset = args.tone_freq if args.dir == 'tx' else 0.0
        self.lo_offset = args.lo_offset if args.lo_offset else usrp_cal.lo_offset
        if self.lo_offset:
            print("=== Using USRP LO offset: {:.2f} MHz"
                  .format(self.lo_offset / 1e6))

    def run(self, chan, freq):
        """
        Run all cal steps for a single frequency
        """
        print("=== Running calibration at frequency {:.3f} MHz...".format(freq / 1e6))
        tune_req = uhd.types.TuneRequest(freq, self.lo_offset)
        getattr(self.usrp, 'set_{}_freq'.format(self.dir))(tune_req, chan)
        time.sleep(self.usrp_cal.tune_settling_time)
        actual_freq = getattr(self.usrp, 'get_{}_freq'.format(self.dir))(chan)
        if abs(actual_freq - freq) > 1.0:
            print("WARNING: Frequency was coerced from {:.2f} MHz to {:.2f} MHz!"
                  .format(freq / 1e6, actual_freq / 1e6))
        self.meas_dev.set_frequency(actual_freq + self.tone_offset)
        getattr(self.usrp_cal, 'run_{}_cal'.format(self.dir))(freq)

def main():
    """Go, go, go!"""
    args = parse_args()
    print("=== Detecting USRP...")
    usrp = uhd.usrp.MultiUSRP(args.args)
    print("=== Measurement direction:", args.dir)
    print("=== Initializing measurement device...")
    meas_dev = uhd.usrp.cal.get_meas_device(args.dir, args.meas_dev, args.meas_option)
    meas_dev.power_offset = args.attenuation
    # If we're transmitting, then we need to factor in the "attenuation" from us
    # not transmitting at full scale
    if args.dir == 'tx':
        meas_dev.power_offset -= 20 * math.log10(args.amplitude)
    print("=== Initializing port connector...")
    switch = uhd.usrp.cal.get_switch(args.dir, args.switch, args.switch_option)
    print("=== Initializing USRP calibration object...")
    usrp_cal = uhd.usrp.cal.get_usrp_calibrator(
        usrp, meas_dev, args.dir,
        gain_step=args.gain_step,
    )
    channels, antennas, rate = sanitize_args(usrp, args, usrp_cal.default_rate)
    results = init_results(args.load)
    usrp_cal.init(
        rate=rate,
        tone_freq=args.tone_freq,
        amplitude=args.amplitude,
    )
    print("=== Launching calibration...")
    cal_runner = CalRunner(usrp, usrp_cal, meas_dev, args)
    for chan in channels:
        if chan not in results:
            results[chan] = {}
        for ant in antennas:
            if ant in results[chan]:
                print("=== Using pickled data for channel {}, antenna {}."
                      .format(chan, ant))
                continue
            print("=== Running calibration for channel {}, antenna {}."
                  .format(chan, ant))
            # Set up all the objects
            getattr(usrp, 'set_{}_antenna'.format(args.dir))(ant, chan)
            switch.connect(chan, ant)
            usrp_cal.update_port(chan, ant)
            freqs = usrp_cal.init_frequencies(args.start, args.stop, args.step)
            usrp_cal.start() # This will activate siggen
            # Now calibrate
            for freq in freqs:
                try:
                    cal_runner.run(chan, freq)
                except RuntimeError as ex:
                    print("ERROR: Stopping calibration due to exception: {}"
                          .format(str(ex)))
                    usrp_cal.stop()
                    return 1
            # Store results for pickling and shut down for next antenna port
            results[chan][ant] = usrp_cal.results
            usrp_cal.stop() # This will deactivate siggen and store the data
    if args.store:
        print("=== Storing pickled calibration data to {}...".format(args.store))
        with open(args.store, 'wb') as results_file:
            pickle.dump(results, results_file)
    return 0

if __name__ == "__main__":
    try:
        sys.exit(main())
    except (RuntimeError, ValueError) as ex:
        print("ERROR:", str(ex))
        sys.exit(1)
