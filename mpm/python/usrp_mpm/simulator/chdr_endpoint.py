#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""This module houses the ChdrEndpoint class, which handles networking,
packet dispatch, and acts as an interface between these and the RFNoC
Graph.
"""

from threading import Thread
import socket
from uhd.chdr import ChdrPacket, ChdrWidth, PacketType
from .rfnoc_graph import XbarNode, XportNode, StreamEndpointNode, RFNoCGraph, NodeType

CHDR_W = ChdrWidth.W64

class ChdrEndpoint:
    """This class is created by the sim periph_manager
    It is responsible for opening sockets, dispatching all chdr packet
    traffic to the appropriate destination, and responding to said
    traffic.

    The extra_args parameter is passed in from the periph_manager, and
    coresponds to the --default_args flag of usrp_hwd.py on the
    command line
    """
    def __init__(self, log, extra_args):
        self.log = log.getChild("ChdrEndpoint")
        self.thread = Thread(target=self.socket_worker, daemon=True)
        self.thread.start()
        self.graph = RFNoCGraph(self.get_default_nodes(), self.log, 1, self.begin_tx,
                                self.end_tx, self.send_strc, self.begin_rx)
        self.xport_map = {}

    def set_sample_rate(self, rate):
        """Set the sample_rate of the next tx_stream.

        This method is called by the daughterboard. It coresponds to
        sim_dboard.py:sim_db#set_catalina_clock_rate()
        """
        self.graph.get_stream_spec().sample_rate = rate

    def get_default_nodes(self):
        """Get a sensible NoC Core setup. This is the simplest
        functional layout. It has one of each required component.
        """
        nodes = [
            XportNode(0),
            XbarNode(0, [2], [0]),
            StreamEndpointNode(0)
        ]
        return nodes

    def send_strc(self, stream_ep, addr):
        pass # TODO: currently not implemented

    def begin_tx(self, src_epid, stream_spec):
        pass # TODO: currently not implemented

    def end_tx(self, src_epid):
        pass # TODO: currently not implemented

    def begin_rx(self, dst_epid):
        pass # TODO: currently not implemented

    def socket_worker(self):
        """This is the method that runs in a background thread. It
        blocks on the CHDR socket and processes packets as they come
        in.
        """
        self.log.info("Starting ChdrEndpoint Thread")
        main_sock = socket.socket(socket.AF_INET,
                                  socket.SOCK_DGRAM)
        main_sock.bind(("0.0.0.0", 49153))

        while True:
            # This allows us to block on multiple sockets at the same time
            buffer = bytearray(8000) # Max MTU
            # received Data over socket
            n_bytes, sender = main_sock.recvfrom_into(buffer)
            self.log.trace("received {} bytes of data from {}"
                           .format(n_bytes, sender))
            try:
                packet = ChdrPacket.deserialize(CHDR_W, buffer[:n_bytes])
                self.log.trace("Decoded Packet: {}".format(packet.to_string_with_payload()))
                entry_xport = (1, NodeType.XPORT, 0)
                pkt_type = packet.get_header().pkt_type
                response = self.graph.handle_packet(packet, entry_xport, sender)

                if response is not None:
                    data = response.serialize()
                    self.log.trace("Returning Packet: {}"
                                   .format(packet.to_string_with_payload()))
                    main_sock.sendto(bytes(data), sender)
            except BaseException as ex:
                self.log.warning("Unable to decode packet: {}"
                                 .format(ex))
                raise ex
