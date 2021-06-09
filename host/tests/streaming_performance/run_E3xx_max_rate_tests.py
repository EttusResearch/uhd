#!/usr/bin/env python3
"""
Copyright 2019 Ettus Research, A National Instrument Brand

SPDX-License-Identifier: GPL-3.0-or-later

Runs streaming tests for N3xx at rates in the neighborhoood of the maximum
rate that UHD can sustain. Each test consists of a batch of runs of the
benchmark rate C++ example with different streaming parameters.

To run all the tests, execute it with all supported options for the test_type
parameter:
    E320_XG      Runs E320 tests with single and dual 10 GbE links
    E310_Liberio Runs E310 tests with Liberio

Example usage:
run_E3xx_max_rate_tests.py --path <benchmark_rate_dir>/benchmark_rate --addr 192.168.10.2 --second_addr 192.168.20.2 --test_type E320_XG
"""
import argparse
import sys
import time
import datetime
import batch_run_benchmark_rate

Test_Type_E320_XG = "E320_XG"
Test_Type_E310_Liberio = "E310_Liberio"

test_type_list = [Test_Type_E320_XG, Test_Type_E310_Liberio]

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
        choices=test_type_list,
        help="path to benchmark rate example")
    parser.add_argument(
        "--addr",
        type=str,
        default = "",
        help="address of the 10 GbE interface")
    parser.add_argument(
        "--use_dpdk",
        action='store_true',
        help="enable DPDK")
    parser.add_argument(
        "--mgmt_addr",
        type=str,
        default="",
        help="address of management interface. only needed for DPDK test cases"
    )
    args = parser.parse_args()

    return args.path, args.test_type, args.addr, args.use_dpdk, args.mgmt_addr

def run_test(path, params, iterations, label):
    """
    Runs benchmark rate for the number of iterations in the command line arguments
    """
    print("-----------------------------------------------------------")
    print(label + "\n")
    results = batch_run_benchmark_rate.run(path, iterations, params, False)
    stats = batch_run_benchmark_rate.calculate_stats(results)
    print(batch_run_benchmark_rate.get_summary_string(stats, iterations, params))


def run_E320_tests_for_single_10G(
        path, addr, iterations, duration, use_dpdk=False, mgmt_addr=''):
    """
    Runs tests that are in the neighborhood of max rate for 10 GbE
    """

    def base_params(rate):
        if use_dpdk == True:
            return {
                "args": "addr={},master_clock_rate={},use_dpdk=1,mgmt_addr={}"
                        .format(addr, rate, mgmt_addr),
                "duration": duration
            }
        else:
            return {
                "args": "addr={},master_clock_rate={}".format(addr, rate),
                "duration": duration
            }

    # Run RX at 61.44 Msps with one channel
    rate = "61.44e6"
    rx_params = base_params(rate)
    rx_params["rx_rate"] = rate
    rx_params["rx_channels"] = "0"
    run_test(path, rx_params, iterations, "1xRX @{}".format(rate))

    # Run RX at 61.44 Msps with two channels
    rate = "61.44e6"
    rx_params = base_params(rate)
    rx_params["rx_rate"] = rate
    rx_params["rx_channels"] = "0,1"
    run_test(path, rx_params, iterations, "2xRX @{}".format(rate))

    # Run TX at 61.44 Msps with one channel
    rate = "61.44e6"
    tx_params = base_params(rate)
    tx_params["tx_rate"] = rate
    tx_params["tx_channels"] = "0"
    run_test(path, tx_params, iterations, "1xTX @{}".format(rate))

    # Run TX at 61.44 Msps with two channels
    rate = "61.44e6"
    tx_params = base_params(rate)
    tx_params["tx_rate"] = rate
    tx_params["tx_channels"] = "0,1"
    run_test(path, tx_params, iterations, "2xTX @{}".format(rate))

    # Run TRX at 61.44 Msps with one channels
    rate = "61.44e6"
    trx_params = base_params(rate)
    trx_params["tx_rate"] = rate
    trx_params["rx_rate"] = rate
    trx_params["tx_channels"] = "0"
    trx_params["rx_channels"] = "0"
    run_test(path, trx_params, iterations, "1xTRX @{}".format(rate))

    # Run TRX at 61.44 Msps with two channels
    rate = "61.44e6"
    trx_params = base_params(rate)
    trx_params["tx_rate"] = rate
    trx_params["rx_rate"] = rate
    trx_params["tx_channels"] = "0,1"
    trx_params["rx_channels"] = "0,1"
    run_test(path, trx_params, iterations, "2xTRX @{}".format(rate))


def run_E320_tests_for_single_10G_long_duration(
        path, addr, iterations, duration, use_dpdk=False, mgmt_addr=''):
    """
    Runs tests that are in the neighborhood of max rate for 10 GbE. Only include
    a small subset of tests to run for a longer period.
    """
    def base_params(rate):
        if use_dpdk == True:
            return {
                "args": "addr={},master_clock_rate={},use_dpdk=1,mgmt_addr={}"
                        .format(addr, rate, mgmt_addr),
                "duration": duration
            }
        else:
            return {
                "args": "addr={},master_clock_rate={}".format(addr, rate),
                "duration": duration
            }

    # Run TRX at 61.44 Msps with two channels
    rate = "61.44e6"
    trx_params = base_params(rate)
    trx_params["tx_rate"] = rate
    trx_params["rx_rate"] = rate
    trx_params["tx_channels"] = "0,1"
    trx_params["rx_channels"] = "0,1"
    run_test(path, trx_params, iterations, "2xTRX @{}".format(rate))

def main():
    path, test_type, addr, use_dpdk, mgmt_addr = parse_args()
    start_time = time.time()

    if test_type == Test_Type_E320_XG:
        run_E320_tests_for_single_10G(
            path, addr, 10, 30, use_dpdk, mgmt_addr)
        run_E320_tests_for_single_10G_long_duration(
            path, addr, 2, 600, use_dpdk, mgmt_addr)

    end_time = time.time()
    elapsed = end_time - start_time
    print("Elapsed time: {}".format(datetime.timedelta(seconds=elapsed)))
    return True

if __name__ == "__main__":
    sys.exit(not main())
