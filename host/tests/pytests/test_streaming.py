import pytest
from pathlib import Path
import batch_run_benchmark_rate
import test_length_utils
from test_length_utils import Test_Length_Smoke, Test_Length_Full, Test_Length_Stress

ARGNAMES_DUAL_SFP = ["dual_SFP", "rate", "rx_rate", "rx_channels", "tx_rate", "tx_channels"]
ARGNAMES =                      ["rate", "rx_rate", "rx_channels", "tx_rate", "tx_channels"]

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


def generate_N310_test_cases(metafunc, test_length):
    test_cases = [
        # Test Lengths                                         dual_SFP  rate     rx_rate  rx_channels tx_rate  tx_channels  test case ID
        # ----------------------------------------------------------------------------------------------------------------------------------------------
        [{},                                      pytest.param(False,    153.6e6, 153.6e6, "0",        0,       "",          id="1x10GbE-1xRX@153.6e6")],
        [{},                                      pytest.param(False,    153.6e6, 153.6e6, "0,1",      0,       "",          id="1x10GbE-2xRX@153.6e6")],
        [{},                                      pytest.param(False,    153.6e6, 0,       "",         153.6e6, "0",         id="1x10GbE-1xTX@153.6e6")],
        [{},                                      pytest.param(False,    153.6e6, 0,       "",         153.6e6, "0,1",       id="1x10GbE-2xTX@153.6e6")],
        [{},                                      pytest.param(False,    153.6e6, 153.6e6, "0",        153.6e6, "0",         id="1x10GbE-1xTRX@153.6e6")],
        [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,    153.6e6, 153.6e6, "0,1",      153.6e6, "0,1",       id="1x10GbE-2xTRX@153.6e6")],
        [{},                                      pytest.param(False,    125e6,   125e6,   "0,1",      125e6,   "0,1",       id="1x10GbE-2xTRX@125e6")],
        [{},                                      pytest.param(False,    62.5e6,  62.5e6,  "0,1,2,3",  0,       "",          id="1x10GbE-4xRX@62.5e6")],
        [{},                                      pytest.param(False,    62.5e6,  0,       "",         62.5e6,  "0,1,2,3",   id="1x10GbE-4xTX@62.5e6")],
        [{Test_Length_Smoke, Test_Length_Stress}, pytest.param(False,    62.5e6,  62.5e6,  "0,1,2,3",  62.5e6,  "0,1,2,3",   id="1x10GbE-4xTRX@62.5e6")],
        [{},                                      pytest.param(True,     153.6e6, 153.6e6, "0,1",      0,       "",          id="2x10GbE-2xRX@153.6e6")],
        [{},                                      pytest.param(True,     153.6e6, 0,       "",         153.6e6, "0,1",       id="2x10GbE-2xTX@153.6e6")],
        [{},                                      pytest.param(True,     153.6e6, 153.6e6, "0,1",      153.6e6, "0,1",       id="2x10GbE-2xTRX@153.6e6")],
        [{},                                      pytest.param(True,     153.6e6, 153.6e6, "0,1,2,3",  0,       "",          id="2x10GbE-4xRX@153.6e6")],
        [{},                                      pytest.param(True,     153.6e6, 0,       "",         153.6e6, "0,1,2,3",   id="2x10GbE-4xTX@153.6e6")],
        [{},                                      pytest.param(True,     153.6e6, 153.6e6, "0,1,2,3",  153.6e6, "0,1,2,3",   id="2x10GbE-4xTRX@153.6e6")],
        [{},                                      pytest.param(True,     125e6,   125e6,   "0,1,2,3",  0,       "",          id="2x10GbE-4xRX@125e6")],
        [{},                                      pytest.param(True,     125e6,   0,       "",         125e6,   "0,1,2,3",   id="2x10GbE-4xTX@62.5e6")],
        [{Test_Length_Smoke, Test_Length_Stress}, pytest.param(True,     125e6,   125e6,   "0,1,2,3",  125e6,   "0,1,2,3",   id="2x10GbE-4xTRX@62.5e6")],
    ]

    argvalues = test_length_utils.select_test_cases_by_length(test_length, test_cases)
    metafunc.parametrize(ARGNAMES_DUAL_SFP, argvalues)

    fast_params = test_length_utils.test_length_params(iterations=10, duration=30)
    stress_params = test_length_utils.test_length_params(iterations=2, duration=600)
    parametrize_test_length(metafunc, test_length, fast_params, stress_params)


def generate_N320_test_cases(metafunc, test_length):
    test_cases = [
        # Test Lengths                                         dual_SFP  rate     rx_rate  rx_channels tx_rate  tx_channels  test case ID
        # ---------------------------------------------------------------------------------------------------------------------------------------------
        [{},                                      pytest.param(False,    250e6,   250e6,   "0",        0,       "",          id="1x10GbE-1xRX@250e6")],
        [{},                                      pytest.param(False,    250e6,   0,       "",         250e6,   "0",         id="1x10GbE-1xTX@250e6")],
        [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,    250e6,   250e6,   "0",        250e6,   "0",         id="1x10GbE-1xTRX@250e6")],
        [{},                                      pytest.param(True,     250e6,   250e6,   "0,1",      0,       "",          id="2x10GbE-2xRX@250e6")],
        [{},                                      pytest.param(True,     250e6,   0,       "",         250e6,   "0,1",       id="2x10GbE-2xTX@250e6")],
        [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(True,     250e6,   250e6,   "0,1",      250e6,   "0,1",       id="2x10GbE-2xTRX@250e6")],
    ]

    argvalues = test_length_utils.select_test_cases_by_length(test_length, test_cases)
    metafunc.parametrize(ARGNAMES_DUAL_SFP, argvalues)

    fast_params = test_length_utils.test_length_params(iterations=10, duration=30)
    stress_params = test_length_utils.test_length_params(iterations=2, duration=600)
    parametrize_test_length(metafunc, test_length, fast_params, stress_params)


def generate_B210_test_cases(metafunc, test_length):
    test_cases = [
        # Test Lengths                                         rate     rx_rate  rx_channels tx_rate  tx_channels  test case ID
        # ------------------------------------------------------------------------------------------------------------------------------
        [{},                                      pytest.param(61.44e6, 61.44e6, "0",        0,       "",          id="1xRX@61.44e6")],
        [{},                                      pytest.param(30.72e6, 30.72e6, "0,1",      0,       "",          id="2xRX@30.72e6")],
        [{},                                      pytest.param(61.44e6, 0,       "",         61.44e6, "0",         id="1xTX@61.44e6")],
        [{},                                      pytest.param(30.72e6, 0,       "",         30.72e6, "0,1",       id="2xTX@30.72e6")],
        [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(30.72e6, 30.72e6, "0",        30.72e6, "0",         id="1xTRX@30.72e6")],
        [{},                                      pytest.param(15.36e6, 15.36e6, "0,1",      15.36e6, "0,1",       id="2xTRX@15.36e6")],
    ]

    argvalues = test_length_utils.select_test_cases_by_length(test_length, test_cases)
    metafunc.parametrize(ARGNAMES, argvalues)

    fast_params = test_length_utils.test_length_params(iterations=10, duration=30)
    stress_params = test_length_utils.test_length_params(iterations=2, duration=600)
    parametrize_test_length(metafunc, test_length, fast_params, stress_params)


def generate_E320_test_cases(metafunc, test_length):
    test_cases = [
        # Test Lengths                                         rate     rx_rate  rx_channels tx_rate  tx_channels  test case ID
        # ------------------------------------------------------------------------------------------------------------------------------
        [{},                                      pytest.param(61.44e6, 61.44e6, "0",        0,       "",          id="1xRX@61.44e6")],
        [{},                                      pytest.param(61.44e6, 61.44e6, "0,1",      0,       "",          id="2xRX@61.44e6")],
        [{},                                      pytest.param(61.44e6, 0,       "",         61.44e6, "0",         id="1xTX@61.44e6")],
        [{},                                      pytest.param(61.44e6, 0,       "",         61.44e6, "0,1",       id="2xTX@61.44e6")],
        [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(61.44e6, 61.44e6, "0",        61.44e6, "0",         id="1xTRX@61.44e6")],
        [{},                                      pytest.param(61.44e6, 61.44e6, "0,1",      61.44e6, "0,1",       id="2xTRX@61.44e6")],

    ]

    argvalues = test_length_utils.select_test_cases_by_length(test_length, test_cases)
    metafunc.parametrize(ARGNAMES, argvalues)

    fast_params = test_length_utils.test_length_params(iterations=10, duration=30)
    stress_params = test_length_utils.test_length_params(iterations=2, duration=600)
    parametrize_test_length(metafunc, test_length, fast_params, stress_params)

def generate_X310_test_cases(metafunc, test_length):
    test_cases = [
        # Test Lengths                                         dual_SFP  rate     rx_rate  rx_channels tx_rate  tx_channels  test case ID
        # ---------------------------------------------------------------------------------------------------------------------------------------------
        [{},                                      pytest.param(False,    200e6,   200e6,   "0",        0,       "",          id="1x10GbE-1xRX@200e6")],
        [{},                                      pytest.param(False,    100e6,   100e6,   "0,1",      0,       "",          id="1x10GbE-2xRX@100e6")],
        [{},                                      pytest.param(False,    200e6,   0,       "",         200e6,   "0",         id="1x10GbE-1xTX@200e6")],
        [{},                                      pytest.param(False,    100e6,   0,       "",         100e6,   "0,1",       id="1x10GbE-2xTX@100e6")],
        [{},                                      pytest.param(False,    200e6,   200e6,   "0",        200e6,   "0",         id="1x10GbE-1xTRX@200e6")],
        [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,    100e6,   100e6,   "0,1",      100e6,   "0",         id="1x10GbE-2xTRX@100e6")],
        [{},                                      pytest.param(True,     200e6,   200e6,   "0,1",      0,       "",          id="2x10GbE-2xRX@200e6")],
        [{},                                      pytest.param(True,     200e6,   0,       "",         200e6,   "0,1",       id="2x10GbE-2xTX@200e6")],
        [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(True,     200e6,   200e6,   "0,1",      200e6,   "0,1",       id="2x10GbE-2xTRX@200e6")],
    ]

    argvalues = test_length_utils.select_test_cases_by_length(test_length, test_cases)
    metafunc.parametrize(ARGNAMES_DUAL_SFP, argvalues)

    fast_params = test_length_utils.test_length_params(iterations=10, duration=60)
    stress_params = test_length_utils.test_length_params(iterations=2, duration=600)
    parametrize_test_length(metafunc, test_length, fast_params, stress_params)

def generate_X310_TwinRx_test_cases(metafunc, test_length):
    test_cases = [
        # Test Lengths                                         dual_SFP  rate     rx_rate  rx_channels tx_rate  tx_channels  test case ID
        # --------------------------------------------------------------------------------------------------------------------------------------------
        [{},                                      pytest.param(False,    100e6,   100e6,   "0,1,2",    0,       "",          id="1x10GbE-3xRX@100e6")],
        [{},                                      pytest.param(False,    50e6,    50e6,    "0,1,2,4",  0,       "",          id="1x10GbE-4xRX@50e6")],
        [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(True,     100e6,   100e6,   "0,1,2,4",  0,       "",          id="2x10GbE-4xRX@100e6")],
    ]

    argvalues = test_length_utils.select_test_cases_by_length(test_length, test_cases)
    metafunc.parametrize(ARGNAMES_DUAL_SFP, argvalues)

    fast_params = test_length_utils.test_length_params(iterations=10, duration=30)
    stress_params = test_length_utils.test_length_params(iterations=2, duration=600)
    parametrize_test_length(metafunc, test_length, fast_params, stress_params)

def generate_x4xx_test_cases(metafunc, test_length):
    test_cases = [
        # Test Lengths                                         dual_SFP  rate     rx_rate  rx_channels tx_rate  tx_channels  test case ID
        # ------------------------------------------------------------------------------------------------------------------------------
        #[{},                                      pytest.param(False,    200e6,   200e6,   "0",        0,       "",          id="1x10GbE-1xRX@200e6")],
        #[{},                                      pytest.param(False,    200e6,   100e6,   "0,1",      0,       "",          id="1x10GbE-2xRX@100e6")],
        #[{},                                      pytest.param(False,    200e6,   0,       "",         200e6,   "0",         id="1x10GbE-1xTX@200e6")],
        #[{},                                      pytest.param(False,    200e6,   0,       "",         100e6,   "0,1",       id="1x10GbE-2xTX@100e6")],
        #[{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,    200e6,   200e6,   "0",        200e6,   "0",         id="1x10GbE-1xTRX@200e6")],
        #[{},                                      pytest.param(False,    200e6,   100e6,   "0,1",      100e6,   "0,1",       id="1x10GbE-2xTRX@100e6")],
        #[{},                                      pytest.param(True,     200e6,   200e6,   "0,1",      0,       "",          id="2x10GbE-2xRX@200e6")],
        #[{},                                      pytest.param(True,     200e6,   0,       "",         200e6,   "0,1",       id="2x10GbE-2xTX@200e6")],
        #[{Test_Length_Stress, Test_Length_Smoke}, pytest.param(True,     200e6,   100e6,   "0,1",      100e6,   "0,1",       id="2x10GbE-2xTRX@100e6")],
        [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(False,     491.52e6, 491.52e6, "0,1",    491.52e6, "0,1",       id="1x100GbE-2xTRX@491.52e6")],
        [{},                                      pytest.param(False,     491.52e6, 0,      "",         491.52e6, "0,1",       id="1x100GbE-2xTX@491.52e6")],
        [{},                                      pytest.param(False,     491.52e6, 491.52e6, "0,1",     0,        "",         id="1x100GbE-2xRX@491.52e6")],
        [{Test_Length_Stress, Test_Length_Smoke}, pytest.param(True,      491.52e6, 491.52e6, "0,1,2,3", 491.52e6, "0,1,2,3",  id="2x100GbE-4xTRX@491.52e6", marks=pytest.mark.xfail)],
        [{},                                      pytest.param(True,      491.52e6, 0,       "",         491.52e6, "0,1,2,3",  id="2x100GbE-4xTX@491.52e6")],
        [{},                                      pytest.param(True,      491.52e6, 491.52e6, "0,1,2,3", 0,        "",         id="2x100GbE-4xRX@491.52e6")],
    ]

    argvalues = test_length_utils.select_test_cases_by_length(test_length, test_cases)
    metafunc.parametrize(ARGNAMES_DUAL_SFP, argvalues)

    fast_params = test_length_utils.test_length_params(iterations=10, duration=30)
    stress_params = test_length_utils.test_length_params(iterations=2, duration=600)
    parametrize_test_length(metafunc, test_length, fast_params, stress_params)


def pytest_generate_tests(metafunc):
    dut_type = metafunc.config.getoption("dut_type")
    test_length = metafunc.config.getoption("test_length")

    metafunc.parametrize("dut_type", [dut_type])

    if dut_type.lower() != "b210":
        argvalues_DPDK = [
            #            use_dpdk  test case ID  marks
            pytest.param(True,     id="DPDK",    marks=pytest.mark.dpdk),
            pytest.param(False,    id="NO DPDK",)
        ]
        metafunc.parametrize("use_dpdk", argvalues_DPDK)

    if dut_type.lower() == 'n310':
        generate_N310_test_cases(metafunc, test_length)
    elif dut_type.lower() == 'n320':
        generate_N320_test_cases(metafunc, test_length)
    elif dut_type.lower() == 'b210':
        generate_B210_test_cases(metafunc, test_length)
    elif dut_type.lower() == 'e320':
        generate_E320_test_cases(metafunc, test_length)
    elif dut_type.lower() == 'x310':
        generate_X310_test_cases(metafunc, test_length)
    elif dut_type.lower() == 'x310_twinrx':
        generate_X310_TwinRx_test_cases(metafunc, test_length)
    elif dut_type.lower() == 'x4xx':
        generate_x4xx_test_cases(metafunc, test_length)


def test_streaming(pytestconfig, dut_type, use_dpdk, dual_SFP, rate, rx_rate, rx_channels,
                   tx_rate, tx_channels, iterations, duration):

    benchmark_rate_path = Path(pytestconfig.getoption('uhd_build_dir')) / 'examples/benchmark_rate'

    # construct device args string
    device_args = f"master_clock_rate={rate},"

    if dut_type == "B210":
        device_args += f"name={pytestconfig.getoption('name')},"
    else:
        device_args += f"addr={pytestconfig.getoption('addr')},"

    if dual_SFP:
        device_args += f"second_addr={pytestconfig.getoption('second_addr')},"

    if use_dpdk:
        device_args += f"use_dpdk=1,mgmt_addr={pytestconfig.getoption('mgmt_addr')}"

    # construct benchmark_rate params dictionary
    benchmark_rate_params = {
        "args": device_args,
        "duration": duration,
    }

    if rx_channels:
        benchmark_rate_params["rx_rate"] = rx_rate
        benchmark_rate_params["rx_channels"] = rx_channels

    if tx_channels:
        benchmark_rate_params["tx_rate"] = tx_rate
        benchmark_rate_params["tx_channels"] = tx_channels

    # Run X410 streaming tests in multi_streamer mode and high thread priority
    # since those settings allow for best performance.
    if dut_type.lower() == "x4xx":
        benchmark_rate_params["multi_streamer"] = 1
        benchmark_rate_params["priority"] = "high"

    # run benchmark rate
    print()
    results = batch_run_benchmark_rate.run(benchmark_rate_path, iterations, benchmark_rate_params)
    stats = batch_run_benchmark_rate.calculate_stats(results)
    print(batch_run_benchmark_rate.get_summary_string(stats, iterations, benchmark_rate_params))

    # compare results against thresholds
    # TODO: Have non adhoc better thresholds.
    dropped_samps_threshold = 50
    overruns_threshold = 50
    rx_timeouts_threshold = 50
    rx_seq_err_threshold = 50

    underruns_threshold = 50
    tx_timeouts_threshold = 50
    tx_seq_err_threshold = 50

    late_cmds_threshold = 50

    # TODO: define custom failed assertion explanations to avoid extra output
    # https://docs.pytest.org/en/6.2.x/assert.html#defining-your-own-explanation-for-failed-assertions

    if rx_channels:
        assert stats.avg_vals.dropped_samps <= dropped_samps_threshold, \
            f"""Number of dropped samples exceeded threshold.
                Expected dropped samples: <= {dropped_samps_threshold}
                Actual dropped samples:      {stats.avg_vals.dropped_samps}"""
        assert stats.avg_vals.overruns <= overruns_threshold, \
            f"""Number of overruns exceeded threshold.
                Expected overruns: <= {overruns_threshold}
                Actual overruns:      {stats.avg_vals.overruns}"""
        assert stats.avg_vals.rx_timeouts <= rx_timeouts_threshold, \
            f"""Number of rx timeouts exceeded threshold.
                Expected rx timeouts: <= {rx_timeouts_threshold}
                Actual rx timeouts:      {stats.avg_vals.rx_timeouts}"""
        assert stats.avg_vals.rx_seq_errs <= rx_seq_err_threshold, \
            f"""Number of rx sequence errors exceeded threshold.
                Expected rx sequence errors: <= {rx_seq_err_threshold}
                Actual rx sequence errors:      {stats.avg_vals.rx_seq_errs}"""

    if tx_channels:
        assert stats.avg_vals.underruns <= underruns_threshold, \
            f"""Number of underruns exceeded threshold.
                Expected underruns: <= {underruns_threshold}
                Actual underruns:      {stats.avg_vals.underruns}"""
        assert stats.avg_vals.tx_timeouts <= tx_timeouts_threshold, \
            f"""Number of tx timeouts exceeded threshold.
                Expected tx timeouts: <= {tx_timeouts_threshold}
                Actual tx timeouts:      {stats.avg_vals.tx_timeouts}"""
        assert stats.avg_vals.tx_seq_errs <= tx_seq_err_threshold, \
            f"""Number of tx sequence errors exceeded threshold.
                Expected tx sequence errors: <= {tx_seq_err_threshold}
                Actual tx sequence errors:      {stats.avg_vals.tx_seq_errs}"""

    assert stats.avg_vals.late_cmds <= late_cmds_threshold, \
        f"""Number of late commands exceeded threshold.
            Expected late commands: <= {late_cmds_threshold}
            Actual late commands:      {stats.avg_vals.late_cmds}"""
