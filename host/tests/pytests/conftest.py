import time
import pytest
import util_test_length
import run_benchmark_rate
import parse_benchmark_rate

from collections import namedtuple


dut_type_list = [
   "N310",
   "N320",
   "B206",
   "B210",
   "E320",
   "X310",
   "X310_TwinRx",
   "X410",
   "X440"
]

test_length_list = [
    util_test_length.Test_Length_Smoke,
    util_test_length.Test_Length_Full,
    util_test_length.Test_Length_Stress
]

def pytest_sessionstart(session):
    print(f"[{time.strftime('%Y-%m-%d %H:%M:%S')}] Starting pytest session")

@pytest.fixture(scope='function')
def run_benchmark():
    # This fixture creates a function that runs the benchmark_rate tool
    # This function is being called when using the fixture
    def _run_benchmark(path, benchmark_rate_params, stop_on_error=True):
        proc = run_benchmark_rate.run(path, benchmark_rate_params)
        result = parse_benchmark_rate.parse(proc.stdout.decode('ASCII'))
        if result is not None:
            return result
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
        return None
    
    return _run_benchmark

    # RESULT_STR = """
    # [00:00:00.000376] Creating the usrp device with: addr=192.168.30.2, second_addr=192.168.40.2...
    # [00:00:05.63100253] Testing receive rate 200.000000 Msps on 2 channels
    # [00:00:05.73100253] Testing transmit rate 100.000000 Msps on 1 channels
    # [00:00:15.113339078] Benchmark complete.

    # Benchmark rate summary:
    # Num received samples:     10000
    # Num dropped samples:      200
    # Num overruns detected:    10
    # Num transmitted samples:  20000
    # Num sequence errors (Tx): 5
    # Num sequence errors (Rx): 6
    # Num underruns detected:   20
    # Num late commands:        2
    # Num timeouts (Tx):        0
    # Num timeouts (Rx):        100

    # Done!
    # """
    # return RESULT_STR

@pytest.fixture(scope='function')
def iterate_benchmark(run_benchmark, threshold, capsys):
    # This fixture creates a function that runs the benchmark_rate tool multiple times
    # This function is being called when using the fixture
    def _iterate_benchmark(path, iterations, extraruns, benchmark_rate_params):
        # extract the parameter "use_dpdk" from the benchmark_rate_params for later evaluation
        print(benchmark_rate_params)
        args_dict = dict(pair.split('=') for pair in benchmark_rate_params['args'].split(',') if '=' in pair)
        use_dpdk = args_dict.get('use_dpdk', 0) == 1
        for key, val in benchmark_rate_params.items():
            print("{:14} {}".format(key, val))
        results = []
        good_runs = 0
        for iteration in range(iterations + extraruns):
            result = run_benchmark(path, benchmark_rate_params, False)
            if result is None or result == "":
                print(f"[{time.strftime('%Y-%m-%d %H:%M:%S')}] Iteration {iteration + 1} of {iterations}: No result from benchmark rate test")
                continue
            single_pass = (
                result.dropped_samps <= threshold['single_run'].dropped_samps and
                result.overruns <= threshold['single_run'].overruns and
                (result.underruns <= threshold['single_run'].underruns or not use_dpdk) and
                result.late_commands <= threshold['single_run'].late_commands and
                result.rx_timeouts <= threshold['single_run'].rx_timeouts and
                result.rx_seq_errs <= threshold['single_run'].rx_seq_errs and
                result.tx_timeouts <= threshold['single_run'].tx_timeouts and
                result.tx_seq_errs <= threshold['single_run'].tx_seq_errs
                )
            result = result._replace(iteration=iteration + 1, single_pass=single_pass)
            results.append(result)
            if single_pass:
                good_runs += 1
            if good_runs >= iterations:
                break
        return results

    return _iterate_benchmark


@pytest.fixture(scope='session')
def threshold():
    # This fixture creates a function that returns the thresholds for the benchmark rate test
    Thresholds = namedtuple(
        'Thresholds',
        '''
        dropped_samps
        overruns
        underruns
        late_commands
        rx_timeouts
        rx_seq_errs
        tx_timeouts
        tx_seq_errs
        '''
    )
    thresholds = {}
    # These are the thresholds for a single run of the benchmark rate test
    thresholds['single_run'] = Thresholds(
        dropped_samps=100,
        overruns=100,
        underruns=100,
        late_commands=100,
        rx_timeouts=100,
        rx_seq_errs=100,
        tx_timeouts=100,
        tx_seq_errs=100,
    )
    # These are the thresholds for averaged numbers of the benchmark rate test
    thresholds['average'] = Thresholds(
        dropped_samps=50,
        overruns=50,
        underruns=50,
        late_commands=50,
        rx_timeouts=50,
        rx_seq_errs=50,
        tx_timeouts=50,
        tx_seq_errs=50,
    )
    return thresholds


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
        choices=dut_type_list + [x.lower() for x in dut_type_list],
        help="")
    parser.addoption(
        "--dut_fpga",
        type=str,
        required=False,
        help="")
    parser.addoption(
        "--test_length",
        type=str,
        default=util_test_length.Test_Length_Full,
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
    parser.addoption(
        "--sfp_int0",
        type=str,
        required=False,
        help="configures name of sfp0 interface")
    parser.addoption(
        "--sfp_int1",
        type=str,
        required=False,
        help="configures name of sfp1 interface")

def pytest_configure(config):
    # register additional markers
    config.addinivalue_line("markers", "dpdk: run with DPDK enable")
