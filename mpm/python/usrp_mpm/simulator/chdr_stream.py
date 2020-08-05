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
from uhd.chdr import PacketType, StrcOpCode, StrsPayload, StrsStatus, ChdrHeader, ChdrPacket

class XferCount:
    """This class keeps track of flow control transfer status which are
    used to populate Strc and Strs packets
    """
    def __init__(self):
        self.num_bytes = 0
        self.num_packets = 0

    def count_packet(self, length):
        """Accounts for a packet of len bytes in the xfer count"""
        self.num_bytes += length
        self.num_packets += 1

    def clear(self):
        """Reset the xfer counts to 0"""
        self.num_bytes = 0
        self.num_packets = 0

    def __str__(self):
        return "XferCount{{num_bytes:{}, num_packets:{}}}".format(self.num_bytes, self.num_packets)

class SelectableQueue:
    """ A simple python Queue implementation which can be selected.
    This allows waiting on a queue and a socket simultaneously.
    """
    def __init__(self, max_size=0):
        self._queue = queue.Queue(max_size)
        self._send_signal_rx, self._send_signal_tx = socket.socketpair()

    def put(self, item, block=True, timeout=None):
        """ Put an element into the queue, optionally blocking """
        self._queue.put(item, block, timeout)
        self._send_signal_tx.send(b"\x00")

    def fileno(self):
        """ A fileno compatible with select.select """
        return self._send_signal_rx.fileno()

    def get(self):
        """ Return the first element in the queue, blocking if none
        are available.
        """
        self._send_signal_rx.recv(1)
        return self._queue.get_nowait()

class SendWrapper:
    """This class is used as an abstraction over queueing packets to be
    sent by the socket thread.
    """
    def __init__(self, queue):
        self.queue = queue

    def send_packet(self, packet, addr):
        """Serialize packet and then queue the data to be sent to addr
        returns the length of the serialized packet
        """
        data = packet.serialize()
        self.send_data(bytes(data), addr)
        return len(data)

    def send_data(self, data, addr):
        """Queue data to be sent to addr"""
        self.queue.put((data, addr))


class ChdrInputStream:
    """This class encapsulates an Rx Thread. This thread blocks on a
    queue which receives STRC and DATA ChdrPackets. It places the data
    packets into the sample_sink and responds to the STRC packets using
    the send_wrapper
    """
    CAPACITY_BYTES = int(5e6) # 5 MB
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
            pkt_type = header.pkt_type
            if pkt_type in (PacketType.DATA_WITH_TS, PacketType.DATA_NO_TS):
                self.xfer.count_packet(recv_len)
                self.sample_sink.accept_packet(packet)
            elif pkt_type == PacketType.STRC:
                req_payload = packet.get_payload_strc()
                resp_header = ChdrHeader()
                resp_header.dst_epid = req_payload.src_epid
                if req_payload.op_code == StrcOpCode.INIT:
                    self.xfer.clear()
                elif req_payload.op_code == StrcOpCode.RESYNC:
                    self.xfer.num_bytes = req_payload.xfer_count_bytes
                    self.xfer.num_packets = req_payload.xfer_count_pkts
                resp_payload = self._generate_strs_payload(header.dst_epid)
                resp_packet = ChdrPacket(self.chdr_w, resp_header, resp_payload)
                self.send_wrapper.send_packet(resp_packet, addr)
            else:
                raise RuntimeError("RX Worker received unsupported packet: {}".format(pkt_type))
        self.sample_sink.close()
        self.log.info("Stream RX Worker Done")

    def finish(self):
        """Unblocks the worker and stops the thread.
        The worker will close its sample_sink
        """
        self.stop = True
        self.rx_queue.put((None, None, None))

    def _generate_strs_payload(self, src_epid):
        """Create an strs payload from the information in self.xfer"""
        resp_payload = StrsPayload()
        resp_payload.src_epid = src_epid
        resp_payload.status = StrsStatus.OKAY
        resp_payload.capacity_bytes = ChdrInputStream.CAPACITY_BYTES
        resp_payload.capacity_pkts = 0xFFFFFF
        resp_payload.xfer_count_bytes = self.xfer.num_bytes
        resp_payload.xfer_count_pkts = self.xfer.num_packets
        return resp_payload

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
        self.stop = False
        self.thread = Thread(target=self._tx_worker, daemon=True)
        self.thread.start()

    def _tx_worker(self):
        self.log.info("Stream TX Worker Starting with {} packets/sec"
                      .format(1/self.stream_spec.seconds_per_packet()))
        header = ChdrHeader()
        start_time = time.time()
        next_send = start_time
        header.dst_epid = self.stream_spec.dst_epid
        header.pkt_type = PacketType.DATA_NO_TS

        num_samps_left = None
        if not self.stream_spec.is_continuous:
            num_samps_left = self.stream_spec.total_samples * 4 # SC16 is 4 bytes per sample

        for seq_num in self.stream_spec.seq_num_iter():
            if self.stop:
                self.log.info("Stream Worker Stopped")
                break
            if num_samps_left == 0:
                break
            header.seq_num = seq_num
            packet = ChdrPacket(self.chdr_w, header, bytes(0))
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
