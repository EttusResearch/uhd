"""
Copyright 2019 Ettus Research, A National Instrument Brand

SPDX-License-Identifier: GPL-3.0-or-later

Helper script that provides a Python interface to run the benchmark rate C++
example.
"""
import argparse
import subprocess

def run(path, params):
    """
    Run benchmark rate and return a CompletedProcess object.
    """
    proc_params = [path]

    for key, val in params.items():
        proc_params.append("--" + str(key))
        proc_params.append(str(val))

    return subprocess.run(proc_params, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

def create_parser():
    parser = argparse.ArgumentParser()
    parser.add_argument("--args", type=str, help="single uhd device address args")
    parser.add_argument("--duration", type=str, help="duration for the test in seconds")
    parser.add_argument("--rx_subdev", type=str, help="specify the device subdev for RX")
    parser.add_argument("--tx_subdev", type=str, help="specify the device subdev for TX")
    parser.add_argument("--rx_stream_args", type=str, help="stream args for RX streamer")
    parser.add_argument("--tx_stream_args", type=str, help="stream args for TX streamer")
    parser.add_argument("--rx_rate", type=str, help="specify to perform a RX rate test (sps)")
    parser.add_argument("--tx_rate", type=str, help="specify to perform a TX rate test (sps)")
    parser.add_argument("--rx_otw", type=str, help="specify the over-the-wire sample mode for RX")
    parser.add_argument("--tx_otw", type=str, help="specify the over-the-wire sample mode for TX")
    parser.add_argument("--rx_cpu", type=str, help="specify the host/cpu sample mode for RX")
    parser.add_argument("--tx_cpu", type=str, help="specify the host/cpu sample mode for TX")
    parser.add_argument("--ref", type=str, help="clock reference (internal, external, mimo, gpsdo)")
    parser.add_argument("--pps", type=str, help="PPS source (internal, external, mimo, gpsdo)")
    parser.add_argument("--random", type=str, help="Run with random values of samples in send() and recv()")
    parser.add_argument("--rx_channels", type=str, help="which RX channel(s) to use")
    parser.add_argument("--tx_channels", type=str, help="which TX channel(s) to use")
    parser.add_argument("--priority", type=str, help="thread priority (normal, high)")
    parser.add_argument("--multi_streamer", action="count", help="create a separate streamer per channel")
    return parser

def parse_args():
    """
    Parse the command line arguments for benchmark rate, and returns arguments
    in a dict.
    """
    parser = create_parser()
    params = vars(parser.parse_args())
    return { key : params[key] for key in params if params[key] != None }

def parse_known_args():
    """
    Parse the command line arguments for benchmark rate. Returns a dict
    containing the benchmark rate args, and a list containing args that
    are not recognized by benchmark rate.
    """
    parser = create_parser()
    params, rest = parser.parse_known_args()
    params = vars(params)
    return { key : params[key] for key in params if params[key] != None }, rest

if __name__ == "__main__":
    """
    Main function used for testing only. Requires path to the example to be
    passed as a command line parameter.
    """
    benchmark_rate_params, rest = parse_known_args()
    parser = argparse.ArgumentParser()
    parser.add_argument("--path", type=str, required=True, help="path to benchmark rate example")
    params = parser.parse_args(rest);
    proc = run(params.path, benchmark_rate_params)

    print("ARGS")
    print(proc.args)

    print("RETURNCODE")
    print(proc.returncode)

    print("STDERR")
    print(proc.stderr.decode('ASCII'))

    print("STDOUT")
    print(proc.stdout.decode('ASCII'))
