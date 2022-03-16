"""
Copyright 2019 Ettus Research, A National Instrument Brand

SPDX-License-Identifier: GPL-3.0-or-later

Helper script that parses the results of benchmark_rate and extracts numeric
values printed at the end of execution.
"""

import collections
import re
import csv

Results = collections.namedtuple(
    'Results',
    """
    num_rx_channels
    num_tx_channels
    rx_rate
    tx_rate
    received_samps
    dropped_samps
    overruns
    transmitted_samps
    tx_seq_errs
    rx_seq_errs
    underruns
    late_cmds
    tx_timeouts
    rx_timeouts
    """
)

def average(results):
    """
    Returns the average of a list of results.
    """
    results_as_lists = [list(r) for r in results]
    avg_vals = [sum(x)/len(results) for x in zip(*results_as_lists)]
    return Results(*avg_vals)

def min_vals(results):
    """
    Returns the minimum values of a list of results.
    """
    results_as_lists = [list(r) for r in results]
    min_vals = [min(x) for x in zip(*results_as_lists)]
    return Results(*min_vals)

def max_vals(results):
    """
    Returns the maximum values of a list of results.
    """
    results_as_lists = [list(r) for r in results]
    max_vals = [max(x) for x in zip(*results_as_lists)]
    return Results(*max_vals)

def non_zero_vals(results):
    """
    Returns the number of non-zero values from a list of results.
    """
    results_as_lists = [list(r) for r in results]
    results_as_lists = [[1 if x > 0 else 0 for x in y] for y in results_as_lists]
    non_zero_vals = [sum(x) for x in zip(*results_as_lists)]
    return Results(*non_zero_vals)

def parse(result_str):
    """
    Parses benchmark results and returns numerical values.
    """
    # Parse rx rate
    rx_rate = 0.0
    num_rx_channels = 0
    expr = "Testing receive rate ([0-9]+\.[0-9]+) Msps on (\d+) channels"
    matches = re.findall(expr, result_str)
    if matches:
        rx_rate = float(matches[0][0]) * 1.0e6
        for match in matches:
            num_rx_channels += int(match[1])

    tx_rate = 0.0
    num_tx_channels = 0
    expr = "Testing transmit rate ([0-9]+\.[0-9]+) Msps on (\d+) channels"
    matches = re.findall(expr, result_str)
    if matches:
        tx_rate = float(matches[0][0]) * 1.0e6
        for match in matches:
            num_tx_channels += int(match[1])

    # Parse results
    expr = "Benchmark rate summary:"
    expr += r"\s*Num received samples:\s*(\d+)"
    expr += r"\s*Num dropped samples:\s*(\d+)"
    expr += r"\s*Num overruns detected:\s*(\d+)"
    expr += r"\s*Num transmitted samples:\s*(\d+)"
    expr += r"\s*Num sequence errors \(Tx\):\s*(\d+)"
    expr += r"\s*Num sequence errors \(Rx\):\s*(\d+)"
    expr += r"\s*Num underruns detected:\s*(\d+)"
    expr += r"\s*Num late commands:\s*(\d+)"
    expr += r"\s*Num timeouts \(Tx\):\s*(\d+)"
    expr += r"\s*Num timeouts \(Rx\):\s*(\d+)"
    match = re.search(expr, result_str)
    if match:
        return Results(
            num_rx_channels   = num_rx_channels,
            num_tx_channels   = num_tx_channels,
            rx_rate           = rx_rate,
            tx_rate           = tx_rate,
            received_samps    = int(match.group(1)),
            dropped_samps     = int(match.group(2)),
            overruns          = int(match.group(3)),
            transmitted_samps = int(match.group(4)),
            tx_seq_errs       = int(match.group(5)),
            rx_seq_errs       = int(match.group(6)),
            underruns         = int(match.group(7)),
            late_cmds         = int(match.group(8)),
            tx_timeouts       = int(match.group(9)),
            rx_timeouts       = int(match.group(10))
        )
    else:
        return None

def write_benchmark_rate_csv(results, file_name):
    with open(file_name, 'w', newline='') as f:
        w = csv.writer(f)
        w.writerow(results[0]._fields)
        w.writerows(results)

if __name__ == "__main__":
    result_str = """
    [00:00:00.000376] Creating the usrp device with: addr=192.168.30.2, second_addr=192.168.40.2...
    [00:00:05.63100253] Testing receive rate 200.000000 Msps on 2 channels
    [00:00:05.73100253] Testing transmit rate 100.000000 Msps on 1 channels
    [00:00:15.113339078] Benchmark complete.

    Benchmark rate summary:
    Num received samples:     10000
    Num dropped samples:      200
    Num overruns detected:    10
    Num transmitted samples:  20000
    Num sequence errors (Tx): 5
    Num sequence errors (Rx): 6
    Num underruns detected:   20
    Num late commands:        2
    Num timeouts (Tx):        0
    Num timeouts (Rx):        100

    Done!
    """
    print("Parsing hardcoded string for testing only")
    print(parse(result_str))
