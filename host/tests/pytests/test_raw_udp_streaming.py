import pytest
from pathlib import Path
import test_length_utils
import test_nic_utils
from test_length_utils import Test_Length_Smoke, Test_Length_Full, Test_Length_Stress
from test_nic_utils import get_nic_statistics

ARGNAMES_DUAL_SFP = ["dual_SFP", "rate", "rx_rate", "rx_channels", "dest_addr", "dest_port", "adapter", "host_interface", "keep_hdr"]

def parametrize_test_length(metafunc, test_length, fast_params, stress_params):
    argnames = ["iterations", "duration"]

    # select how long to run tests
    if(test_length == Test_Length_Smoke or test_length == Test_Length_Full):
        argvalues = [
            #            iterations                duration                test case ID
            #            ------------------------------------------------------------
            pytest.param(fast_params.iterations,   fast_params.duration,   id="fast"),
        ]
    elif(test_length == Test_Length_Stress):
        argvalues = [
            #            iterations                duration                test case ID
            #            ----------------------------------------------------------
            pytest.param(stress_params.iterations, stress_params.duration, id="stress"),
        ]

    metafunc.parametrize(argnames, argvalues)


def generate_x4xx_10GbE_test_cases(metafunc, test_length):
    test_cases = [
        # Test Lengths                                         dual_SFP  rate     rx_rate  rx_channels dest_addr        dest_port   adapter  host_interface   keep_hdr test case ID
        # --------------------------------------------------------------------------------------------------------------------------------------------------------
        [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,     245.76e6, 122.88e6, "0",   "192.168.110.1",    1234,      "sfp0",     "enp1s0f0",  True,    id="SFP0_FULL_PACKET_1x10GbE-1xRX@122.88e6")],
        [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,     245.76e6, 122.88e6, "1",   "192.168.110.1",    1234,      "sfp0",     "enp1s0f0",  False,   id="SFP0_RAW_PAYLOAD_1x10GbE-1xRX@122.88e6")],
        [{},                                      pytest.param(False,     245.76e6, 122.88e6, "2",   "192.168.110.1",    1234,      "sfp0",     "enp1s0f0",  True,    id="SFP0_FULL_PACKET_1x10GbE-1xRX@122.88e6")],
        [{},                                      pytest.param(False,     245.76e6, 122.88e6, "3",   "192.168.110.1",    1234,      "sfp0",     "enp1s0f0",  False,   id="SFP0_RAW_PAYLOAD_1x10GbE-1xRX@122.88e6")],
        [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,     245.76e6, 122.88e6, "0",   "192.168.111.1",    1234,      "sfp0_1",   "enp1s0f1",  True,    id="SFP0_1_FULL_PACKET_1x10GbE-1xRX@122.88e6")],
        [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,     245.76e6, 122.88e6, "1",   "192.168.111.1",    1234,      "sfp0_1",   "enp1s0f1",  False,   id="SFP0_1_RAW_PAYLOAD_1x10GbE-1xRX@122.88e6")],
        [{},                                      pytest.param(False,     245.76e6, 122.88e6, "2",   "192.168.111.1",    1234,      "sfp0_1",   "enp1s0f1",  True,    id="SFP0_1_FULL_PACKET_1x10GbE-1xRX@122.88e6")],
        [{},                                      pytest.param(False,     245.76e6, 122.88e6, "3",   "192.168.111.1",    1234,      "sfp0_1",   "enp1s0f1",  False,   id="SFP0_1_RAW_PAYLOAD_1x10GbE-1xRX@122.88e6")],
    ]

    argvalues = test_length_utils.select_test_cases_by_length(test_length, test_cases)
    metafunc.parametrize(ARGNAMES_DUAL_SFP, argvalues)

    fast_params = test_length_utils.test_length_params(iterations=2, duration=10)
    stress_params = test_length_utils.test_length_params(iterations=2, duration=600)
    parametrize_test_length(metafunc, test_length, fast_params, stress_params)

def generate_x4xx_100GbE_test_cases(metafunc, test_length):
    test_cases = [
        # Test Lengths                                         dual_SFP  rate     rx_rate  rx_channels dest_addr        dest_port   adapter  host_interface   keep_hdr test case ID
        # --------------------------------------------------------------------------------------------------------------------------------------------------------
        [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,     491.52e6, 491.52e6, "0",   "192.168.110.1",    1234,      "sfp0",     "ens6f0",  True,    id="SFP0_FULL_PACKET_1x100GbE-1xRX@491.52e6")],
        [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,     491.52e6, 491.52e6, "1",   "192.168.110.1",    1234,      "sfp0",     "ens6f0",  False,   id="SFP0_RAW_PAYLOAD_1x100GbE-1xRX@491.52e6")],
        [{},                                      pytest.param(False,     491.52e6, 491.52e6, "2",   "192.168.110.1",    1234,      "sfp0",     "ens6f0",  True,    id="SFP0_FULL_PACKET_1x100GbE-1xRX@491.52e6")],
        [{},                                      pytest.param(False,     491.52e6, 491.52e6, "3",   "192.168.110.1",    1234,      "sfp0",     "ens6f0",  False,   id="SFP0_RAW_PAYLOAD_1x100GbE-1xRX@491.52e6")],
        [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,     491.52e6, 491.52e6, "0",   "192.168.120.1",    1234,      "sfp1",     "ens6f1",  True,    id="SFP1_FULL_PACKET_1x100GbE-1xRX@491.52e6")],
        [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,     491.52e6, 491.52e6, "1",   "192.168.120.1",    1234,      "sfp1",     "ens6f1",  False,   id="SFP1_RAW_PAYLOAD_1x100GbE-1xRX@491.52e6")],
        [{},                                      pytest.param(False,     491.52e6, 491.52e6, "2",   "192.168.120.1",    1234,      "sfp1",     "ens6f1",  True,    id="SFP1_FULL_PACKET_1x100GbE-1xRX@491.52e6")],
        [{},                                      pytest.param(False,     491.52e6, 491.52e6, "3",   "192.168.120.1",    1234,      "sfp1",     "ens6f1",  False,   id="SFP1_RAW_PAYLOAD_1x100GbE-1xRX@491.52e6")],
    ]

    argvalues = test_length_utils.select_test_cases_by_length(test_length, test_cases)
    metafunc.parametrize(ARGNAMES_DUAL_SFP, argvalues)

    fast_params = test_length_utils.test_length_params(iterations=5, duration=30)
    stress_params = test_length_utils.test_length_params(iterations=2, duration=600)
    parametrize_test_length(metafunc, test_length, fast_params, stress_params)

def generate_X310_10GbE_test_cases(metafunc, test_length):
    test_cases = [
        # Test Lengths                                        dual_SFP  rate  rx_rate  rx_channels dest_addr        dest_port   adapter  host_interface   keep_hdr test case ID
        # --------------------------------------------------------------------------------------------------------------------------------------------------------
        [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,   200e6, 200e6, "0",      "192.168.10.1",     1234,      "sfp0",     "ens4f0",        True,    id="SFP0_FULL_PACKET_1x10GbE-1xRX@200e6")],
        [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,   200e6, 200e6, "1",      "192.168.10.1",     1234,      "sfp0",     "ens4f0",        False,   id="SFP0_RAW_PAYLOAD_1x10GbE-1xRX@200e6")],
        [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,   200e6, 200e6, "0",      "192.168.20.1",     1234,      "sfp1",     "ens4f1",        True,    id="SFP1_FULL_PACKET_1x10GbE-1xRX@200e6")],
        [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,   200e6, 200e6, "1",      "192.168.20.1",     1234,      "sfp1",     "ens4f1",        False,   id="SFP1_RAW_PAYLOAD_1x10GbE-1xRX@200e6")],
    ]

    argvalues = test_length_utils.select_test_cases_by_length(test_length, test_cases)
    metafunc.parametrize(ARGNAMES_DUAL_SFP, argvalues)

    fast_params = test_length_utils.test_length_params(iterations=5, duration=30)
    stress_params = test_length_utils.test_length_params(iterations=2, duration=600)
    parametrize_test_length(metafunc, test_length, fast_params, stress_params)

def pytest_generate_tests(metafunc):
    dut_type = metafunc.config.getoption("dut_type")
    dut_fpga = metafunc.config.getoption("dut_fpga")
    test_length = metafunc.config.getoption("test_length")

    metafunc.parametrize("dut_type", [dut_type])

    if dut_type.lower() == 'x4xx' and dut_fpga.upper() == 'X4_200':
        generate_x4xx_10GbE_test_cases(metafunc, test_length)
    if dut_type.lower() == 'x4xx' and dut_fpga.upper() == 'CG_400':
        generate_x4xx_100GbE_test_cases(metafunc, test_length)
    if dut_type.lower() == 'x310' and dut_fpga.upper() == 'XG':
        generate_X310_10GbE_test_cases(metafunc, test_length)


def test_raw_udp_streaming(pytestconfig, dut_type, dual_SFP, rate, rx_rate, rx_channels,
                    dest_addr, dest_port, adapter, host_interface, keep_hdr, iterations, duration):

    remote_rx_path = Path(pytestconfig.getoption('uhd_build_dir')) / 'examples/python/remote_rx.py'

    device_args = ""

    # construct device args string
    if dut_type.lower() in ['n310', 'n320', 'e320', 'x4xx', 'x310']:
        device_args += f"master_clock_rate={rate},"

    addr = pytestconfig.getoption('addr')
    second_addr = pytestconfig.getoption('second_addr')
    if addr:
        device_args += f"addr={addr},"

    if second_addr:
        device_args += f"second_addr={second_addr},"

    mgmt_addr = pytestconfig.getoption('mgmt_addr')
    if mgmt_addr:
        device_args += f"mgmt_addr={mgmt_addr},"

    print("Constructed device_args: " + device_args)

    # construct remote_rx params dictionary
    remote_rx_params = {
        "args": device_args,
        "duration": duration,
        "rate": rx_rate,
        "channels":  rx_channels,
        "dest-addr":  dest_addr,
        "dest-port": dest_port,
        "adapter": adapter,
        "freq": 1e9,
    }

    if keep_hdr:
        remote_rx_params["keep-hdr"] = ""

    # run remote streaming
    stats = run_remote_rx(remote_rx_params, remote_rx_path, iterations, host_interface, True)

    dut_fpga = pytestconfig.getoption('dut_fpga')
    chdr_hdr_size = 64 #100GBE default
    if dut_fpga:
        if dut_fpga.upper() == "CG_400":
            chdr_hdr_size = 64
        if dut_fpga.upper() in {"X4_200", "XG"}:
            chdr_hdr_size = 16
    analyze_stats(stats, remote_rx_params, chdr_hdr_size)

def analyze_stats(stats, remote_rx_params, chdr_hdr_size):
    ALLOWED_LOSS = 0.1 #percentage, arbitrary limit for now

    def get_expected_data(remote_rx_params, no_of_packets, chdr_hdr_size):
        # Our calculation here is approx only. Since we are not monitoring each
        # packet's size and making these calculations.
        ETH_HDR_SIZE = 42 #bytes referenced from Wireshark experiments and docs.
        rate = remote_rx_params["rate"]
        no_of_channels = remote_rx_params["channels"].count(" ") + 1 #channels are space separated.
        bytes_per_sample = 4 #sc16
        duration = remote_rx_params["duration"]
        expected_data_rate_on_host_interface = rate * no_of_channels * bytes_per_sample #bytes per second
        expected_raw_rf_data = expected_data_rate_on_host_interface * duration # bytes
        # Approx since there might be some non-data packets received on the SFP port.
        expected_data = expected_raw_rf_data + no_of_packets * ETH_HDR_SIZE
        if 'keep-hdr' in remote_rx_params.keys():
            expected_data = expected_data + no_of_packets * chdr_hdr_size

        return expected_data

    def get_analyzed_stats(stats):
        analyzed_stats = []
        for [stats_before_run, stats_after_run] in stats:
            expected_data = get_expected_data(remote_rx_params, stats_after_run.packets_recv - stats_before_run.packets_recv, chdr_hdr_size)
            received_data = stats_after_run.bytes_recv - stats_before_run.bytes_recv
            if (expected_data != 0):
                analyzed_stats.append([expected_data, received_data, abs(100 * (received_data - expected_data) / expected_data)])
            else:
                raise RuntimeError("Expected Data is of zero size! Modify rate or duration params!")
        return analyzed_stats

    def print_analyzed_stats(analyzed_stats):
        header = "| expected_data | received_data | deviation(%)  |"
        ruler  = "|---------------|---------------|---------------|"
        s = header + "\n" + ruler + "\n"
        for [expected_data, received_data, deviation] in analyzed_stats:
            row = (
                "| {:>15} ".format(expected_data) +
                "| {:>15} ".format(received_data) +
                "| {:>15.3f} ".format(deviation)
            )
            s += row + "|" + "\n"

        print(s)

    # Basic check
    for [stats_before_run, stats_after_run] in stats:
        assert (stats_after_run.bytes_recv > stats_before_run.bytes_recv)

    # Actual check
    analyzed_stats = get_analyzed_stats(stats)
    print_analyzed_stats(analyzed_stats)
    for [_, _, deviation] in analyzed_stats:
        assert(deviation < ALLOWED_LOSS)


def run_remote_rx(remote_rx_params, remote_rx_path, iterations, host_interface, stop_on_error=True):
    """
    Runs remote_rx multiple times
    """
    import subprocess
    import re

    stats = []

    print("Running remote_rx {} times with the following arguments: ".format(iterations))
    proc_params = [remote_rx_path]
    for key, val in remote_rx_params.items():
        proc_params.append("--" + str(key))
        if (str(key) != "keep-hdr"):
            proc_params.append(str(val))

    print(proc_params)

    iteration = 0
    while iteration < iterations:
        stats_before_run = get_nic_statistics(host_interface)
        proc = subprocess.run(proc_params, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        match = re.search("Streaming complete. Exiting.", proc.stdout.decode('ASCII'))
        if match is None:
            if stop_on_error:
                msg = "Exception occurred while running remote_rx\n"
                msg += "remote_rx arguments:\n"
                msg += str(proc.args) + "\n"
                msg += "Stderr capture:\n"
                msg += proc.stderr.decode('ASCII')
                msg += "Stdout capture:\n"
                msg += proc.stdout.decode('ASCII')
                raise RuntimeError(msg)

        stats_after_run = get_nic_statistics(host_interface)
        stats.append([stats_before_run, stats_after_run])
        iteration += 1

    return stats
