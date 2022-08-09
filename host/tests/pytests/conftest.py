import test_length_utils

dut_type_list = [
   "N310",
   "N320",
   "B210",
   "E320",
   "X310",
   "X310_TwinRx",
   "x4xx"
]


test_length_list = [
    test_length_utils.Test_Length_Smoke,
    test_length_utils.Test_Length_Full,
    test_length_utils.Test_Length_Stress
]


def pytest_addoption(parser):
    parser.addoption(
        "--addr",
        type=str,
        nargs='?',
        help="address of first 10 GbE interface",)
    parser.addoption(
        "--second_addr",
        type=str,
        nargs='?',
        help="address of second 10 GbE interface")
    parser.addoption(
        "--name",
        type=str,
        nargs='?',
        help="name of B2xx device")
    parser.addoption(
        "--mgmt_addr",
        type=str,
        nargs='?',
        help="address of management interface. only needed for DPDK test cases")
    parser.addoption(
        "--dut_type",
        type=str,
        required=True,
        choices=dut_type_list,
        help="")
    parser.addoption(
        "--test_length",
        type=str,
        default=test_length_utils.Test_Length_Full,
        choices=test_length_list,
        help="")
    parser.addoption(
        "--uhd_build_dir",
        required=True,
        type=str,
        help="")
    parser.addoption(
        "--num_recv_frames",
        type=str,
        nargs='?',
        help="configures num_recv_frames parameter")
    parser.addoption(
        "--num_send_frames",
        type=str,
        nargs='?',
        help="configures num_send_frames parameter")

def pytest_configure(config):
    # register additional markers
    config.addinivalue_line("markers", "dpdk: run with DPDK enable")
