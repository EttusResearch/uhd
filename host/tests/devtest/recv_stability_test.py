#!/usr/bin/env python3
#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
""" Test recv, including some corner cases. """

import sys
import time
import argparse
import numpy as np
import uhd
try:
    from ruamel import yaml
except:
    import yaml


def run_recv_stability_test(usrp, device_config):
    """
    Run tests related to recv().
    """
    num_chans = device_config.get('num_channels', usrp.get_rx_num_channels())
    str_args = uhd.usrp.StreamArgs(
        device_config.get('cpu_type', 'fc32'),
        device_config.get('cpu_type', 'sc16'),
    )
    str_args.channels = list(range(num_chans))
    rx_streamer = usrp.get_rx_stream(str_args)
    usrp.set_rx_rate(device_config.get('rate', 1e6))
    # Run tests
    run_choke_test(usrp, rx_streamer, device_config)
    return True

def run_choke_test(usrp, rx_streamer, device_config):
    """
    This will kick off a continuous stream, then interrupt it, then resume it.
    We verify that:
    - We get an overrun message (metadata)
    - The stream resumes.
    """

    bufsize = 100 * rx_streamer.get_max_num_samps()
    recv_buf = np.zeros(
        (rx_streamer.get_num_channels(), bufsize), dtype=np.complex64)
    stream_cmd = uhd.types.StreamCMD(uhd.types.StreamMode.start_cont)
    stream_cmd.stream_now = False
    init_delay = device_config.get('init_delay', 1.0)
    stream_cmd.time_spec = usrp.get_time_now() + init_delay
    rx_streamer.issue_stream_cmd(stream_cmd)
    metadata = uhd.types.RXMetadata()
    long_timeout = init_delay + 1.0
    num_samps_recvd = rx_streamer.recv(recv_buf, metadata, timeout=long_timeout)
    test_pass = True
    # Large timeout, small rate: This should not fail
    if num_samps_recvd != bufsize:
        test_pass = False
        print(f"run_choke_test(): First buffer recv() failed, rx'd "
              f"{num_samps_recvd}/{bufsize} samples!")
    if metadata.error_code != uhd.types.RXMetadataErrorCode.none:
        test_pass = False
        print(
            f"run_choke_test(): First buffer recv() failed, rx'd "
            f"error code: {metadata.strerror()}")
    print("Choking RX...")
    time.sleep(1)
    print("Now, collecting the overrun (you should see an 'O' now).")
    # On one of the next recv(), we should get an overrun. It may take a bit,
    # because there will already be data in the pipe and depending on the USRP,
    # overruns will be inline.
    max_num_samps_before_o = device_config.get('max_num_samps_before_o', 1000000)
    num_samps_recvd = 0
    overrun_recvd = False
    while num_samps_recvd < max_num_samps_before_o:
        num_samps_recvd += rx_streamer.recv(recv_buf, metadata, timeout=.1)
        if metadata.error_code == uhd.types.RXMetadataErrorCode.overflow:
            overrun_recvd = True
            break
        if metadata.error_code != uhd.types.RXMetadataErrorCode.none:
            test_pass = False
            print(
                f"run_choke_test(): Second buffer recv() failed, rx'd "
                f"error code: {metadata.strerror()} (should have been overflow or none)")
    if not overrun_recvd:
        test_pass = False
        print(
            f"run_choke_test(): Second buffer recv() failed, never rx'd "
            f"an overflow")
    # It should recover now:
    num_samps_recvd = rx_streamer.recv(recv_buf, metadata, timeout=long_timeout)
    if num_samps_recvd != bufsize:
        test_pass = False
        print(
            f"run_choke_test(): Third buffer recv() failed, rx'd "
            f"{num_samps_recvd}/{bufsize} samples!")
    if metadata.error_code != uhd.types.RXMetadataErrorCode.none:
        test_pass = False
        print(
            f"run_choke_test(): Third buffer recv() failed, rx'd "
            f"error code: {metadata.strerror()}")
    stop_and_flush(rx_streamer)
    if not test_pass:
        raise RuntimeError("run_choke_test(): Test failed.")

def stop_and_flush(rx_streamer):
    """
    Utility to stop a streamer and clear the FIFO.
    """
    print("Flushing FIFOs...")
    stream_cmd = uhd.types.StreamCMD(uhd.types.StreamMode.stop_cont)
    bufsize = rx_streamer.get_max_num_samps()
    recv_buf = np.zeros(
        (rx_streamer.get_num_channels(), bufsize), dtype=np.complex64)
    rx_streamer.issue_stream_cmd(stream_cmd)
    metadata = uhd.types.RXMetadata()
    stop_time = time.monotonic() + 5.0
    while time.monotonic() < stop_time:
        rx_streamer.recv(recv_buf, metadata, timeout=0)
        if metadata.error_code == uhd.types.RXMetadataErrorCode.timeout:
            break

def parse_args():
    """
    Parse args.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--args', default='',
    )
    parser.add_argument(
        '--dump-defaults',
        help="Specify a device type, and the default config will be dumped as YAML"
    )
    parser.add_argument(
        '--device-config',
        help="Specify path to YAML file to use as device config"
    )
    return parser.parse_args()

def get_device_config(usrp_type, device_config_path=None):
    """
    Return a device configuration object.
    """
    if device_config_path:
        with open(device_config_path, 'r') as yaml_f:
            return yaml.load(yaml_f)
    return {}

def dump_defaults(usrp_type):
    """
    Print the hard-coded defaults as YAML
    """
    defaults = get_device_config(usrp_type)
    print(yaml.dump(defaults, default_flow_style=False))

def main():
    """
    Returns True on Success
    """
    args = parse_args()
    if args.dump_defaults:
        dump_defaults(args.dump_defaults)
        return 0
    usrp = uhd.usrp.MultiUSRP(args.args)
    usrp_type = usrp.get_usrp_rx_info().get('mboard_id')
    device_config = get_device_config(usrp_type, args.device_config)
    ret_val = run_recv_stability_test(usrp, device_config)
    if ret_val != 1:
        raise Exception("Python API Tester Received Errors")
    return ret_val

if __name__ == "__main__":
    sys.exit(not main())
