#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
This module contains the streaming backend for the simulator. It
handles managing threads, as well as interfacing with sample sources
and sinks.
"""
import time
from threading import Thread
import queue
import socket
from uhd.chdr import PacketType, StrcOpCode, StrcPayload, StrsPayload, StrsStatus, ChdrHeader, ChdrPacket

class XferCount:
    """This class keeps track of flow control transfer status which are
    used to populate Strc and Strs packets
    """
    def __init__(self):
        self.num_bytes = 0
        self.num_packets = 0

    @classmethod
    def from_strc(cls, payload):
        """Construct an XferCount from a Strc Payload"""
        xfer = cls()
        xfer.num_packets = payload.num_pkts
        xfer.num_bytes = payload.num_bytes
        return xfer

    @classmethod
    def from_strs(cls, payload):
        xfer = cls()
        xfer.num_packets = payload.xfer_count_pkts
        xfer.num_bytes = payload.xfer_count_bytes
        return xfer

    def has_exceeded(self, limit):
        """returns true if this XferCount >= the limit
        in either packets or bytes
        """
        return self.num_packets >= limit.num_packets or self.num_bytes >= limit.num_bytes

    def count_packet(self, len):
        """Account for a len bytes sized packet transfer"""
        self.num_bytes += len
        self.num_packets += 1

    def clear(self):
        """Reset the counts to zero"""
        self.num_bytes = 0
        self.num_packets = 0

    def __str__(self):
        return "XferCount{{num_bytes:{}, num_packets:{}}}".format(self.num_bytes, self.num_packets)

class ChdrInputStream:
    """This class encapsulates an Rx Thread. This thread blocks on a
    queue which receives STRC and DATA ChdrPackets. It places the data
    packets into the sample_sink and responds to the STRC packets using
    the send_wrapper
    """
    CAPACITY_BYTES = int(5e3) # 5 KB
    QUEUE_CAP = 3
    def __init__(self, log, chdr_w, sample_sink, send_wrapper, our_epid):
        self.log = log
        self.chdr_w = chdr_w
        self.sample_sink = sample_sink
        self.send_wrapper = send_wrapper
        self.xfer = XferCount()
        self.accum = XferCount()
        self.fc_freq = None
        self.command_target = None
        self.command_addr = None
        self.command_epid = None
        self.our_epid = our_epid
        self.rx_queue = queue.Queue(ChdrInputStream.QUEUE_CAP)
        self.stop = False
        self.thread = Thread(target=self._rx_worker, daemon=True)
        self.thread.start()

    def _rx_worker(self):
        self.log.info("Stream RX Worker Starting")
        while True:
            packet, recv_len, addr = self.rx_queue.get()
            # This break is here because when ChdrInputStream.stop() is called,
            # a tuple of 3 None values is pushed into the queue to unblock the worker.
            if self.stop:
                break
            header = packet.get_header()
            self.xfer.count_packet(recv_len)
            self.accum.count_packet(recv_len)
            pkt_type = header.pkt_type
            if pkt_type in (PacketType.DATA_WITH_TS, PacketType.DATA_NO_TS):
                self.sample_sink.accept_packet(packet)
            elif pkt_type == PacketType.STRC:
                req_payload = packet.get_payload_strc()
                # Ping doesn't change anything, just requests a stream status packet
                if req_payload.op_code == StrcOpCode.INIT:
                    self.xfer.clear()
                    self.fc_freq = XferCount.from_strc(req_payload)
                    self.command_addr = addr
                    self.command_epid = req_payload.src_epid
                elif req_payload.op_code == StrcOpCode.RESYNC:
                    self.xfer = XferCount.from_strc(req_payload)
                resp_packet = self._generate_strs_packet(req_payload.src_epid, self.our_epid)
                self.send_wrapper.send_packet(resp_packet, addr)
            else:
                raise RuntimeError("RX Worker received unsupported packet: {}".format(pkt_type))

            # Check if a fc status packet is due
            if self.fc_freq is not None and self.accum.has_exceeded(self.fc_freq):
                self.accum.clear()
                self.log.trace("Flow Control Due, sending STRS")
                self.command_target = None
                resp_packet = self._generate_strs_packet(self.command_epid, self.our_epid)
                self.log.trace("Sending Flow Control: {}".format(resp_packet.to_string_with_payload()))
                self.send_wrapper.send_packet(resp_packet, self.command_addr)

        self.sample_sink.close()
        self.log.info("Stream RX Worker Done")

    def finish(self):
        """Unblocks the worker and stops the thread.
        The worker will close its sample_sink
        """
        self.stop = True
        self.rx_queue.put((None, None, None))

    def _generate_strs_packet(self, dst_epid, src_epid):
        """Create an strs packet from the information in self.xfer"""
        resp_header = ChdrHeader()
        resp_header.dst_epid = dst_epid
        resp_payload = StrsPayload()
        resp_payload.src_epid = src_epid
        resp_payload.status = StrsStatus.OKAY
        resp_payload.capacity_bytes = ChdrInputStream.CAPACITY_BYTES
        resp_payload.capacity_pkts = 0xFFFFFF
        resp_payload.xfer_count_bytes = self.xfer.num_bytes
        resp_payload.xfer_count_pkts = self.xfer.num_packets
        resp_packet = ChdrPacket(self.chdr_w, resp_header, resp_payload)
        return resp_packet

    def queue_packet(self, packet, recv_len, addr):
        """Queue a packet to be processed by the ChdrInputStream"""
        self.rx_queue.put((packet, recv_len, addr))

class ChdrOutputStream:
    """This class encapsulates a Tx Thread. It takes data from its
    sample_source and then sends it in a data packet using its
    send_wrapper.

    The tx stream is configured using the stream_spec object, which
    sets parameters such as sample rate and destination
    """
    def __init__(self, log, chdr_w, sample_source, stream_spec, send_wrapper):
        self.log = log
        self.chdr_w = chdr_w
        self.sample_source = sample_source
        self.stream_spec = stream_spec
        self.send_wrapper = send_wrapper
        self.xfer = XferCount()
        self.recv = XferCount()
        self.stop = False
        self.strs_queue = queue.Queue(100)
        self.strc_seq_num = 0
        self.data_seq_num = 0

        self.thread = Thread(target=self._tx_worker, daemon=True)
        self.thread.start()

    def _tx_worker(self):
        self.log.info("Stream TX Worker Starting with {} packets/sec"
                      .format(1/self.stream_spec.seconds_per_packet()))
        self.log.info("Downstream Buffer Capacity: {} packets or {} bytes"
                      .format(self.stream_spec.capacity_packets, self.stream_spec.capacity_bytes))
        header = ChdrHeader()
        start_time = time.time()
        next_send = start_time
        header.dst_epid = self.stream_spec.dst_epid
        header.pkt_type = PacketType.DATA_NO_TS

        is_continuous = self.stream_spec.is_continuous
        num_samps_left = None
        if not is_continuous:
            # TODO: Put sample format/width in the stream spec
            num_samps_left = self.stream_spec.total_samples * 4 # SC16 is 4 bytes per sample

        timestamp = self.stream_spec.init_timestamp  \
            if self.stream_spec.is_timed else None

        while is_continuous or num_samps_left > 0:
            if self.stop:
                self.log.info("Stream Worker Stopped")
                break
            header.seq_num = self.data_seq_num
            # When seq_num gets to 65535 (Max Unsigned 16 bit integer)
            # It wraps back around to 0
            self.data_seq_num = int(self.data_seq_num + 1) & 0xFFFF
            packet = ChdrPacket(self.chdr_w, header, bytes(0), timestamp)
            packet_samples = self.stream_spec.packet_samples
            if num_samps_left is not None:
                packet_samples = min(packet_samples, num_samps_left)
                num_samps_left -= packet_samples
            packet = self.sample_source.fill_packet(packet, packet_samples)
            if packet is None:
                break
            send_data = bytes(packet.serialize()) # Serialize before waiting

            delay = next_send - time.time()
            if delay > 0:
                time.sleep(delay)
            next_send = next_send + self.stream_spec.seconds_per_packet()

            timestamp = None

            # Check Flow Control to assert there is space downstream
            while not self._can_fit_packet(len(send_data)):
                strs_update = self.strs_queue.get()
                strs_payload = strs_update.get_payload_strs()
                self._update_recv(strs_payload)

            self.send_wrapper.send_data(send_data, self.stream_spec.addr)
            self.xfer.count_packet(len(send_data))

        self.log.info("Stream Worker Done")
        finish_time = time.time()
        self.log.info("Actual Packet Rate was {} packets/sec"
                      .format(self.xfer.num_packets/(finish_time - start_time)))
        self.sample_source.close()

    def finish(self):
        """Stops the ChdrOutputStream"""
        self.stop = True

    def queue_packet(self, packet):
        """ Place an incoming STRS packet in the Queue """
        self.strs_queue.put_nowait(packet)

    def _can_fit_packet(self, length):
        """ Can the downstream buffer fit a packet of length right now """
        packets_in_transit = self.xfer.num_packets - self.recv.num_packets
        space_packets = self.stream_spec.capacity_packets - packets_in_transit
        bytes_in_transit = self.xfer.num_bytes - self.recv.num_bytes
        space_bytes = self.stream_spec.capacity_bytes - bytes_in_transit
        return space_packets > 0 and space_bytes >= length

    def _update_recv(self, strs_payload):
        """ Update the Xfer counts for downstream receive using
        an incoming strs payload
        """
        assert strs_payload.status == StrsStatus.OKAY, \
            "Flow Control Error: STRS Status is {}".format(strs_payload.status)
        self.recv.num_packets = strs_payload.xfer_count_pkts
        self.recv.num_bytes = strs_payload.xfer_count_bytes
