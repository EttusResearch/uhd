#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Unit test for libpyuhd.chdr (CHDR Parsing API)
"""

import unittest
from uhd import chdr
from chdr_resource import hardcoded_packets
from chdr_resource import rfnoc_packets_data
from chdr_resource import rfnoc_packets_ctrl_mgmt

# unittest doesn't support parameterized tests natively,
# rather than add another dependency just for testing, we use this
class parameterize:
    """Decorate a class with this. It deletes the method named by
    func_name and adds methods for every test case, appending _{name}
    to the name of the function
    """
    def __init__(self, func_name, names, cases):
        self.func_name = func_name
        self.cases = cases
        self.names = names

    def __call__(self, cls):
        func = getattr(cls, self.func_name)
        # First remove the function
        delattr(cls, self.func_name)
        for case, name in zip(self.cases, self.names):
            # Add a new test function for every case
            def new_func(self, this_case=case):
                return func(self, *this_case)
            setattr(cls, self.func_name + "_" + name, new_func)
        return cls


@parameterize("test_serialize_deserialize_eq", hardcoded_packets.names, hardcoded_packets.packets)
class CHDRParseTest(unittest.TestCase):
    """ Test Python-wrapped CHDR Parser classes """

    def test_parse_no_errors(self):
        """Parse every packet in the trace we have.
        This test is just looking for errors
        """
        packets = [packet_data for peer in [
            rfnoc_packets_ctrl_mgmt.peer0,
            rfnoc_packets_ctrl_mgmt.peer1,
            rfnoc_packets_data.peer0,
            rfnoc_packets_data.peer1
            ] for packet_data in peer]
        for packet_data in packets:
            _packet = chdr.ChdrPacket.deserialize(
                chdr.ChdrWidth.W64, packet_data)

    def test_serialize_deserialize_eq(self, packet, data):
        """This test serializes and then deserializes a few packets to
        make sure that they survive a round trip without changing
        """
        generated_data = bytes(packet.serialize())
        self.assertEqual(generated_data, data)

        generated_packet = chdr.ChdrPacket.deserialize(
            chdr.ChdrWidth.W64, data)
        generated_data = bytes(generated_packet.serialize())
        self.assertEqual(generated_data, data)
