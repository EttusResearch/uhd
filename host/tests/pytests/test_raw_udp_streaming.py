import pytest
import re
import util_test_length

from pathlib import Path
from util_test_nic import get_nic_statistics
from util_test_length import Test_Length_Full, Test_Length_Smoke, Test_Length_Stress

# fmt: off

ARGNAMES_DUAL_SFP = [
    "dual_sfp",
    "rate",
    "rx_rate",
    "rx_channels",
    "dest_addr",
    "dest_port",
    "adapter",
    "host_interface",
    "keep_hdr",
]

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


def generate_x410_10GbE_test_cases(metafunc, test_length, sfp_int0, sfp_int1, dest_addr0, dest_addr1):
    test_cases = [
        # Test Lengths                                         dual_sfp  rate     rx_rate  rx_channels dest_addr        dest_port   adapter  host_interface   keep_hdr test case ID
        # --------------------------------------------------------------------------------------------------------------------------------------------------------
        [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,     245.76e6, 122.88e6, "0",   dest_addr0,    1234,      "sfp0",     sfp_int0,    True,    id="SFP0_FULL_PACKET_1x10GbE-1xRX@122.88e6")],
        [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,     245.76e6, 122.88e6, "1",   dest_addr0,    1234,      "sfp0",     sfp_int0,    False,   id="SFP0_RAW_PAYLOAD_1x10GbE-1xRX@122.88e6")],
        [{},                                      pytest.param(False,     245.76e6, 122.88e6, "2",   dest_addr0,    1234,      "sfp0",     sfp_int0,    True,    id="SFP0_FULL_PACKET_1x10GbE-1xRX@122.88e6")],
        [{},                                      pytest.param(False,     245.76e6, 122.88e6, "3",   dest_addr0,    1234,      "sfp0",     sfp_int0,    False,   id="SFP0_RAW_PAYLOAD_1x10GbE-1xRX@122.88e6")],
        [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,     245.76e6, 122.88e6, "0",   dest_addr1,    1234,      "sfp0_1",   sfp_int1,    True,    id="SFP0_1_FULL_PACKET_1x10GbE-1xRX@122.88e6")],
        [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,     245.76e6, 122.88e6, "1",   dest_addr1,    1234,      "sfp0_1",   sfp_int1,    False,   id="SFP0_1_RAW_PAYLOAD_1x10GbE-1xRX@122.88e6")],
        [{},                                      pytest.param(False,     245.76e6, 122.88e6, "2",   dest_addr1,    1234,      "sfp0_1",   sfp_int1,    True,    id="SFP0_1_FULL_PACKET_1x10GbE-1xRX@122.88e6")],
        [{},                                      pytest.param(False,     245.76e6, 122.88e6, "3",   dest_addr1,    1234,      "sfp0_1",   sfp_int1,    False,   id="SFP0_1_RAW_PAYLOAD_1x10GbE-1xRX@122.88e6")],
    ]

    argvalues = util_test_length.select_test_cases_by_length(test_length, test_cases)
    metafunc.parametrize(ARGNAMES_DUAL_SFP, argvalues)

    fast_params = util_test_length.test_length_params(iterations=2, duration=10)
    stress_params = util_test_length.test_length_params(iterations=2, duration=600)
    parametrize_test_length(metafunc, test_length, fast_params, stress_params)

def generate_x410_100GbE_test_cases(metafunc, test_length, dut_fpga, sfp_int0, sfp_int1, dest_addr0, dest_addr1):
    if dut_fpga.upper() == 'CG_400':
        test_cases = [
            # Test Lengths                                         dual_sfp  rate      rx_rate   rx_channels dest_addr        dest_port adapter host_interface keep_hdr test case ID
            # --------------------------------------------------------------------------------------------------------------------------------------------------------
            [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,    491.52e6, 491.52e6, "0",        dest_addr0, 1234,     "sfp0", sfp_int0,      True,    id="SFP0_FULL_PACKET_1x100GbE-1xRX@491.52e6")],
            [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,    491.52e6, 491.52e6, "1",        dest_addr0, 1234,     "sfp0", sfp_int0,      False,   id="SFP0_RAW_PAYLOAD_1x100GbE-1xRX@491.52e6")],
            [{},                                      pytest.param(False,    491.52e6, 491.52e6, "2",        dest_addr0, 1234,     "sfp0", sfp_int0,      True,    id="SFP0_FULL_PACKET_1x100GbE-1xRX@491.52e6")],
            [{},                                      pytest.param(False,    491.52e6, 491.52e6, "3",        dest_addr0, 1234,     "sfp0", sfp_int0,      False,   id="SFP0_RAW_PAYLOAD_1x100GbE-1xRX@491.52e6")],
            [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,    491.52e6, 491.52e6, "0",        dest_addr1, 1234,     "sfp1", sfp_int1,      True,    id="SFP1_FULL_PACKET_1x100GbE-1xRX@491.52e6")],
            [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,    491.52e6, 491.52e6, "1",        dest_addr1, 1234,     "sfp1", sfp_int1,      False,   id="SFP1_RAW_PAYLOAD_1x100GbE-1xRX@491.52e6")],
            [{},                                      pytest.param(False,    491.52e6, 491.52e6, "2",        dest_addr1, 1234,     "sfp1", sfp_int1,      True,    id="SFP1_FULL_PACKET_1x100GbE-1xRX@491.52e6")],
            [{},                                      pytest.param(False,    491.52e6, 491.52e6, "3",        dest_addr1, 1234,     "sfp1", sfp_int1,      False,   id="SFP1_RAW_PAYLOAD_1x100GbE-1xRX@491.52e6")],
        ]

    if dut_fpga.upper() == 'UC_200':
        test_cases = [
            # Test Lengths                                         dual_sfp  rate   rx_rate  rx_channels dest_addr        dest_port  adapter  host_interface  keep_hdr test case ID
            # --------------------------------------------------------------------------------------------------------------------------------------------------------
            [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,    250e6, 250e6,   "0",        dest_addr1, 1234,      "sfp1",  sfp_int1,       True,    id="SFP1_FULL_PACKET_1x100GbE-1xRX@250e6")],
            [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,    250e6, 250e6,   "1",        dest_addr1, 1234,      "sfp1",  sfp_int1,       False,   id="SFP1_RAW_PAYLOAD_1x100GbE-1xRX@250e6")],
            [{},                                      pytest.param(False,    250e6, 250e6,   "2",        dest_addr1, 1234,      "sfp1",  sfp_int1,       True,    id="SFP1_FULL_PACKET_1x100GbE-1xRX@250e6")],
            [{},                                      pytest.param(False,    250e6, 250e6,   "3",        dest_addr1, 1234,      "sfp1",  sfp_int1,       False,   id="SFP1_RAW_PAYLOAD_1x100GbE-1xRX@250e6")],
        ]

    argvalues = util_test_length.select_test_cases_by_length(test_length, test_cases)
    metafunc.parametrize(ARGNAMES_DUAL_SFP, argvalues)

    fast_params = util_test_length.test_length_params(iterations=5, duration=2)
    stress_params = util_test_length.test_length_params(iterations=2, duration=600)
    parametrize_test_length(metafunc, test_length, fast_params, stress_params)

def generate_x440_100GbE_test_cases(metafunc, test_length, dut_fpga, sfp_int0, sfp_int1, dest_addr0, dest_addr1):
    if dut_fpga.upper() == 'CG_400':
        test_cases = [
            # Test Lengths                                         dual_sfp  rate    rx_rate  rx_channels dest_addr        dest_port adapter host_interface keep_hdr test case ID
            # --------------------------------------------------------------------------------------------------------------------------------------------------------
            [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,    500e6,  500e6,   "0",        dest_addr0, 1234,     "sfp0", sfp_int0,      True,    id="SFP0_FULL_PACKET_1x100GbE-1xRX@500e6")],
            [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,    500e6,  500e6,   "1",        dest_addr0, 1234,     "sfp0", sfp_int0,      False,   id="SFP0_RAW_PAYLOAD_1x100GbE-1xRX@500e6")],
            [{},                                      pytest.param(False,    500e6,  500e6,   "2",        dest_addr0, 1234,     "sfp0", sfp_int0,      True,    id="SFP0_FULL_PACKET_1x100GbE-1xRX@500e6")],
            [{},                                      pytest.param(False,    500e6,  500e6,   "3",        dest_addr0, 1234,     "sfp0", sfp_int0,      False,   id="SFP0_RAW_PAYLOAD_1x100GbE-1xRX@500e6")],
            [{},                                      pytest.param(False,    500e6,  500e6,   "4",        dest_addr0, 1234,     "sfp0", sfp_int0,      True,    id="SFP0_FULL_PACKET_1x100GbE-1xRX@500e6")],
            [{},                                      pytest.param(False,    500e6,  500e6,   "5",        dest_addr0, 1234,     "sfp0", sfp_int0,      False,   id="SFP0_RAW_PAYLOAD_1x100GbE-1xRX@500e6")],
            [{},                                      pytest.param(False,    500e6,  500e6,   "6",        dest_addr0, 1234,     "sfp0", sfp_int0,      True,    id="SFP0_FULL_PACKET_1x100GbE-1xRX@500e6")],
            [{},                                      pytest.param(False,    500e6,  500e6,   "7",        dest_addr0, 1234,     "sfp0", sfp_int0,      False,   id="SFP0_RAW_PAYLOAD_1x100GbE-1xRX@500e6")],
            [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,    500e6,  500e6,   "0",        dest_addr1, 1234,     "sfp1", sfp_int1,      True,    id="SFP1_FULL_PACKET_1x100GbE-1xRX@500e6")],
            [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,    500e6,  500e6,   "1",        dest_addr1, 1234,     "sfp1", sfp_int1,      False,   id="SFP1_RAW_PAYLOAD_1x100GbE-1xRX@500e6")],
            [{},                                      pytest.param(False,    500e6,  500e6,   "2",        dest_addr1, 1234,     "sfp1", sfp_int1,      True,    id="SFP1_FULL_PACKET_1x100GbE-1xRX@500e6")],
            [{},                                      pytest.param(False,    500e6,  500e6,   "3",        dest_addr1, 1234,     "sfp1", sfp_int1,      False,   id="SFP1_RAW_PAYLOAD_1x100GbE-1xRX@500e6")],
            [{},                                      pytest.param(False,    500e6,  500e6,   "4",        dest_addr1, 1234,     "sfp1", sfp_int1,      True,    id="SFP1_FULL_PACKET_1x100GbE-1xRX@500e6")],
            [{},                                      pytest.param(False,    500e6,  500e6,   "5",        dest_addr1, 1234,     "sfp1", sfp_int1,      False,   id="SFP1_RAW_PAYLOAD_1x100GbE-1xRX@500e6")],
            [{},                                      pytest.param(False,    500e6,  500e6,   "6",        dest_addr1, 1234,     "sfp1", sfp_int1,      True,    id="SFP1_FULL_PACKET_1x100GbE-1xRX@500e6")],
            [{},                                      pytest.param(False,    500e6,  500e6,   "7",        dest_addr1, 1234,     "sfp1", sfp_int1,      False,   id="SFP1_RAW_PAYLOAD_1x100GbE-1xRX@500e6")],
        ]

    if dut_fpga.upper() == 'CG_1600':
        test_cases = [
            # Test Lengths                                         dual_sfp  rate   rx_rate  rx_channels dest_addr        dest_port  adapter  host_interface  keep_hdr test case ID
            # --------------------------------------------------------------------------------------------------------------------------------------------------------
            [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,    1000e6, 1000e6,   "0",        dest_addr0, 1234,      "sfp0",  sfp_int0,       True,    id="SFP0_FULL_PACKET_1x100GbE-1xRX@1000e6")],
            [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,    1000e6, 1000e6,   "1",        dest_addr0, 1234,      "sfp0",  sfp_int0,       False,   id="SFP0_RAW_PAYLOAD_1x100GbE-1xRX@1000e6")],
            [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,    1000e6, 1000e6,   "0",        dest_addr1, 1234,      "sfp1",  sfp_int1,       True,    id="SFP1_FULL_PACKET_1x100GbE-1xRX@1000e6")],
            [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,    1000e6, 1000e6,   "1",        dest_addr1, 1234,      "sfp1",  sfp_int1,       False,   id="SFP1_RAW_PAYLOAD_1x100GbE-1xRX@1000e6")],
        ]

    argvalues = util_test_length.select_test_cases_by_length(test_length, test_cases)
    metafunc.parametrize(ARGNAMES_DUAL_SFP, argvalues)

    fast_params = util_test_length.test_length_params(iterations=5, duration=0.5)
    stress_params = util_test_length.test_length_params(iterations=2, duration=600)
    parametrize_test_length(metafunc, test_length, fast_params, stress_params)

def generate_X310_10GbE_test_cases(metafunc, test_length, sfp_int0, sfp_int1, dest_addr0, dest_addr1):
    test_cases = [
        # Test Lengths                                        dual_sfp  rate  rx_rate  rx_channels dest_addr        dest_port   adapter  host_interface   keep_hdr test case ID
        # --------------------------------------------------------------------------------------------------------------------------------------------------------
        [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,   200e6, 200e6, "0",      dest_addr0,     1234,      "sfp0",     sfp_int0,        True,    id="SFP0_FULL_PACKET_1x10GbE-1xRX@200e6")],
        [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,   200e6, 200e6, "1",      dest_addr0,     1234,      "sfp0",     sfp_int0,        False,   id="SFP0_RAW_PAYLOAD_1x10GbE-1xRX@200e6")],
        [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,   200e6, 200e6, "0",      dest_addr1,     1234,      "sfp1",     sfp_int1,        True,    id="SFP1_FULL_PACKET_1x10GbE-1xRX@200e6")],
        [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,   200e6, 200e6, "1",      dest_addr1,     1234,      "sfp1",     sfp_int1,        False,   id="SFP1_RAW_PAYLOAD_1x10GbE-1xRX@200e6")],
    ]

    argvalues = util_test_length.select_test_cases_by_length(test_length, test_cases)
    metafunc.parametrize(ARGNAMES_DUAL_SFP, argvalues)

    fast_params = util_test_length.test_length_params(iterations=5, duration=5)
    stress_params = util_test_length.test_length_params(iterations=2, duration=600)
    parametrize_test_length(metafunc, test_length, fast_params, stress_params)

def pytest_generate_tests(metafunc):
    dut_type = metafunc.config.getoption("dut_type")
    dut_fpga = metafunc.config.getoption("dut_fpga")
    test_length = metafunc.config.getoption("test_length")
    sfp_int0 = metafunc.config.getoption("sfp_int0")
    sfp_int1 = metafunc.config.getoption("sfp_int1")
    dest_addr0 = re.sub(r'\b(\d+\.\d+\.\d+)\.\d+\b', r'\1.1', metafunc.config.getoption("addr"))
    dest_addr1 = re.sub(r'\b(\d+\.\d+\.\d+)\.\d+\b', r'\1.1', metafunc.config.getoption("second_addr"))

    metafunc.parametrize("dut_type", [dut_type])

    if dut_type.lower() == 'x410' and dut_fpga.upper() == 'X4_200':
        generate_x410_10GbE_test_cases(metafunc, test_length, sfp_int0, sfp_int1, dest_addr0, dest_addr1)
    if dut_type.lower() == 'x410' and dut_fpga.upper() in {'CG_400', 'UC_200'}:
        generate_x410_100GbE_test_cases(metafunc, test_length, dut_fpga, sfp_int0, sfp_int1, dest_addr0, dest_addr1)
    if dut_type.lower() == 'x440' and dut_fpga.upper() in {'CG_400', 'CG_1600'}:
        generate_x440_100GbE_test_cases(metafunc, test_length, dut_fpga, sfp_int0, sfp_int1, dest_addr0, dest_addr1)
    if dut_type.lower() == 'x310' and dut_fpga.upper() == 'XG':
        generate_X310_10GbE_test_cases(metafunc, test_length, sfp_int0, sfp_int1, dest_addr0, dest_addr1)

def test_raw_udp_streaming(iterate_remote_rx, pytestconfig, dut_type, dual_sfp, rate, rx_rate, rx_channels,
                    dest_addr, dest_port, adapter, host_interface, keep_hdr, iterations, duration):

    remote_rx_path = Path(pytestconfig.getoption('uhd_build_dir')) / 'examples/python/rx_to_remote_udp.py'

    device_args = ""

    # construct device args string
    if dut_type.lower() in ['n310', 'n320', 'e320', 'x310', 'x410', 'x440']:
        device_args += f"master_clock_rate={rate},"

    # mpm reboot on x440 is for spurrious RF performance, these tests do not care about RF performance
    if dut_type.lower() == 'x440':
        device_args += f"skip_mpm_reboot=1,"

    for item in ["addr", "second_addr", "mgmt_addr"]:
        if pytestconfig.getoption(item):
            device_args += f"{item}={pytestconfig.getoption(item)},"

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
    stats = iterate_remote_rx(remote_rx_params, remote_rx_path, iterations, host_interface, True)
    print("Stats after run:")
    print("=" * 20)
    for stat in stats:
        print(stats)
        
    dut_fpga = pytestconfig.getoption('dut_fpga')
    chdr_hdr_size = 64 #100GBE default
    if dut_fpga:
        if dut_fpga.upper() == "CG_400":
            chdr_hdr_size = 64
        if dut_fpga.upper() == "UC_200":
            chdr_hdr_size = 32
        if dut_fpga.upper() in {"X4_200", "XG"}:
            chdr_hdr_size = 16
    analyze_stats(stats, remote_rx_params, chdr_hdr_size)

def analyze_stats(stats, remote_rx_params, chdr_hdr_size):
    ALLOWED_LOSS = 0.1 #percentage, arbitrary limit for now
    ALLOWED_CAPTURE_LOSS = 1 #percentage, arbitrary limit for now

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
        for [stats_before_run, stats_after_run, capture_file_size] in stats:
            expected_data = get_expected_data(remote_rx_params, stats_after_run.packets_recv - stats_before_run.packets_recv, chdr_hdr_size)
            received_data = stats_after_run.bytes_recv - stats_before_run.bytes_recv
            if (expected_data != 0):
                analyzed_stats.append([capture_file_size, expected_data, received_data, abs(100 * (received_data - expected_data) / expected_data), abs(100 * (capture_file_size - expected_data) / expected_data)])
            else:
                raise RuntimeError("Expected Data is of zero size! Modify rate or duration params!")
        return analyzed_stats

    def print_analyzed_stats(analyzed_stats):
        header = "| capture_file_size | expected_data | received_data | deviation(%) | capture_file_size_deviation(%) |"
        ruler  = "|-------------------|---------------|---------------|--------------|--------------------------------|"
        s = header + "\n" + ruler + "\n"
        for [capture_file_size, expected_data, received_data, deviation, capture_file_size_deviation] in analyzed_stats:
            row = (
                "| {:<17} ".format(capture_file_size) +
                "| {:<13} ".format(expected_data) +
                "| {:<13} ".format(received_data) +
                "| {:<12.3f} ".format(deviation) +
                "| {:<27.3f} ".format(capture_file_size_deviation)
            )
            s += row + "|" + "\n"

        print(s)

    # Basic check
    for [stats_before_run, stats_after_run, capture_file_size] in stats:
        assert (stats_after_run.bytes_recv > stats_before_run.bytes_recv)
        assert (capture_file_size > 0)

    # Actual check
    analyzed_stats = get_analyzed_stats(stats)
    print_analyzed_stats(analyzed_stats)
    for [_, _, _, deviation, capture_file_size_deviation] in analyzed_stats:
        assert(deviation < ALLOWED_LOSS)
        assert(capture_file_size_deviation < ALLOWED_CAPTURE_LOSS)

# fmt: on
