#
# Copyright 2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
""" Test a series of radio RX timed commands."""


import uhd
from uhd_test_base import UHDPythonTestCase


class RXStreamCommandsTest(UHDPythonTestCase):
    """Test series of radio RX timed commands.

    The test needs an RFNoC enabled device with radio and replay block.
    These two blocks are connected and the radio gets a series of
    stream commands (test is parameterized by the number of commands).
    Each command should stream a small fixed number of sample to the
    replay block. The test checks whether the replay block received all
    samples, hence all stream commands were processed.
    """

    test_params = {
        "1 stream command": {"commands": 1},  # simplest case, a single command
        "10 stream commands": {"commands": 30},  # few more command
        "32 stream commands": {"commands": 32},  # first border (size of the RX command FIFO)
        "33 stream commands": {"commands": 33},  # one more command
        "100 stream commands": {"commands": 100},  # lot more commands
        "512 stream commands": {"commands": 512},  # next border (size of CrossBar FIFO)
        "513 stream commands": {"commands": 513},  # one more command
        "1000 stream commands": {"commands": 1000},  # lot more commands
    }

    def tear_down(self):
        """Free objects that hold RFNoC components."""
        self.graph = None
        self.replay = None
        self.radio = None

    def set_up(self):
        """Setting up the test.

        Aquire the device by creating the graph with the arguments passed by the
        test setup. Connect the radio and replay.
        """
        self.graph = uhd.rfnoc.RfnocGraph(self.args_str)

        self.replay = uhd.rfnoc.ReplayBlockControl(self.graph.get_block("0/Replay#0"))
        self.radio = uhd.rfnoc.RadioControl(self.graph.get_block("0/Radio#0"))

        uhd.rfnoc.connect_through_blocks(
            self.graph, self.radio.get_unique_id(), 0, self.replay.get_unique_id(), 0, True
        )
        self.graph.commit()

    def run_test(self, test_name, test_args):
        """Run a single test iteration.

        A test is queue up a number of stream commands. The test succeeds
        iff there are no errors are reported (e.g. late commands) and the
        number of samples received by the replay block equals the number
        of requested samples summed over all stream commands.
        """
        self.log.info(f"run {type(self).__name__} with {test_name}")

        num_samples = 1024  # number of samples to send per stream cmd
        delay = 0.5  # delay (s) between current FPGA time and test start
        cmd_interval = 0.001  # spacing (s) between two stream commands
        num_commands = test_args["commands"]  # number of stream commands to be send
        result = {}

        mem_size = self.replay.get_mem_size()
        self.replay.record(mem_size // 2, num_commands * 4 * num_samples, 0)

        usrp_time = self.graph.get_mb_controller().get_timekeeper(0).get_time_now()

        stream_cmd = uhd.types.StreamCMD(uhd.types.StreamMode.num_done)
        stream_cmd.num_samps = num_samples
        stream_cmd.stream_now = False

        for i in range(num_commands):
            stream_cmd.time_spec = uhd.types.TimeSpec(
                usrp_time.get_real_secs() + delay + i * cmd_interval
            )
            self.radio.issue_stream_cmd(stream_cmd, 0)

        timeout = 1
        while True:
            rx_md = self.replay.get_record_async_metadata(timeout)
            if not rx_md:
                break
            if rx_md.error_code != uhd.types.RXMetadataErrorCode.none:
                error_str = f"Recording error: {rx_md.strerror()}"
                self.log.error(error_str)
                result["errors"] = [*result.get("errors", []), error_str]
            timeout = 0.01

        result["recorded_bytes"] = self.replay.get_record_fullness(0)
        result["expected_bytes"] = num_commands * 4 * num_samples
        result["passed"] = "errors" not in result and (
            result["recorded_bytes"] == result["expected_bytes"]
        )
        return result
