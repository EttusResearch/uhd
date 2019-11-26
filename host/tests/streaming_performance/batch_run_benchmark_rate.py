#!/usr/bin/env python3
"""
Copyright 2019 Ettus Research, A National Instrument Brand

SPDX-License-Identifier: GPL-3.0-or-later

Runs the benchmark rate C++ example for a specified number of iterations and
aggregates results.

Example usage:
batch_run_benchmark_rate.py --path <benchmark_rate_dir>/benchmark_rate --iterations 1 --args "addr=192.168.30.2" --rx_rate 1e6
"""
import argparse
import collections
import parse_benchmark_rate
import run_benchmark_rate

Results = collections.namedtuple(
    'Results',
    """
    avg_vals
    min_vals
    max_vals
    non_zero_vals
    """
)

def calculate_stats(results):
    """
    Calculates performance metrics from list of parsed benchmark rate results.
    """
    result_avg = parse_benchmark_rate.average(results)
    result_min = parse_benchmark_rate.min_vals(results)
    result_max = parse_benchmark_rate.max_vals(results)
    result_nz  = parse_benchmark_rate.non_zero_vals(results)
    return Results(
        avg_vals      = result_avg,
        min_vals      = result_min,
        max_vals      = result_max,
        non_zero_vals = result_nz)

def run(path, iterations, benchmark_rate_params, stop_on_error=True):
    """
    Runs benchmark rate multiple times and returns a list of parsed results.
    """
    print("Running benchmark rate {} times with the following arguments: ".format(iterations))
    for key, val in benchmark_rate_params.items():
        print("{:14} {}".format(key, val))

    parsed_results = []
    iteration = 0
    while iteration < iterations:
        proc = run_benchmark_rate.run(path, benchmark_rate_params)
        result = parse_benchmark_rate.parse(proc.stdout.decode('ASCII'))
        if result != None:
            parsed_results.append(result)
            iteration += 1
        else:
            if stop_on_error:
                msg = "Could not parse results of benchmark_rate\n"
                msg += "Benchmark rate arguments:\n"
                msg += str(proc.args) + "\n"
                msg += "Stderr capture:\n"
                msg += proc.stderr.decode('ASCII')
                msg += "Stdout capture:\n"
                msg += proc.stdout.decode('ASCII')
                raise RuntimeError(msg)
            else:
                print("Failed to parse benchmark rate results")
                print(proc.stderr.decode('ASCII'))

    return parsed_results

def get_summary_string(summary, params=None):
    """
    Returns summary info in a string resembling benchmark_rate output.
    """
    statistics_msg = """
Benchmark rate summary:
    Num received samples:     avg {}, min {}, max {}, non-zero {}
    Num dropped samples:      avg {}, min {}, max {}, non-zero {}
    Num overruns detected:    avg {}, min {}, max {}, non-zero {}
    Num transmitted samples:  avg {}, min {}, max {}, non-zero {}
    Num sequence errors (Tx): avg {}, min {}, max {}, non-zero {}
    Num sequence errors (Rx): avg {}, min {}, max {}, non-zero {}
    Num underruns detected:   avg {}, min {}, max {}, non-zero {}
    Num late commands:        avg {}, min {}, max {}, non-zero {}
    Num timeouts (Tx):        avg {}, min {}, max {}, non-zero {}
    Num timeouts (Rx):        avg {}, min {}, max {}, non-zero {}
""".format(
        summary.avg_vals.received_samps,
        summary.min_vals.received_samps,
        summary.max_vals.received_samps,
        summary.non_zero_vals.received_samps,
        summary.avg_vals.dropped_samps,
        summary.min_vals.dropped_samps,
        summary.max_vals.dropped_samps,
        summary.non_zero_vals.dropped_samps,
        summary.avg_vals.overruns,
        summary.min_vals.overruns,
        summary.max_vals.overruns,
        summary.non_zero_vals.overruns,
        summary.avg_vals.transmitted_samps,
        summary.min_vals.transmitted_samps,
        summary.max_vals.transmitted_samps,
        summary.non_zero_vals.transmitted_samps,
        summary.avg_vals.tx_seq_errs,
        summary.min_vals.tx_seq_errs,
        summary.max_vals.tx_seq_errs,
        summary.non_zero_vals.tx_seq_errs,
        summary.avg_vals.rx_seq_errs,
        summary.min_vals.rx_seq_errs,
        summary.max_vals.rx_seq_errs,
        summary.non_zero_vals.rx_seq_errs,
        summary.avg_vals.underruns,
        summary.min_vals.underruns,
        summary.max_vals.underruns,
        summary.non_zero_vals.underruns,
        summary.avg_vals.late_cmds,
        summary.min_vals.late_cmds,
        summary.max_vals.late_cmds,
        summary.non_zero_vals.late_cmds,
        summary.avg_vals.tx_timeouts,
        summary.min_vals.tx_timeouts,
        summary.max_vals.tx_timeouts,
        summary.non_zero_vals.tx_timeouts,
        summary.avg_vals.rx_timeouts,
        summary.min_vals.rx_timeouts,
        summary.max_vals.rx_timeouts,
        summary.non_zero_vals.rx_timeouts)

    if params is not None:
        duration = 10
        if "duration" in params:
            duration = int(params["duration"])

        num_rx_channels = summary.min_vals.num_rx_channels
        rx_rate =  summary.min_vals.rx_rate

        if num_rx_channels != 0:
            avg_samps = summary.avg_vals.received_samps
            expected_samps = num_rx_channels * duration * rx_rate
            percent = (avg_samps) / expected_samps * 100
            statistics_msg += "    Expected samps (Rx):      {}\n".format(expected_samps)
            statistics_msg += "    Actual samps % (Rx):      {}\n".format(round(percent, 1))

        num_tx_channels = summary.min_vals.num_tx_channels
        tx_rate =  summary.min_vals.tx_rate

        if num_tx_channels != 0:
            avg_samps = summary.avg_vals.transmitted_samps
            expected_samps = num_tx_channels * duration * tx_rate
            percent = (avg_samps) / expected_samps * 100
            statistics_msg += "    Expected samps (Tx):      {}\n".format(expected_samps)
            statistics_msg += "    Actual samps % (Tx):      {}\n".format(round(percent, 1))

    return statistics_msg

def parse_args():
    """
    Parse the command line arguments for batch run benchmark rate.
    """
    benchmark_rate_params, rest = run_benchmark_rate.parse_known_args()
    parser = argparse.ArgumentParser()
    parser.add_argument("--path", type=str, required=True, help="path to benchmark rate example")
    parser.add_argument("--iterations", type=int, default=100, help="number of iterations to run")
    params = parser.parse_args(rest)
    return params.path, params.iterations, benchmark_rate_params

if __name__ == "__main__":
    path, iterations, params = parse_args();
    results = run(path, iterations, params)
    summary = calculate_stats(results)
    print(get_summary_string(summary, params))
