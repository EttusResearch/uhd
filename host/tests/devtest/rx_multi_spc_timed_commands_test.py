#
# Copyright 2022 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Tests RX multi-spc timed commands by generating samples from USRP with a delay that is not a multiple
of the spc and makes sure the tick count does not get rounded down to the nearest multiple of spc.
"""
import logging
import numpy as np
import os
import sys
import uhd
from uhd_test_base import shell_application
from uhd_test_base import UHDPythonTestCase

class rx_multi_spc_timed_commands_test(UHDPythonTestCase):
    """ Run rx_multi_spc_timed_commands_test """
    test_name = 'rx_multi_spc_timed_commands_test'

    def run_test(self, test_name, test_args):
        """
        Run test and report results.
        """
        rate = test_args.get('rate', 1e6)
        channel = test_args.get('channel', 0)
        delay = test_args.get('delay', 0.5)
        spp = test_args.get('spp', 1024)
        # test_spc is the number of samples per cycle to test. We want
        # to use the maximum supported spc on any device, so that we
        # check that no errors slipped in for any timestamp modulo spc.
        test_spc = test_args.get('test_spc', 4)

        usrp = uhd.usrp.MultiUSRP(self.args_str)
        usrp.set_rx_rate(rate)
        st_args = uhd.usrp.StreamArgs('fc32', 'sc16')
        st_args.channels = [channel]
        rx_streamer = usrp.get_rx_stream(st_args)

        run_results = {
            'passed': True
        }

        for timekeeper_offset in range(test_spc):
            usrp.set_time_now(uhd.types.TimeSpec(timekeeper_offset/rate))
            for sample_offset in range(test_spc):
                if not run_results['passed']:
                    continue

                # Calculate a future time in ticks that has the needed alignment
                ticks = usrp.get_time_now().to_ticks(rate)
                ticks = ticks + uhd.types.TimeSpec(delay).to_ticks(rate)
                ticks = (ticks // test_spc) * test_spc;
                ticks = ticks + sample_offset

                # Set up stream command for the first packet
                stream_cmd = uhd.types.StreamCMD(uhd.types.StreamMode.num_done)
                stream_cmd.num_samps = spp
                stream_cmd.stream_now = False
                stream_cmd.time_spec = uhd.types.TimeSpec.from_ticks(ticks, rate)

                rx_streamer.issue_stream_cmd(stream_cmd)

                iq = np.empty(spp, dtype=np.complex64)
                rx_md = uhd.types.RXMetadata()
                # timeout (delay before recv + recv time + padding)
                timeout = delay + spp/rate + 0.5
                rx_streamer.recv(iq, rx_md, timeout=timeout)

                # If multi-spc is not working, the tick count of the sample returned will be rounded down
                # to the nearest multiple of spc
                if rx_md.time_spec.get_tick_count(rate) != stream_cmd.time_spec.get_tick_count(rate):
                    self.log.error("Actual packet received time does not match requested time.")
                    run_results['passed'] = False

                if not run_results['passed']:
                    self.log.error(
                        f"Test Failed.\n"
                        f"Timekeeper offset: {timekeeper_offset}, "
                        f"Sample offset: {sample_offset}, "
                        f"Sample rate: {rate}"
                    )

        for key in sorted(run_results):
            self.log.info('%s = %s', str(key), str(run_results[key]))
            self.report_result(
                'python_spc_rx_tester',
                key, run_results[key]
            )
        if 'passed' in run_results:
            self.report_result(
                'python_spc_rx_tester',
                'status',
                'Passed' if run_results['passed'] else 'Failed',
            )
        return run_results
