#!/usr/bin/env python
#
# Copyright 2012 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

import subprocess, time
from optparse import OptionParser
from string import split
import sys
import os

try:
    from gnuradio.eng_option import eng_option
except:
    eng_option = None

def launch_test(args="", rate=None, spb=None, spp=0, prefix="", suffix="", extra=[], verbose=False, title=None):
    real = os.path.realpath(__file__)
    basedir = os.path.dirname(real)
    responder = [
        os.path.join(basedir, "responder")
    ]

    if args is not None and len(args) > 0:
        responder += ["--args=" + args]
    if rate is not None and rate > 0:
        responder += ["--rate=%f" % (rate)]
    if spb is not None and spb > 0:
        responder += ["--spb=%d" % (spb)]
    if spp is not None and spp > 0:
        responder += ["--spp=%d" % (spp)]
    if prefix is not None and len(prefix) > 0:
        responder += ["--stats-file-prefix=" + prefix]
    if suffix is not None and len(suffix) > 0:
        responder += ["--stats-file-suffix=" + suffix]
    if extra is not None:
        responder += extra
    if title is not None and len(title) > 0:
        responder += ["--title=\"" + title + "\""]
    if verbose:
        print "==> Executing:", " ".join(responder)
    try:
        responder += ["--log-file"] # This will produce another output file with logs
        responder += ["--combine-eob"]
        p = subprocess.Popen(responder)
        res = p.wait() # make sure subprocess finishes
    except KeyboardInterrupt:
        res = p.wait() # even in CTRL+C case wait till subprocess finishes
        print "==> Caught CTRL+C"
        return None

    return res

# These return codes should match the C++ return codes
class ReturnCode:
    RETCODE_OK = 0
    RETCODE_BAD_ARGS = -1
    RETCODE_RUNTIME_ERROR = -2
    RETCODE_UNKNOWN_EXCEPTION = -3
    RETCODE_RECEIVE_TIMEOUT = -4
    RETCODE_RECEIVE_FAILED = -5
    RETCODE_MANUAL_ABORT = -6
    RETCODE_BAD_PACKET = -7
    RETCODE_OVERFLOW = -8


def get_initialized_OptionParser():
    def_rates = ".25 1 4 8 25"
    usage = "%prog: [options] -- [extra arguments]"
    opt_kwds = {}
    if eng_option: opt_kwds['option_class'] = eng_option
    parser = OptionParser(usage=usage, **opt_kwds)

    parser.add_option("", "--rates", type="string", help="sample rates (Msps) [default: %default]", default=def_rates)
    parser.add_option("", "--spbs", type="string", help="samples per block [default: %default]",
                      default="32 64 256 1024")
    parser.add_option("", "--spps", type="string", help="samples per packet (0: driver default) [default: %default]",
                      default="0 64 128 256 512")
    parser.add_option("", "--args", type="string", help="UHD device arguments [default: %default]", default=None)
    parser.add_option("", "--prefix", type="string", help="Stats filename prefix [default: %default]", default=None)
    parser.add_option("", "--suffix", type="string", help="Stats filename suffix [default: %default]", default=None)
    parser.add_option("", "--pause", action="store_true", help="pause between tests [default=%default]", default=False)
    parser.add_option("", "--interactive", action="store_true", help="enable prompts within test [default=%default]",
                      default=False)
    parser.add_option("", "--wait", type="float", help="time to wait between tests (seconds) [default=%default]",
                      default=0.0)
    parser.add_option("", "--abort", action="store_true", help="abort on error [default=%default]", default=False)
    parser.add_option("", "--verbose", action="store_true", help="be verbose [default=%default]", default=False)
    parser.add_option("", "--title", type="string", help="test title [default: %default]", default=None)

    return parser


def set_gen_prefix(prefix, save_dir):
    if not save_dir[-1] == "/":
        save_dir = save_dir + "/"

    if prefix == None:
        if os.path.exists(save_dir) is not True:
            os.makedirs(save_dir)
        prefix = save_dir
    return prefix


def get_extra_args(options, args):
    extra_args = {
    "adjust-simulation-rate": None,
    "time-mul": "1e6",
    "test-success": 5,
    "simulate": 1000,
    "iterations": 1000,
    "delay-min": "50e-6",
    "delay-max": "5e-3",
    "delay-step": "50e-6",
    }

    if options.interactive is not True:
        extra_args["batch-mode"] = None
    if options.pause is True:
        extra_args["pause"] = None

    for arg in args:
        if len(arg) > 2 and arg[0:2] == "--":
            arg = arg[2:]
        idx = arg.find('=')
        if idx == -1:
            extra_args[arg] = None
        else:
            extra_args[arg[0:idx]] = arg[idx + 1:]

    def _format_arg(d, k):
        a = "--" + str(k)
        if d[k] is not None:
            a += "=" + str(d[k])
        return a

    extra = map(lambda x: _format_arg(extra_args, x), extra_args)

    print "\n".join(map(lambda x: str(x) + " = " + str(extra_args[x]), extra_args.keys()))

    return extra


def wait_for_keyboard():
    try:
        print "\nPress ENTER to start..."
        raw_input()
        return ReturnCode.RETCODE_OK
    except KeyboardInterrupt:
        print "Aborted"
        return ReturnCode.RETCODE_MANUAL_ABORT


def main():
    parser = get_initialized_OptionParser()
    (options, args) = parser.parse_args()

    save_dir = "results"
    options.prefix = set_gen_prefix(options.prefix, save_dir)
    extra = get_extra_args(options, args)

    rates = map(lambda x: float(x) * 1e6, split(options.rates))
    spbs = map(int, split(options.spbs))
    spps = map(int, split(options.spps))
    total = len(rates) * len(spbs) * len(spps)

    title = options.title or ""
    if len(title) >= 2 and title[0] == "\"" and title[-1] == "\"":
        title = title[1:-1]

    count = 0
    results = {}

    try:
        for rate in rates:
            results_rates = results[rate] = {}
            for spb in spbs:
                results_spbs = results_rates[spb] = {}
                for spp in spps:
                    if count > 0:
                        if options.pause:
                            print "Press ENTER to begin next test..."
                            raw_input()
                        elif options.wait > 0:
                            time.sleep(options.wait)
                    title = "Test #%d of %d (%d%% complete, %d to go)" % (
                        count + 1, total, int(100 * count / total), total - count - 1)
                    res = launch_test(options.args, rate, spb, spp, options.prefix, options.suffix, extra,
                                      options.verbose, title)
                    sys.stdout.flush()
                    count += 1
                    # Break out of loop. Exception thrown if Ctrl + C was pressed.
                    if res is None:
                        raise Exception
                    results_spbs[spp] = res
                    if res < 0 and (res == ReturnCode.RETCODE_MANUAL_ABORT or options.abort):
                        raise Exception
    except:
        pass

    for rate in results.keys():
        results_rates = results[rate]
        for spb in results_rates.keys():
            results_spbs = results_rates[spb]
            for spp in results_spbs.keys():
                res = results_spbs[spp]
                print res, ":", rate, spb, spp
    print "Tests finished"
    return 0


if __name__ == '__main__':
    main()
