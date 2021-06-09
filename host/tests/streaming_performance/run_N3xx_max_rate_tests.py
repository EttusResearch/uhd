#!/usr/bin/env python3
"""
Copyright 2019 Ettus Research, A National Instrument Brand

SPDX-License-Identifier: GPL-3.0-or-later

Runs streaming tests for N3xx at rates in the neighborhoood of the maximum
rate that UHD can sustain. Each test consists of a batch of runs of the
benchmark rate C++ example with different streaming parameters.

To run all the tests, execute it with all supported options for the test_type
parameter:
    N310_XG      Runs N310 tests with single and dual 10 GbE links
    N310_Liberio Runs N310 tests with Liberio
    N320_XG      Runs N320 tests with single and dual 10 GbE links
    N320_Liberio Runs N320 tests with Liberio

Example usage:
run_N3xx_max_rate_tests.py --path <benchmark_rate_dir>/benchmark_rate --addr 192.168.10.2 --second_addr 192.168.20.2 --test_type N310_XG
"""
import argparse
import sys
import time
import datetime
import batch_run_benchmark_rate

Test_Type_N310_XG = "N310_XG"
Test_Type_N310_Liberio = "N310_Liberio"
Test_Type_N320_XG = "N320_XG"
Test_Type_N320_Liberio = "N320_Liberio"

test_type_list = [
    Test_Type_N310_XG,
    Test_Type_N310_Liberio,
    Test_Type_N320_Liberio,
    Test_Type_N320_XG]

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
        "--mgmt_addr",
        type=str,
        default="",
        help="address of management interface. only needed for DPDK test cases")
    parser.add_argument(
        "--use_dpdk",
        action='store_true',
        help="enable DPDK")
    args = parser.parse_args()

    return args.path, args.test_type, args.addr, args.second_addr,\
        args.mgmt_addr, args.use_dpdk

def run_test(path, params, iterations, label):
    """
    Runs benchmark rate for the number of iterations in the command line arguments
    """
    print("-----------------------------------------------------------")
    print(label + "\n")
    results = batch_run_benchmark_rate.run(path, iterations, params)
    stats = batch_run_benchmark_rate.calculate_stats(results)
    print(batch_run_benchmark_rate.get_summary_string(stats, iterations, params))


def run_N310_tests_for_single_10G(
        path, addr, iterations, duration, use_dpdk=False, mgmt_addr=''):
    """
    Runs tests that are in the neighborhood of max rate for 10 GbE
    """
    def base_params(rate):
        if use_dpdk is True:
            return {
                "args": "addr={},master_clock_rate={},use_dpdk=1,mgmt_addr={},"
                         .format(addr, rate, mgmt_addr),
                "duration": duration
            }
        else:
            return {
                "args" : "addr={},master_clock_rate={}".format(addr, rate),
                "duration" : duration
            }

    # Run RX at 153.6 Msps with one channel
    rate = "153.6e6"
    params = base_params(rate)
    params["rx_rate"] = rate
    params["rx_channels"] = "0"
    run_test(path, params, iterations, "1xRX @{}".format(rate))

    # Run RX at 153.6 Msps with two channels
    rate = "153.6e6"
    params = base_params(rate)
    params["rx_rate"] = rate
    params["rx_channels"] = "0,1"
    run_test(path, params, iterations, "2xRX @{}".format(rate))

    # Run TX at 153.6 Msps with one channel
    rate = "153.6e6"
    params = base_params(rate)
    params["tx_rate"] = rate
    params["tx_channels"] = "0"
    run_test(path, params, iterations, "1xTX @{}".format(rate))

    # Run TX at 153.6 Msps with two channels
    rate = "153.6e6"
    params = base_params(rate)
    params["tx_rate"] = rate
    params["tx_channels"] = "0,1"
    run_test(path, params, iterations, "2xTX @{}".format(rate))

    # Run TRX at 153.6 Msps with one channel
    rate = "153.6e6"
    params = base_params(rate)
    params["tx_rate"] = rate
    params["rx_rate"] = rate
    params["tx_channels"] = "0"
    params["rx_channels"] = "0"
    run_test(path, params, iterations, "1xTRX @{}".format(rate))

    # Run TRX at 153.6 Msps with two channels
    rate = "153.6e6"
    params = base_params(rate)
    params["tx_rate"] = rate
    params["rx_rate"] = rate
    params["tx_channels"] = "0,1"
    params["rx_channels"] = "0,1"
    run_test(path, params, iterations, "2xTRX @{}".format(rate))

    # Run TRX at 125 Msps with two channels
    rate = "125e6"
    params = base_params(rate)
    params["tx_rate"] = rate
    params["rx_rate"] = rate
    params["tx_channels"] = "0,1"
    params["rx_channels"] = "0,1"
    run_test(path, params, iterations, "2xTRX @{}".format(rate))

    # Run RX at 62.5 Msps with four channels
    rate = "62.5e6"
    params = base_params("125e6")
    params["rx_rate"] = rate
    params["rx_channels"] = "0,1,2,3"
    run_test(path, params, iterations, "4xRX @{}".format(rate))

    # Run TX at 62.5 Msps with four channels
    rate = "62.5e6"
    params = base_params("125e6")
    params["tx_rate"] = rate
    params["tx_channels"] = "0,1,2,3"
    run_test(path, params, iterations, "4xTX @{}".format(rate))

    # Run TRX at 62.5 Msps with four channels
    rate = "62.5e6"
    params = base_params("125e6")
    params["tx_rate"] = rate
    params["rx_rate"] = rate
    params["tx_channels"] = "0,1,2,3"
    params["rx_channels"] = "0,1,2,3"
    run_test(path, params, iterations, "4xTRX @{}".format(rate))


def run_N310_tests_for_single_10G_long_duration(
        path, addr, iterations, duration, use_dpdk=False, mgmt_addr=''):
    """
    Runs tests that are in the neighborhood of max rate for 10 GbE. Only include
    a small subset of tests to run for a longer period.
    """
    def base_params(rate):
        if use_dpdk is True:
            return {
                "args": "addr={},master_clock_rate={},use_dpdk=1,mgmt_addr={},"
                        .format(addr, rate, mgmt_addr),
                "duration": duration
            }
        else:
            return {
                "args": "addr={},master_clock_rate={}".format(addr, rate),
                "duration": duration
            }

    rate = "153.6e6"

    # Run TRX at 125 Msps with two channels
    rate = "125e6"
    params = base_params(rate)
    params["tx_rate"] = rate
    params["rx_rate"] = rate
    params["tx_channels"] = "0,1"
    params["rx_channels"] = "0,1"
    run_test(path, params, iterations, "2xTRX @{}".format(rate))

    # Run TRX at 62.5 Msps with four channels
    rate = "62.5e6"
    params = base_params("125e6")
    params["tx_rate"] = rate
    params["rx_rate"] = rate
    params["tx_channels"] = "0,1,2,3"
    params["rx_channels"] = "0,1,2,3"
    run_test(path, params, iterations, "4xTRX @{}".format(rate))


def run_N310_tests_for_dual_10G(
        path, addr, second_addr, iterations,
        duration, use_dpdk=False, mgmt_addr=''):
    """
    Runs tests that are in the neighborhood of max rate for dual 10 GbE
    """
    def base_params(rate):
        if use_dpdk is True:
            return {
                "args": ("addr={},second_addr={},master_clock_rate={},"
                         "use_dpdk=1,mgmt_addr={}")
                        .format(addr, second_addr, rate, mgmt_addr),
                "duration": duration
            }
        else:
            return {
                "args": "addr={},second_addr={},master_clock_rate={}"
                        .format(addr, second_addr, rate),
                "duration": duration
            }

    # Run RX at 153.6 Msps with two channels
    rate = "153.6e6"
    params = base_params(rate)
    params["rx_rate"] = rate
    params["rx_channels"] = "0,1"
    run_test(path, params, iterations, "2xRX @{}".format(rate))

    # Run TX at 153.6 Msps with two channels
    rate = "153.6e6"
    params = base_params(rate)
    params["tx_rate"] = rate
    params["tx_channels"] = "0,1"
    run_test(path, params, iterations, "2xTX @{}".format(rate))

    # Run TRX at 153.6 Msps with two channels
    rate = "153.6e6"
    params = base_params(rate)
    params["tx_rate"] = rate
    params["rx_rate"] = rate
    params["tx_channels"] = "0,1"
    params["rx_channels"] = "0,1"
    run_test(path, params, iterations, "2xTRX @{}".format(rate))

    # Run RX at 153.6 Msps with four channels
    rate = "153.6e6"
    params = base_params(rate)
    params["rx_rate"] = rate
    params["rx_channels"] = "0,1,2,3"
    run_test(path, params, iterations, "4xRX @{}".format(rate))

    # Run TX at 153.6 Msps with four channels
    rate = "153.6e6"
    params = base_params(rate)
    params["tx_rate"] = rate
    params["tx_channels"] = "0,1,2,3"
    run_test(path, params, iterations, "4xTX @{}".format(rate))

    # Run TRX at 153.6 Msps with four channels
    rate = "153.6e6"
    params = base_params(rate)
    params["tx_rate"] = rate
    params["rx_rate"] = rate
    params["tx_channels"] = "0,1,2,3"
    params["rx_channels"] = "0,1,2,3"
    run_test(path, params, iterations, "4xTRX @{}".format(rate))

    # Run RX at 125 Msps with four channels
    rate = "125e6"
    params = base_params(rate)
    params["rx_rate"] = rate
    params["rx_channels"] = "0,1,2,3"
    run_test(path, params, iterations, "4xRX @{}".format(rate))

    # Run TX at 125 Msps with four channels
    rate = "125e6"
    params = base_params(rate)
    params["tx_rate"] = rate
    params["tx_channels"] = "0,1,2,3"
    run_test(path, params, iterations, "4xTX @{}".format(rate))

    # Run TRX at 125 Msps with four channels
    rate = "125e6"
    params = base_params(rate)
    params["tx_rate"] = rate
    params["rx_rate"] = rate
    params["tx_channels"] = "0,1,2,3"
    params["rx_channels"] = "0,1,2,3"
    run_test(path, params, iterations, "4xTRX @{}".format(rate))


def run_N310_tests_for_dual_10G_long_duration(
        path, addr, second_addr, iterations,
        duration, use_dpdk=False, mgmt_addr=''):
    """
    Runs tests that are in the neighborhood of max rate for dual 10 GbE. Only include
    a small subset of tests to run for a longer period.
    """
    def base_params(rate):
        if use_dpdk is True:
            return {
                "args": ("addr={},second_addr={},master_clock_rate={},"
                         "use_dpdk=1,mgmt_addr={}")
                         .format(addr, second_addr, rate, mgmt_addr),
                "duration": duration
            }
        else:
            return {
                "args": "addr={},second_addr={},master_clock_rate={}"
                .format(addr, second_addr, rate),
                "duration": duration
            }

    # Run TRX at 122.88 Msps with four channels
    rate = "122.88e6"
    params = base_params(rate)
    params["tx_rate"] = rate
    params["rx_rate"] = rate
    params["tx_channels"] = "0,1,2,3"
    params["rx_channels"] = "0,1,2,3"
    run_test(path, params, iterations, "4xTRX @{}".format(rate))

    # Run TRX at 125 Msps with four channels
    rate = "125e6"
    params = base_params(rate)
    params["tx_rate"] = rate
    params["rx_rate"] = rate
    params["tx_channels"] = "0,1,2,3"
    params["rx_channels"] = "0,1,2,3"
    run_test(path, params, iterations, "4xTRX @{}".format(rate))

    # Run TRX at 153.6 Msps with four channels
    rate = "153.6e6"
    params = base_params(rate)
    params["tx_rate"] = rate
    params["rx_rate"] = rate
    params["tx_channels"] = "0,1,2,3"
    params["rx_channels"] = "0,1,2,3"
    run_test(path, params, iterations, "4xTRX @{}".format(rate))

def run_N310_tests_for_Liberio_master_next(path, iterations, duration):
    """
    Runs tests that are in the neighborhood of max rate for Liberio
    """
    def base_params():
        return {
            "args" : "master_clock_rate=125e6",
            "duration" : duration
        }

    # Run RX at 4.032258 Msps with one channel
    rate = "4.032258e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0"
    run_test(path, params, iterations, "1xRX @{}".format(rate))

    # Run RX at 4.464286 Msps with one channel
    rate = "4.464286e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0"
    run_test(path, params, iterations, "1xRX @{}".format(rate))

    # Run RX at 4.807692 Msps with one channel
    rate = "4.807692e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0"
    run_test(path, params, iterations, "1xRX @{}".format(rate))

    # Run RX at 5.208333 Msps with one channel
    rate = "5.208333e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0"
    run_test(path, params, iterations, "1xRX @{}".format(rate))

    # Run TX at 11.363636 Msps with one channel
    rate = "11.363636e6"
    params = base_params()
    params["tx_rate"] = rate
    params["tx_channels"] = "0"
    run_test(path, params, iterations, "1xTX @{}".format(rate))

    # Run TX at 12.5Msps with one channel
    rate = "12.5e6"
    params = base_params()
    params["tx_rate"] = rate
    params["tx_channels"] = "0"
    run_test(path, params, iterations, "1xTX @{}".format(rate))

    # Run TX at 13.888889 Msps with one channel
    rate = "13.888889e6"
    params = base_params()
    params["tx_rate"] = rate
    params["tx_channels"] = "0"
    run_test(path, params, iterations, "1xTX @{}".format(rate))

    # Run TX at 15.625000 Msps with one channel
    rate = "15.625000e6"
    params = base_params()
    params["tx_rate"] = rate
    params["tx_channels"] = "0"
    run_test(path, params, iterations, "1xTX @{}".format(rate))

    # Run TX at 17.857143 Msps with one channel
    rate = "17.857143e6"
    params = base_params()
    params["tx_rate"] = rate
    params["tx_channels"] = "0"
    run_test(path, params, iterations, "1xTX @{}".format(rate))

    # Run TRX at 2.976190 Msps with one channel
    rate = "2.976190e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0"
    params["tx_rate"] = rate
    params["tx_channels"] = "0"
    run_test(path, params, iterations, "1xTRX @{}".format(rate))

    # Run TRX at 3.472222 Msps with one channel
    rate = "3.472222e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0"
    params["tx_rate"] = rate
    params["tx_channels"] = "0"
    run_test(path, params, iterations, "1xTRX @{}".format(rate))

    # Run TRX at 4.032258 Msps with one channel
    rate = "4.032258e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0"
    params["tx_rate"] = rate
    params["tx_channels"] = "0"
    run_test(path, params, iterations, "1xTRX @{}".format(rate))

    # Run TRX at 1.404494 Msps with two channels
    rate = "1.404494e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0,2"
    params["tx_rate"] = rate
    params["tx_channels"] = "0,2"
    run_test(path, params, iterations, "2xTRX @{}".format(rate))

    # Run TRX at 1.760563 Msps with two channels
    rate = "1.760563e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0,2"
    params["tx_rate"] = rate
    params["tx_channels"] = "0,2"
    run_test(path, params, iterations, "2xTRX @{}".format(rate))

    # Run TRX at 2.016129 Msps with two channels
    rate = "2.016129e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0,2"
    params["tx_rate"] = rate
    params["tx_channels"] = "0,2"
    run_test(path, params, iterations, "2xTRX @{}".format(rate))

    # Run 4xTRX at 0.899281 Msps with one channel
    rate = "0.899281e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0,1,2,3"
    params["tx_rate"] = rate
    params["tx_channels"] = "0,1,2,3"
    run_test(path, params, iterations, "4xTRX @{}".format(rate))

    # Run 4xTRX at 1.0 Msps with one channel
    rate = "1.0e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0,1,2,3"
    params["tx_rate"] = rate
    params["tx_channels"] = "0,1,2,3"
    run_test(path, params, iterations, "4xTRX @{}".format(rate))

    # Run 4xTRX at 1.25 Msps with one channel
    rate = "1.25e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0,1,2,3"
    params["tx_rate"] = rate
    params["tx_channels"] = "0,1,2,3"
    run_test(path, params, iterations, "4xTRX @{}".format(rate))

def run_N310_tests_for_Liberio_315(path, iterations, duration):
    """
    Runs tests that are in the neighborhood of max rate for Liberio
    """
    def base_params():
        return {
            "args" : "master_clock_rate=125e6",
            "duration" : duration
        }

    # Run RX at 8.928571 Msps with one channel
    rate = "8.928571e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0"
    run_test(path, params, iterations, "1xRX @{}".format(rate))

    # Run RX at 9.615385 Msps with one channel
    rate = "9.615385e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0"
    run_test(path, params, iterations, "1xRX @{}".format(rate))

    # Run RX at 10.416667 Msps with one channel
    rate = "10.416667e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0"
    run_test(path, params, iterations, "1xRX @{}".format(rate))

    # Run RX at 11.363636 Msps with one channel
    rate = "11.363636e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0"
    run_test(path, params, iterations, "1xRX @{}".format(rate))

    # Run TX at 11.363636 Msps with one channel
    rate = "11.363636e6"
    params = base_params()
    params["tx_rate"] = rate
    params["tx_channels"] = "0"
    run_test(path, params, iterations, "1xTX @{}".format(rate))

    # Run TX at 13.888889 Msps with one channel
    rate = "13.888889e6"
    params = base_params()
    params["tx_rate"] = rate
    params["tx_channels"] = "0"
    run_test(path, params, iterations, "1xTX @{}".format(rate))

    # Run TX at 15.625000 Msps with one channel
    rate = "15.625000e6"
    params = base_params()
    params["tx_rate"] = rate
    params["tx_channels"] = "0"
    run_test(path, params, iterations, "1xTX @{}".format(rate))

    # Run TRX at 5.952381 Msps with one channel
    rate = "5.952381e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0"
    params["tx_rate"] = rate
    params["tx_channels"] = "0"
    run_test(path, params, iterations, "1xTRX @{}".format(rate))

    # Run TRX at 6.578947 Msps with one channel
    rate = "6.578947e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0"
    params["tx_rate"] = rate
    params["tx_channels"] = "0"
    run_test(path, params, iterations, "1xTRX @{}".format(rate))

    # Run TRX at 6.944444 Msps with one channel
    rate = "6.944444e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0"
    params["tx_rate"] = rate
    params["tx_channels"] = "0"
    run_test(path, params, iterations, "1xTRX @{}".format(rate))

    # Run TRX at 6.944444 Msps with one channel
    rate = "6.944444e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0"
    params["tx_rate"] = rate
    params["tx_channels"] = "0"
    run_test(path, params, iterations, "1xTRX @{}".format(rate))

    # Run TRX at 7.812500 Msps with one channel
    rate = "7.812500e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0"
    params["tx_rate"] = rate
    params["tx_channels"] = "0"
    run_test(path, params, iterations, "1xTRX @{}".format(rate))

    # Run TRX at 8.333333 Msps with one channel
    rate = "8.333333e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0"
    params["tx_rate"] = rate
    params["tx_channels"] = "0"
    run_test(path, params, iterations, "1xTRX @{}".format(rate))

    # Run TRX at 8.928571 Msps with one channel
    rate = "8.928571e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0"
    params["tx_rate"] = rate
    params["tx_channels"] = "0"
    run_test(path, params, iterations, "1xTRX @{}".format(rate))

    # Run 2xTRX at 0.5 Msps with one channel
    rate = "0.5e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0,2"
    params["tx_rate"] = rate
    params["tx_channels"] = "0,2"
    run_test(path, params, iterations, "2xTRX @{}".format(rate))

    # Run 2xTRX at 0.600962 Msps with one channel
    rate = "0.600962e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0,2"
    params["tx_rate"] = rate
    params["tx_channels"] = "0,2"
    run_test(path, params, iterations, "2xTRX @{}".format(rate))

    # Run 2xTRX at 0.702247 Msps with one channel
    rate = "0.702247e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0,2"
    params["tx_rate"] = rate
    params["tx_channels"] = "0,2"
    run_test(path, params, iterations, "2xTRX @{}".format(rate))

    # Run 2xTRX at 0.801282 Msps with one channel
    rate = "0.801282e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0,2"
    params["tx_rate"] = rate
    params["tx_channels"] = "0,2"
    run_test(path, params, iterations, "2xTRX @{}".format(rate))

    # Run 4xTRX at 0.244141 Msps with one channel
    rate = "0.244141e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0,1,2,3"
    params["tx_rate"] = rate
    params["tx_channels"] = "0,1,2,3"
    run_test(path, params, iterations, "4xTRX @{}".format(rate))

def run_N320_tests_for_Liberio_315(path, iterations, duration):
    """
    Runs tests that are in the neighborhood of max rate for Liberio
    """
    def base_params():
        return {
            "args" : "master_clock_rate=200e6",
            "duration" : duration
        }

    # Run RX at 6.896552 Msps with one channel
    rate = "6.896552e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0"
    run_test(path, params, iterations, "1xRX @{}".format(rate))

    # Run RX at 8 Msps with one channel
    rate = "8.0e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0"
    run_test(path, params, iterations, "1xRX @{}".format(rate))

    # Run RX at 9.090909 Msps with one channel
    rate = "9.090909e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0"
    run_test(path, params, iterations, "1xRX @{}".format(rate))

    # Run RX at 10 Msps with one channel
    rate = "10.0e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0"
    run_test(path, params, iterations, "1xRX @{}".format(rate))

    # Run TX at 2 Msps with one channel
    rate = "2.0e6"
    params = base_params()
    params["tx_rate"] = rate
    params["tx_channels"] = "0"
    run_test(path, params, iterations, "1xTX @{}".format(rate))

    # Run TX at 2.985075 Msps with one channel
    rate = "2.985075e6"
    params = base_params()
    params["tx_rate"] = rate
    params["tx_channels"] = "0"
    run_test(path, params, iterations, "1xTX @{}".format(rate))

    # Run TX at 4 Msps with one channel
    rate = "4.0e6"
    params = base_params()
    params["tx_rate"] = rate
    params["tx_channels"] = "0"
    run_test(path, params, iterations, "1xTX @{}".format(rate))

    # Run TX at 6.060606 Msps with one channel
    rate = "6.060606e6"
    params = base_params()
    params["tx_rate"] = rate
    params["tx_channels"] = "0"
    run_test(path, params, iterations, "1xTX @{}".format(rate))

    # Run TX at 6.896552 Msps with one channel
    rate = "6.896552e6"
    params = base_params()
    params["tx_rate"] = rate
    params["tx_channels"] = "0"
    run_test(path, params, iterations, "1xTX @{}".format(rate))

    # Run TRX at 2.985075 Msps with one channel
    rate = "2.985075e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0"
    params["tx_rate"] = rate
    params["tx_channels"] = "0"
    run_test(path, params, iterations, "1xTRX @{}".format(rate))

    # Run TRX at 4 Msps with one channel
    rate = "4.0e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0"
    params["tx_rate"] = rate
    params["tx_channels"] = "0"
    run_test(path, params, iterations, "1xTRX @{}".format(rate))

    # Run TRX at 5 Msps with one channel
    rate = "5.0e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0"
    params["tx_rate"] = rate
    params["tx_channels"] = "0"
    run_test(path, params, iterations, "1xTRX @{}".format(rate))

    # Run TRX at 0.5 Msps with one channel
    rate = "0.5e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0,1"
    params["tx_rate"] = rate
    params["tx_channels"] = "0,1"
    run_test(path, params, iterations, "2xTRX @{}".format(rate))

    # Run TRX at 0.75 Msps with one channel
    rate = "0.75e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0,1"
    params["tx_rate"] = rate
    params["tx_channels"] = "0,1"
    run_test(path, params, iterations, "2xTRX @{}".format(rate))

    # Run TRX at 1.0 Msps with one channel
    rate = "1.0e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0,1"
    params["tx_rate"] = rate
    params["tx_channels"] = "0,1"
    run_test(path, params, iterations, "2xTRX @{}".format(rate))

def run_N320_tests_for_Liberio_master_next(path, iterations, duration):
    """
    Runs tests that are in the neighborhood of max rate for Liberio
    """
    def base_params():
        return {
            "args" : "master_clock_rate=200e6",
            "duration" : duration
        }


    # Run RX at 2 Msps with one channel
    rate = "2e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0"
    run_test(path, params, iterations, "1xRX @{}".format(rate))

    # Run RX at 2.985075 Msps with one channel
    rate = "2.985075e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0"
    run_test(path, params, iterations, "1xRX @{}".format(rate))

    # Run RX at 4 Msps with one channel
    rate = "4e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0"
    run_test(path, params, iterations, "1xRX @{}".format(rate))

    # Run TX at 2.985075 Msps with one channel
    rate = "2.985075e6"
    params = base_params()
    params["tx_rate"] = rate
    params["tx_channels"] = "0"
    run_test(path, params, iterations, "1xTX @{}".format(rate))

    # Run TX at 4 Msps with one channel
    rate = "4.0e6"
    params = base_params()
    params["tx_rate"] = rate
    params["tx_channels"] = "0"
    run_test(path, params, iterations, "1xTX @{}".format(rate))

    # Run TX at 5 Msps with one channel
    rate = "5.0e6"
    params = base_params()
    params["tx_rate"] = rate
    params["tx_channels"] = "0"
    run_test(path, params, iterations, "1xTX @{}".format(rate))

    # Run TX at 6.060606 Msps with one channel
    rate = "6.060606e6"
    params = base_params()
    params["tx_rate"] = rate
    params["tx_channels"] = "0"
    run_test(path, params, iterations, "1xTX @{}".format(rate))

    # Run TX at 6.896552 Msps with one channel
    rate = "6.896552e6"
    params = base_params()
    params["tx_rate"] = rate
    params["tx_channels"] = "0"
    run_test(path, params, iterations, "1xTX @{}".format(rate))

    # Run TRX at 2 Msps with one channel
    rate = "2e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0"
    params["tx_rate"] = rate
    params["tx_channels"] = "0"
    run_test(path, params, iterations, "1xTRX @{}".format(rate))

    # Run TRX at 2.985075 Msps with one channel
    rate = "2.985075e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0"
    params["tx_rate"] = rate
    params["tx_channels"] = "0"
    run_test(path, params, iterations, "1xTRX @{}".format(rate))

    # Run TRX at 4 Msps with one channel
    rate = "4.0e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0"
    params["tx_rate"] = rate
    params["tx_channels"] = "0"
    run_test(path, params, iterations, "1xTRX @{}".format(rate))

    # Run TRX at 5 Msps with one channel
    rate = "5.0e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0"
    params["tx_rate"] = rate
    params["tx_channels"] = "0"
    run_test(path, params, iterations, "1xTRX @{}".format(rate))

    # Run TRX at 0.5 Msps with two channelS
    rate = "0.5e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0,1"
    params["tx_rate"] = rate
    params["tx_channels"] = "0,1"
    run_test(path, params, iterations, "2xTRX @{}".format(rate))

    # Run TRX at 0.75 Msps with two channels
    rate = "0.75e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0,1"
    params["tx_rate"] = rate
    params["tx_channels"] = "0,1"
    run_test(path, params, iterations, "2xTRX @{}".format(rate))

    # Run TRX at 1.0 Msps with two channels
    rate = "1.0e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0,1"
    params["tx_rate"] = rate
    params["tx_channels"] = "0,1"
    run_test(path, params, iterations, "2xTRX @{}".format(rate))

    # Run TRX at 1.25 Msps with two channels
    rate = "1.25e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0,1"
    params["tx_rate"] = rate
    params["tx_channels"] = "0,1"
    run_test(path, params, iterations, "2xTRX @{}".format(rate))

    # Run TRX at 1.5 Msps with two channels
    rate = "1.5e6"
    params = base_params()
    params["rx_rate"] = rate
    params["rx_channels"] = "0,1"
    params["tx_rate"] = rate
    params["tx_channels"] = "0,1"
    run_test(path, params, iterations, "2xTRX @{}".format(rate))


def run_N320_tests_for_single_10G(
        path, addr, iterations, duration, use_dpdk=False, mgmt_addr=''):
    """
    Runs tests that are in the neighborhood of max rate for single 10 GbE
    """
    def base_params(rate):
        if use_dpdk is True:
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

    # Run RX at 250 Msps with one channel
    rate = "250e6"
    params = base_params(rate)
    params["rx_rate"] = rate
    params["rx_channels"] = "0"
    run_test(path, params, iterations, "1xRX @{}".format(rate))

    # Run TX at 250 Msps with one channel
    rate = "250e6"
    params = base_params(rate)
    params["tx_rate"] = rate
    params["tx_channels"] = "0"
    run_test(path, params, iterations, "1xTX @{}".format(rate))

    # Run TRX at 250 Msps with one channel
    rate = "250e6"
    params = base_params(rate)
    params["tx_rate"] = rate
    params["tx_channels"] = "0"
    params["rx_rate"] = rate
    params["rx_channels"] = "0"
    run_test(path, params, iterations, "1xTRX @{}".format(rate))


def run_N320_tests_for_dual_10G(
        path, addr, second_addr, iterations,
        duration, use_dpdk=False, mgmt_addr=''):
    """
    Runs tests that are in the neighborhood of max rate for dual 10 GbE
    """
    def base_params(rate):
        if use_dpdk is True:
            return {
                "args": ("addr={},second_addr={},master_clock_rate={},"
                        "use_dpdk=1,mgmt_addr={}")
                        .format(addr, second_addr, rate, mgmt_addr),
                "duration": duration
            }
        else:
            return {
                "args": "addr={},second_addr={},master_clock_rate={}"
                .format(addr, second_addr, rate),
                "duration": duration
            }

    # Run RX at 250 Msps with two channels
    rate = "250e6"
    params = base_params(rate)
    params["rx_rate"] = rate
    params["rx_channels"] = "0,1"
    run_test(path, params, iterations, "2xRX @{}".format(rate))

    # Run TX at 250 Msps with two channels
    rate = "250e6"
    params = base_params(rate)
    params["tx_rate"] = rate
    params["tx_channels"] = "0,1"
    run_test(path, params, iterations, "2xTX @{}".format(rate))

    # Run TRX at 250 Msps with two channels
    rate = "250e6"
    params = base_params(rate)
    params["tx_rate"] = rate
    params["rx_rate"] = rate
    params["tx_channels"] = "0,1"
    params["rx_channels"] = "0,1"
    run_test(path, params, iterations, "2xTRX @{}".format(rate))


def main():
    path, test_type, addr, second_addr, mgmt_addr, use_dpdk = parse_args()
    start_time = time.time()

    if test_type == Test_Type_N310_XG:
        run_N310_tests_for_single_10G(
            path, addr, 10, 30, use_dpdk, mgmt_addr)
        run_N310_tests_for_dual_10G(
            path, addr, second_addr, 10, 30, use_dpdk, mgmt_addr)

        run_N310_tests_for_single_10G_long_duration(
            path, addr, 2, 600, use_dpdk, mgmt_addr)
        run_N310_tests_for_dual_10G_long_duration(
            path, addr, second_addr, 2, 600, use_dpdk, mgmt_addr)

    if test_type == Test_Type_N310_Liberio:
        #run_N310_tests_for_Liberio_315(path, 10, 30)
        run_N310_tests_for_Liberio_master_next(path, 10, 30)

    if test_type == Test_Type_N320_Liberio:
        #run_N320_tests_for_Liberio_315(path, 10, 30)
        run_N320_tests_for_Liberio_master_next(path, 10, 30)

    if test_type == Test_Type_N320_XG:
        run_N320_tests_for_single_10G(
            path, addr, 10, 30, use_dpdk, mgmt_addr)
        run_N320_tests_for_dual_10G(
            path, addr, second_addr, 10, 30, use_dpdk, mgmt_addr)

    end_time = time.time()
    elapsed = end_time - start_time
    print("Elapsed time: {}".format(datetime.timedelta(seconds=elapsed)))
    return True

if __name__ == "__main__":
    sys.exit(not main())
