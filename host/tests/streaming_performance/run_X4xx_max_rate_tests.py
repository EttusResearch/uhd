#!/usr/bin/env python3
"""
Copyright 2021 Ettus Research, A National Instrument Brand

SPDX-License-Identifier: GPL-3.0-or-later

Runs streaming tests for X4xx at rates in the neighborhoood of the maximum
rate that UHD can sustain. Each test consists of a batch of runs of the
benchmark rate C++ example with different streaming parameters.

To run all the tests, execute it with all supported options for the test_type
parameter:
    1Gbe, 1x10Gbe, 2x10Gbe, 1x100Gbe, 2x100Gbe

Example usage::
run_X4xx_max_rate_tests.py --path <benchmark_rate_dir>/benchmark_rate --addr 192.168.10.2 --second_addr 192.168.20.2 --mgmt_addr 192.168.40.2 --test_type 1x100Gbe --use_dpdk 1
"""
import argparse
import sys
import time
import datetime
import batch_run_benchmark_rate


def parse_args():
    """
    Parse command line arguments
    """
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--path",
        type=str,
        required=True,
        help="path to benchmark rate example")
    parser.add_argument(
        "--test_type",
        type=str,
        required=True,
        choices=["1Gbe", "1x10Gbe", "2x10Gbe", "1x100Gbe", "2x100Gbe"],
        help="test type you would like to run eg. 1Gbe, 1x10Gbe, 2x10Gbe, 1x100Gbe, 2x100Gbe")
    parser.add_argument(
        "--addr",
        type=str,
        default = "",
        help="address of first interface")
    parser.add_argument(
        "--second_addr",
        type=str,
        default = "",
        help="address of second interface")
    parser.add_argument(
        "--mgmt_addr",
        type=str,
        default = "",
        help="mgmt address")
    parser.add_argument(
        "--use_dpdk",
        default = False,
        action="store_true",
        help="enable DPDK (you must run the script as root to use this)")

    return parser.parse_args()

def run_test(path, params, iterations, label):
    """
    Runs benchmark rate for the number of iterations in the command line arguments.
    """
    print("-----------------------------------------------------------")
    print(label + "\n")
    results = batch_run_benchmark_rate.run(path, iterations, params)
    stats = batch_run_benchmark_rate.calculate_stats(results)
    print(batch_run_benchmark_rate.get_summary_string(stats, iterations, params))

def run_tests_for_single(path, base_params, iterations, duration, rate):

    base_params["duration"] = duration

    rx_params = base_params.copy()

    # Run 20 Msps RX with one channel
    rx_params["rx_rate"] = str(rate)
    rx_params["rx_channels"] = "0"
    print(rx_params)
    run_test(path, rx_params, iterations, "1xRX @"+ str(rate/1e6) +" Msps")

    # Run 10 Msps with two channels
    rx_params["rx_rate"] = str(rate/2)
    rx_params["rx_channels"] = "0,1"
    print(rx_params)
    run_test(path, rx_params, iterations, "2xRX @"+ str(rate/2e6) +" Msps")

    tx_params = base_params.copy()

    # Run 20 Msps TX with one channel
    tx_params["tx_rate"] = str(rate)
    tx_params["tx_channels"] = "0"
    print(tx_params)
    run_test(path, tx_params, iterations, "1xTX @"+ str(rate/1e6) +" Msps")

    # Run 10 Msps TX with two channels
    tx_params["tx_rate"] = "100e5"
    tx_params["tx_channels"] = "0,1"
    print(tx_params)
    run_test(path, tx_params, iterations, "2xTX @"+ str(rate/2e6) +" Msps")

    trx_params = base_params.copy()

    # Run 20 Msps TRX with one channel
    trx_params["tx_rate"] = str(rate)
    trx_params["rx_rate"] = str(rate)
    trx_params["tx_channels"] = "0"
    trx_params["rx_channels"] = "0"
    print(trx_params)
    run_test(path, trx_params, iterations, "1xTRX @"+ str(rate/1e6) +" Msps")

    # Run 10 Msps TRX with two channels
    trx_params["tx_rate"] = str(rate/2)
    trx_params["rx_rate"] = str(rate/2)
    trx_params["tx_channels"] = "0,1"
    trx_params["rx_channels"] = "0,1"
    print(trx_params)
    run_test(path, trx_params, iterations, "2xTRX @"+ str(rate/2e6) +" Msps")


def run_tests_for_dual(base_params, path, duration, iterations, rate):

    base_params["duration"] = duration

    rx_params = base_params.copy()
    # Run 200 Msps with two channels
    rx_params["rx_rate"] = str(rate)
    rx_params["rx_channels"] = "0,1"
    print(rx_params)
    run_test(path, rx_params, iterations, "2xRX @"+ str(rate/1e6) +" Msps")

    tx_params = base_params.copy()

    # Run 200 Msps TX with two channels
    tx_params["tx_rate"] = str(rate)
    tx_params["tx_channels"] = "0,1"
    print(tx_params)
    run_test(path, tx_params, iterations, "2xTX @"+ str(rate/1e6) +" Msps")

    trx_params = base_params.copy()

    # Run 100 Msps TRX with two channels
    trx_params["tx_rate"] = str(rate/2)
    trx_params["rx_rate"] = str(rate/2)
    trx_params["tx_channels"] = "0,1"
    trx_params["rx_channels"] = "0,1"
    print(trx_params)
    run_test(path, trx_params, iterations, "2xTRX @"+ str(rate/2e6) +" Msps")



def main():
    args = parse_args()

    base_params = {
        "args" : f"addr={args.addr},second_addr={args.second_addr}",
    }
    if args.use_dpdk:
        base_params["args"] += f",mgmt_addr={args.mgmt_addr},use_dpdk=1"

    start_time = time.time()

    rate = {
        "1Gbe": 200e5,
        "1x10Gbe": 200e6,
        "2x10Gbe": 200e6,
        "1x100Gbe": 200e7, # Rate doesn't matter here as 100Gbe has no DUC
        "2x100Gbe": 200e7}

    test_config  = {
        "1Gbe": run_tests_for_single,
        "1x10Gbe": run_tests_for_single,
        "2x10Gbe": run_tests_for_dual,
        "1x100Gbe": run_tests_for_single,
        "2x100Gbe": run_tests_for_dual}


    # Run 10 test iterations for 60 seconds each
    test_config[args.test_type](args.path, base_params, 10, 60, rate[args.test_type])
    # Run 2 test iterations for 600 seconds each
    test_config[args.test_type](args.path, base_params, 2, 600, rate[args.test_type])

    end_time = time.time()
    elapsed = end_time - start_time
    print("Elapsed time: {}".format(datetime.timedelta(seconds=elapsed)))
    return True

if __name__ == "__main__":
    sys.exit(not main())
