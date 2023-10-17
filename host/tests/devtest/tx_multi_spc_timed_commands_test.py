#
# Copyright 2022 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Tests TX multi-spc timed commands by transmitting samples from USRP with a delay that is not a multiple
of the spc and makes sure all samples were successfully transmitted without late packets.
"""
import logging
import numpy as np
import os
import sys
import uhd
from uhd_test_base import shell_application
from uhd_test_base import UHDPythonTestCase

class tx_multi_spc_timed_commands_test(UHDPythonTestCase):
    """ Run tx_multi_spc_timed_commands_test """
    test_name = 'tx_multi_spc_timed_commands_test'

    def run_test(self, test_name, test_args):
        """
        Run test and report results.
        """
        rate = test_args.get('rate', 1e6)
        channel = test_args.get('channel', 0)
        delay = test_args.get('delay', 0.1)
        spp = test_args.get('spp', 1024)
        # test_spc is the number of samples per cycle to test. We want
        # to use the maximum supported spc on any device, so that we
        # check that no errors slipped in for any timestamp modulo spc.
        test_spc = test_args.get('test_spc', 4)

        usrp = uhd.usrp.MultiUSRP(self.args_str)
        usrp.set_tx_rate(rate)
        st_args = uhd.usrp.StreamArgs('fc32', 'sc16')
        st_args.channels = [channel]
        tx_streamer = usrp.get_tx_stream(st_args)
        samples = np.zeros(spp, dtype=np.complex64)

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

                tx_md = uhd.types.TXMetadata()
                tx_md.start_of_burst = True
                tx_md.end_of_burst = True
                tx_md.has_time_spec = True
                tx_md.time_spec = uhd.types.TimeSpec.from_ticks(ticks, rate)

                # The first call to send() will block while waiting for the delay
                # timeout (delay before transmit + transmit time + padding)
                timeout = delay + spp/rate + 0.1

                sent = tx_streamer.send(samples, tx_md)

                # Wait long enough for all the sample to go out and check for ACK packet
                # Underflow messages in the queue are allowed, but no lates
                async_md = uhd.types.TXAsyncMetadata()
                got_ack = False
                while not got_ack and tx_streamer.recv_async_msg(async_md, timeout):
                    if async_md.event_code == uhd.types.TXMetadataEventCode.burst_ack:
                        got_ack = True
                    if async_md.event_code == uhd.types.TXMetadataEventCode.time_error:
                        self.log.error("Packet was late")
                        run_results['passed'] = False

                if not got_ack:
                    self.log.error("Packet not acknowledged")
                    run_results['passed'] = False

                if sent != spp:
                    self.log.error(f"Expected {spp} samples transmitted, but only got {sent}.")
                    run_results['passed'] = False

                if not run_results['passed']:
                    self.log.error(
                        f"Test failed.\n"
                        f"Timekeeper offset: {timekeeper_offset}, "
                        f"Sample offset: {sample_offset}, "
                        f"Sample rate: {rate}"
                    )

        for key in sorted(run_results):
            self.log.info('%s = %s', str(key), str(run_results[key]))
            self.report_result(
                'python_spc_tx_tester',
                key, run_results[key]
            )
        if 'passed' in run_results:
            self.report_result(
                'python_spc_tx_tester',
                'status',
                'Passed' if run_results['passed'] else 'Failed',
            )
        return run_results
