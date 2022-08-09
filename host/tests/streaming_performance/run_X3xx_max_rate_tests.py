#!/usr/bin/env python3
"""
Copyright 2019 Ettus Research, A National Instrument Brand

SPDX-License-Identifier: GPL-3.0-or-later

Runs streaming tests for X3xx at rates in the neighborhoood of the maximum
rate that UHD can sustain. Each test consists of a batch of runs of the
benchmark rate C++ example with different streaming parameters.

To run all the tests, execute it with all supported options for the test_type
parameter:
    X3xx_XG   Runs tests with single and dual 10 GbE links
    TwinRX_XG Runs TwinRX tests with single and dual 10 GbE links

Example usage:
run_X3xx_max_rate_tests.py --path <benchmark_rate_dir>/benchmark_rate --addr 192.168.30.2 --second_addr 192.168.40.2 --test_type X3xx_XG
"""
import argparse
import sys
import time
import datetime
import batch_run_benchmark_rate

Test_Type_X3xx_XG = "X3xx_XG"
Test_Type_TwinRX_XG = "TwinRX_XG"

test_type_list = [Test_Type_X3xx_XG, Test_Type_TwinRX_XG]

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
        help="address of first 10 GbE interface")
    parser.add_argument(
        "--second_addr",
        type=str,
        default = "",
        help="address of second 10 GbE interface")
    parser.add_argument(
        "--use_dpdk",
        action='store_true',
        help="enable DPDK")
    args = parser.parse_args()

    return args.path, args.test_type, args.addr, args.second_addr, args.use_dpdk

def run_test(path, params, iterations, label):
    """
    Runs benchmark rate for the number of iterations in the command line arguments.
    """
    print("-----------------------------------------------------------")
    print(label + "\n")
    results = batch_run_benchmark_rate.run(path, iterations, params)
    stats = batch_run_benchmark_rate.calculate_stats(results)
    print(batch_run_benchmark_rate.get_summary_string(stats, iterations, params))


def run_tests_for_single_10G(path, addr, iterations, duration, use_dpdk=False):
    if use_dpdk == True:
        base_params = {
            "args": "addr={},use_dpdk=1".format(addr),
            "duration": duration
        }
    else:
        base_params = {
            "args": "addr={}".format(addr),
            "duration": duration
        }
    rx_params = base_params.copy()

    # Run 200 Msps RX with one channel
    rx_params["rx_rate"] = "200e6"
    rx_params["rx_channels"] = "0"
    run_test(path, rx_params, iterations, "1xRX @200 Msps")

    # Run 100 Msps with two channels
    rx_params["rx_rate"] = "100e6"
    rx_params["rx_channels"] = "0,1"
    run_test(path, rx_params, iterations, "2xRX @100 Msps")

    tx_params = base_params.copy()

    # Run 200 Msps TX with one channel
    tx_params["tx_rate"] = "200e6"
    tx_params["tx_channels"] = "0"
    run_test(path, tx_params, iterations, "1xTX @200 Msps")

    # Run 100 Msps TX with two channels
    tx_params["tx_rate"] = "100e6"
    tx_params["tx_channels"] = "0,1"
    run_test(path, tx_params, iterations, "2xTX @100 Msps")

    trx_params = base_params.copy()

    # Run 200 Msps TRX with one channel
    trx_params["tx_rate"] = "200e6"
    trx_params["rx_rate"] = "200e6"
    trx_params["tx_channels"] = "0"
    trx_params["rx_channels"] = "0"
    run_test(path, trx_params, iterations, "1xTRX @200Msps")

    # Run 100 Msps TRX with two channels
    trx_params["tx_rate"] = "100e6"
    trx_params["rx_rate"] = "100e6"
    trx_params["tx_channels"] = "0,1"
    trx_params["rx_channels"] = "0,1"
    run_test(path, trx_params, iterations, "2xTRX @100Msps")


def run_tests_for_dual_10G(path, addr, second_addr, iterations, duration, use_dpdk=False):
    if use_dpdk == True:
        base_params = {
            "args": ("addr={},second_addr={},"
                     "enable_tx_dual_eth=1,use_dpdk=1")
                     .format(addr, second_addr),
            "duration": duration
        }
    else:
        base_params = {
            "args": "addr={},second_addr={},enable_tx_dual_eth=1"
                    .format(addr, second_addr),
            "duration": duration
        }

    rx_params = base_params.copy()

    # Run 200 Msps with two channels
    rx_params["rx_rate"] = "200e6"
    rx_params["rx_channels"] = "0,1"
    run_test(path, rx_params, iterations, "2xRX @200 Msps")

    tx_params = base_params.copy()

    # Run 200 Msps TX with two channels
    tx_params["tx_rate"] = "200e6"
    tx_params["tx_channels"] = "0,1"
    run_test(path, tx_params, iterations, "2xTX @200 Msps")

    trx_params = base_params.copy()

    # Run 100 Msps TRX with two channels
    trx_params["tx_rate"] = "200e6"
    trx_params["rx_rate"] = "200e6"
    trx_params["tx_channels"] = "0,1"
    trx_params["rx_channels"] = "0,1"
    run_test(path, trx_params, iterations, "2xTRX @200Msps")


def run_tests_for_single_10G_Twin_RX(
        path, addr, iterations, duration, use_dpdk=False):
    if use_dpdk == True:
        base_params = {
            "args": "addr={},use_dpdk=1".format(addr),
            "duration": duration
        }
    else:
        base_params = {
            "args": "addr={}".format(addr),
            "duration": duration
        }

    rx_params = base_params.copy()

    # Run 100 Msps with three channels
    rx_params["rx_rate"] = "100e6"
    rx_params["rx_channels"] = "0,1,2"
    run_test(path, rx_params, iterations, "3xRX @100 Msps")

    # Run 50 Msps with four channels
    rx_params["rx_rate"] = "50e6"
    rx_params["rx_channels"] = "0,1,2,3"
    run_test(path, rx_params, iterations, "4xRX @50 Msps")


def run_tests_for_dual_10G_Twin_RX(
        path, addr, second_addr, iterations, duration, use_dpdk=False):
    if use_dpdk == True:
        base_params = {
            "args": "addr={},second_addr={},use_dpdk=1"
                    .format(addr, second_addr),
            "duration": duration
        }
    else:
        base_params = {
            "args": "addr={},second_addr={}".format(addr, second_addr),
            "duration": duration
        }

    rx_params = base_params.copy()

    # Run 100 Msps with four channels
    rx_params["rx_rate"] = "100e6"
    rx_params["rx_channels"] = "0,1,2,3"
    run_test(path, rx_params, iterations, "4xRX @100 Msps")


def main():
    path, test_type, addr, second_addr, use_dpdk = parse_args()
    start_time = time.time()

    if test_type == Test_Type_X3xx_XG:
        # Run 10 test iterations for 60 seconds each
        run_tests_for_single_10G(path, addr, 10, 60, use_dpdk)
        run_tests_for_dual_10G(path, addr, second_addr, 10, 60, use_dpdk)

        # Run 2 test iterations for 600 seconds each
        run_tests_for_single_10G(path, addr, 2, 600, use_dpdk)
        run_tests_for_dual_10G(path, addr, second_addr, 2, 600, use_dpdk)

    if test_type == Test_Type_TwinRX_XG:
        # Run 10 test iterations for 60 seconds each
        run_tests_for_single_10G_Twin_RX(path, addr, 10, 60, use_dpdk)
        run_tests_for_dual_10G_Twin_RX(path, addr, second_addr, 10, 60, use_dpdk)

        # Run 2 test iterations for 600 seconds each
        run_tests_for_single_10G_Twin_RX(path, addr, 2, 600, use_dpdk)
        run_tests_for_dual_10G_Twin_RX(path, addr, second_addr, 2, 600, use_dpdk)

    end_time = time.time()
    elapsed = end_time - start_time
    print("Elapsed time: {}".format(datetime.timedelta(seconds=elapsed)))
    return True

if __name__ == "__main__":
    sys.exit(not main())
