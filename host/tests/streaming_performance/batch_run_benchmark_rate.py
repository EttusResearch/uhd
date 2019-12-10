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
import re
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

def get_summary_string(stats, iterations, params):
    """
    Returns summary info in a table format.
    """
    header = "| stat | rx samps | tx samps | rx dropped | overrun | rx seq | tx seq | underrun | rx tmo | tx tmo | late |"
    ruler  = "|------|----------|----------|------------|---------|--------|--------|----------|--------|--------|------|"

    def get_params_row(results, iterations, duration, mcr):
        """
        Returns a row containing the test setup, e.g.:
        1 rx, 1 tx, rate 6.452e+06 sps, 1 iterations, 10s duration
        """
        rate = max(results.rx_rate, results.tx_rate)
        s = ""
        s += "{} rx".format(int(results.num_rx_channels))
        s += ", "
        s += "{} tx".format(int(results.num_tx_channels))
        s += ", "
        s += "rate {:.3e} sps".format(round(rate, 2))
        s += ", "
        s += "{} iterations".format(iterations)
        s += ", "
        s += "{}s duration".format(duration)
        if mcr is not None:
            s += ", "
            s += "mcr {}".format(mcr)

        return "| " + s + " "*(len(ruler)-len(s)-3) + "|"

    def get_table_row(results, iterations, duration, stat_label):
        """
        Returns a row of numeric results.
        """
        expected_samps = results.num_rx_channels * duration * results.rx_rate
        rx_samps = 0
        rx_dropped = 0

        if expected_samps > 0:
            rx_samps = results.received_samps / expected_samps * 100
            rx_dropped = results.dropped_samps / expected_samps * 100

        tx_samps = 0
        expected_samps = results.num_tx_channels * duration * results.tx_rate

        if expected_samps > 0:
            tx_samps = results.transmitted_samps / expected_samps * 100

        s = (
            "| {}  ".format(stat_label) +
            "| {:>8} ".format(round(rx_samps, 1)) +
            "| {:>8} ".format(round(tx_samps, 1)) +
            "| {:>10} ".format(round(rx_dropped, 1)) +
            "| {:>7} ".format(round(results.overruns, 1)) +
            "| {:>6} ".format(round(results.rx_seq_errs, 1)) +
            "| {:>6} ".format(round(results.tx_seq_errs, 1)) +
            "| {:>8.1e} ".format(round(results.underruns, 1)) +
            "| {:>6} ".format(round(results.rx_timeouts, 1)) +
            "| {:>6} ".format(round(results.tx_timeouts, 1)) +
            "| {:>4} ".format(round(results.late_cmds, 1))
        )

        return s + "|"

    def get_non_zero_row(results):
        """
        Returns a row with the number of non-zero values for each value.
        """
        s = (
            "| nz   " +
            "| {:>8} ".format(int(results.received_samps)) +
            "| {:>8} ".format(int(results.transmitted_samps)) +
            "| {:>10} ".format(int(results.dropped_samps)) +
            "| {:>7} ".format(int(results.overruns)) +
            "| {:>6} ".format(int(results.rx_seq_errs)) +
            "| {:>6} ".format(int(results.tx_seq_errs)) +
            "| {:>8} ".format(int(results.underruns)) +
            "| {:>6} ".format(int(results.rx_timeouts)) +
            "| {:>6} ".format(int(results.tx_timeouts)) +
            "| {:>4} ".format(int(results.late_cmds))
        )

        return s + "|"

    duration = 10
    if "duration" in params:
        duration = int(params["duration"])

    mcr = None
    if "args" in params:
        args = params["args"]
        expr = ""
        expr += r"master_clock_rate\s*=\s*(\d[\deE+-.]*)"
        match = re.search(expr, args)
        if match:
            mcr = match.group(1)

    s = ""
    s += header + "\n"
    s += ruler + "\n"
    s += get_params_row(stats.avg_vals, iterations, duration, mcr) + "\n"
    s += get_table_row(stats.avg_vals, iterations, duration, "avg") + "\n"
    s += get_table_row(stats.min_vals, iterations, duration, "min") + "\n"
    s += get_table_row(stats.max_vals, iterations, duration, "max") + "\n"
    s += get_non_zero_row(stats.non_zero_vals) + "\n"

    return s

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
    stats = calculate_stats(results)
    print(get_summary_string(stats, iterations, params))
