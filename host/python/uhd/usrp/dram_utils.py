#
# Copyright 2023 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
UHD Extension, helpers, and utilities for doing more with the available PL DRAM
"""

import time
from uhd import rfnoc
from uhd.usrp import StreamArgs
from uhd.types import TXMetadata, RXMetadata, RXMetadataErrorCode, StreamMode, StreamCMD, TimeSpec

def enumerate_radios(graph, radio_chans):
    """
    Return a list of radio block controllers/chan pairs to use for this test.
    """
    def unpack_rcp_spec(rcps):
        """
        Convert a radio/channel pair specification into a tuple of form
        (radio_block_id, radio_chan).

        Valid inputs (and their corresponding outputs are:
        - "0/Radio#0:0" -> ("0/Radio#0", 0)
        - "0/Radio#0" -> ("0/Radio#0", 0)      [Channel 0 is chosen as default)
        - ("0/Radio#0", 0) -> ("0/Radio#0", 0)
        """
        if isinstance(rcps, str):
            return (rcps.split(':', 2)[0], int(rcps.split(':', 2)[1])) \
                   if ':' in rcps else (rcps, 0)
        if isinstance(rcps, tuple) and len(rcps) == 2:
            return rcps
        raise RuntimeError(f"Unknown radio channel pair specification: {rcps}")
    radio_id_chan_pairs = [unpack_rcp_spec(r) for r in radio_chans]
    # Sanity checks
    available_radios = graph.find_blocks("Radio")
    radio_chan_pairs = []
    for rcp in radio_id_chan_pairs:
        if rcp[0] not in available_radios:
            raise RuntimeError(f"'{rcp[0]}' is not a valid radio block ID!")
        radio_chan_pairs.append((rfnoc.RadioControl(graph.get_block(rcp[0])), rcp[1]))
    return radio_chan_pairs

def find_replay_block(graph, replay_blockid):
    """
    Find any or a specific replay block
    """
    if replay_blockid is None:
        blocklist = graph.find_blocks("Replay")
        if not blocklist:
            raise RuntimeError("No Replay block found on RFNoC graph!")
        replay_blockid = blocklist[0]
    return rfnoc.ReplayBlockControl(graph.get_block(replay_blockid))

class DramTransmitter:
    """
    Helper class to stream data from DRAM to one or more radios.

    This can be useful when setting up a USRP as a transmitter, with a waveform
    preloaded into memory.

    NOTE: This assumes we are using a single memory bank, until UHD is upgraded
    to better handle multiple memory banks.

    Arguments:
    rfnoc_graph -- The graph object
    radio_chan -- A list of radio channels to connect to the DRAM, in one of
                  these formats:
                  - "0/Radio:0#0" (single string)
                  - "0/Radio:0" (single string, channel 0 by default)
                  - ("0/Radio#0", 0) (tuple of block id and channel number)
    replay_blockid -- If specified, use this replay block
    replay_ports -- List of replay ports to use. Must be at least as long as the 
                    list of radio channels. If not specified, use all ports.
                    Therefore this should be used if TX and RX are going to be used.
    cpu_format -- For the upload process, the data format to be used
    mem_regions -- A list of (memory start, memory size) tuples, one per replay block port.
                   If left out, the memory is split up evenly among available replay block ports.
    """
    def __init__(self,
                 rfnoc_graph,
                 radio_chans,
                 replay_blockid=None,
                 replay_ports=None,
                 cpu_format='fc32',
                 mem_regions=None,
                 ):
        # We make replay_blocks a list so we can support multiple replay blocks
        # (not only on multiple motherboards) in the future without changing APIs
        self.replay_blocks = [find_replay_block(rfnoc_graph, replay_blockid)]
        self.word_size = max(x.get_word_size() for x in self.replay_blocks)
        # In the radio, we always use sc16 regardless of cpu_format
        self.bytes_per_sample = 4
        if replay_ports is None:
            replay_ports = list(range(len(radio_chans)))
        self.replay_ports = replay_ports

        self.reconnect(rfnoc_graph, radio_chans, mem_regions)
        # Since for multi-channel we nevertheless only use one input port to upload the 
        # data we always take the first of the given replay ports
        self.replay_in_port = self.replay_ports[0]

        stream_args = StreamArgs(cpu_format, "sc16")
        self.tx_streamer = rfnoc_graph.create_tx_streamer(1, stream_args)

        rfnoc_graph.connect(
            self.tx_streamer, 0,
            self.replay_blocks[0].get_unique_id(), self.replay_in_port)
        rfnoc_graph.commit()

    def reconnect(self, rfnoc_graph, radio_chans, mem_regions=None):
        """
        Reconnect the replay block to a new set of radio channels.
        """
        self.replay_ports = self._sanitize_replay_ports(self.replay_ports)
        # Map requested radio_chans to the outputs of this replay block
        self.radio_chan_pairs = enumerate_radios(rfnoc_graph, radio_chans)
        if len(self.replay_ports) < len(self.radio_chan_pairs):
            raise RuntimeError(
                f"{len(self.radio_chan_pairs)} radio channels were requested to be "
                f"used with {len(self.replay_ports)} replay ports on "
                f"Replay block {self.replay_blocks[0].get_unique_id()}! "
                "There must be at least as many replay block ports as radio channels.")

        # Disconnect the ports of the replay block that we want to use later on
        active_conns = rfnoc_graph.enumerate_active_connections()
        for conn in active_conns:
            # If any of the connections that we require are taken already we will
            # disconnect them here. That is especially important if both the transmitter
            # and the receiver should be used.
            if (conn.src_blockid == self.replay_blocks[0].get_unique_id() and \
                conn.src_port in self.replay_ports) or \
               (conn.dst_blockid == self.replay_blocks[0].get_unique_id() and \
                conn.dst_port in self.replay_ports):
                if "Streamer" in conn.dst_blockid:
                    # Need to use streamer disconnect method for streamers
                    rfnoc_graph.disconnect(
                        conn.dst_blockid, conn.dst_port
                    )
                elif "Streamer" in conn.src_blockid:
                    pass
                else:
                    # Disconnect for all other kinds of blocks
                    rfnoc_graph.disconnect(
                        conn.src_blockid,
                        conn.src_port,
                        conn.dst_blockid,
                        conn.dst_port,
                    )

        self.duc_chan_pairs = []
        for replay_port_idx, rcp in enumerate(self.radio_chan_pairs):
            edge = rfnoc.connect_through_blocks(
                rfnoc_graph,
                self.replay_blocks[0].get_unique_id(), self.replay_ports[replay_port_idx],
                rcp[0].get_unique_id(), rcp[1], True)
            duc_edge = \
                next(filter(lambda edge: rfnoc.BlockID(edge.dst_blockid).match("DUC"), edge), None)
            if duc_edge:
                self.duc_chan_pairs.append((
                    rfnoc.DucBlockControl(rfnoc_graph.get_block(duc_edge.dst_blockid)),
                    duc_edge.dst_port
                ))
            else:
                self.duc_chan_pairs.append(None)
        # Assign memory regions to each output port (and leave space for potential input ports)
        if mem_regions is None:
            mem_stride = self.replay_blocks[0].get_mem_size() \
                // self.replay_blocks[0].get_num_output_ports()
            mem_stride -= mem_stride % self.word_size
            self.mem_regions = [
                (idx * mem_stride, mem_stride)
                for idx in range(self.replay_blocks[0].get_num_output_ports())
            ]
        else:
            # Create mem_regions list prepopulated with 0 and then fill it with the
            # given mem_regions. This way we can also handle the case where only
            # a subset of the available replay ports is used.
            self.mem_regions = self._sanitize_mem_regions(mem_regions)
        # This stores how much data we have currently uploaded to memory. We
        # initialize this with the full memory, not zero.
        self.upload_size = [x[1] for x in self.mem_regions]


    def _sanitize_replay_ports(self, ports):
        """
        Helper function to turn ports into a valid list of ports on the replay
        block.
        """
        if ports is None:
            ports = self.replay_ports
        if isinstance(ports, int):
            ports = [ports]
        if any((port >= self.replay_blocks[0].get_num_output_ports() for port in ports)):
            raise RuntimeError(
                    f"Invalid output port on replay block! Available ports: "
                    f"{self.replay_blocks[0].get_num_output_ports()} Requested: "
                    f"{ports}")
        return ports

    def _sanitize_mem_regions(self, custom_mem_regions=None):
        """
        Helper function to make a list of memory regions valid for this replay
        block and with given custom memory regions.
        """
        # If called without custom regions just return what we already have
        if custom_mem_regions is None:
            return self.mem_regions
        mem_regions = [(0,0)] * self.replay_blocks[0].get_num_output_ports()
        # If custom region is big enough for all possible ports, use it
        if len(mem_regions) == len(custom_mem_regions):
            return custom_mem_regions
        # Otherwise we have to distribute the given regions among the available ports
        for idx, mem_region in enumerate(custom_mem_regions):
            if idx < len(self.replay_ports):
                mem_regions[self.replay_ports[idx]] = mem_region
        return mem_regions

    def _upload(self, waveform, mem_start, mem_size):
        """
        Upload helper function

        Arguments:
        waveform: 1-dimensional numpy array with waveform data. Must be of the
                  same data type as specified during the constructor.
        mem_start: Memory address where the data should be uploaded to
        mem_size: Amount of available memory. If the data in waveforms exceeds
                  this value, then waveform will be only partially uploaded.
        """
        # Sanitize parameters
        assert mem_start < self.replay_blocks[0].get_mem_size(), \
            f"Invalid memory start location: {mem_start}"
        assert mem_start + mem_size <= self.replay_blocks[0].get_mem_size(), \
            f"Invalid memory range: {mem_start} - {mem_start + mem_size}"
        assert mem_start % self.word_size == 0, \
            f"Memory region start (0x{mem_start:X}) is not aligned with " \
            f"word size ({self.word_size})!"
        assert mem_size % self.word_size == 0, \
            f"Memory region size (0x{mem_size:X}) is not aligned with " \
            f"word size ({self.word_size})!"
        num_items = min(len(waveform), int(mem_size) // self.bytes_per_sample)
        waveform = waveform[:num_items]

        num_bytes = num_items * self.bytes_per_sample
        in_port = self.replay_in_port
        # Configure DRAM block for recording
        self.replay_blocks[0].record(mem_start, num_bytes, in_port)
        # Flush data on input buffer
        flush_timeout = time.monotonic() + .25
        while time.monotonic() < flush_timeout:
            if self.replay_blocks[0].get_record_fullness(in_port) == 0:
                break
            self.replay_blocks[0].record_restart(in_port)
        # Upload data
        tx_md = TXMetadata()
        tx_md.start_of_burst = True
        tx_md.end_of_burst = True
        if self.tx_streamer.send(waveform, tx_md, 10.0) != num_items:
            raise RuntimeError("Unable to upload all data without errors!")
        # Make sure DRAM is fully populated
        upload_timeout = time.monotonic() + 20.0
        while time.monotonic() < upload_timeout and \
                self.replay_blocks[0].get_record_fullness(in_port) < num_bytes:
            time.sleep(.05)
        fullness = self.replay_blocks[0].get_record_fullness(in_port)
        if fullness != num_bytes:
            raise RuntimeError(
                f"DRAM fullness did not reach expected levels! "
                f"{fullness}/{num_bytes} bytes.")
        return num_bytes

    def upload(self, waveform, ports=None, mem_regions=None):
        """
        Store a waveform to memory

        Arguments:
        ports: If this argument is given, then we use the mem_regions attribute
               of this class to identify where to store the waveform. If ports
               is a list, then the waveform will be uploaded once per port to
               the corresponding memory region.
        mem_regions: If this argument is given, ports is ignored. This will
                     directly specify the memory regions stored in this object.
                     NOTE: This class attempts to keep track of how many samples
                     have been uploaded into memory. If mem_regions is provided,
                     then the mapping between ports and memory regions is lost,
                     and this class cannot know how many samples are available
                     for a given port. Therefore, use this argument to
                     deliberately override where sample data should be stored,
                     e.g., to override sections of previously uploaded sample
                     data.

        If port is not specified, upload to all ports.
        """
        if mem_regions:
            ports = None
        else:
            ports = self._sanitize_replay_ports(ports)
            # Sanitize again in case self.mem_regions was changed from the outside
            mem_regions = self._sanitize_mem_regions(self.mem_regions)
            mem_regions = [ mem_regions[port] for port in ports]
        if len(mem_regions) == 2 and \
                isinstance(mem_regions, (tuple, list)) and \
                isinstance(mem_regions[0], int):
            mem_regions = [mem_regions]

        # If this method is called from send() then we will work on a per-channel
        # basis. Otherwise we will walk through the mem_regions that we have put
        # together above.
        if len(waveform.shape) == 1:
            waveform = [waveform] * len(mem_regions)
        if len(waveform) < len(mem_regions):
            raise RuntimeError("Number of waveforms in waveform array does not match "
                               "the number of memory regions!")
        for region_idx, mem_region in enumerate(mem_regions):
            # Since by default we slice the dram per input/output port, we only
            # want to upload to the memory regions that we are actually using.
            if ports is None or region_idx < len(ports):
                bytes_uploaded = self._upload(waveform[region_idx], *mem_region)
                if ports:
                    self.upload_size[ports[region_idx]] = bytes_uploaded

    def issue_stream_cmd(self, stream_cmd, ports=None):
        """
        Issue a command to start or stop the streaming from DRAM.

        If ports is not specified, issue the stream command on all ports.

        Under the hood, this will call replay_block_control::config_play() (if
        the stream command is a start-command) and replay_block_control::issue_stream_cmd().

        Note that this will configure the replay block for playback every time
        it is called. If upload() was called previously, then this class knows
        how many samples are available for a given port, and it will only use
        the occupied memory space. This means that if a subset of the available
        memory region for a port is occupied with valid sample data, and the
        number of samples requested in this stream command is either unlimited
        or larger than the available sample data, the replay block will correctly
        loop around the available sample data.
        """
        ports = self._sanitize_replay_ports(ports)
        mem_regions = self._sanitize_mem_regions(self.mem_regions)
        for port in ports:
            if stream_cmd.stream_mode in (
                    StreamMode.start_cont,
                    StreamMode.num_done,
                    StreamMode.num_more):
                mem_region = mem_regions[port]
                mem_size = min(self.upload_size[port], mem_region[1])
                self.replay_blocks[0].config_play(mem_region[0], mem_size, port)
            self.replay_blocks[0].issue_stream_cmd(stream_cmd, port)

    def send(self, data, metadata, timeout=0.1):
        """
        This is a wrapper around upload() and issue_stream_cmd() that can be
        used to use this class like you would use a TxStreamer object.

        Compared to calling those two functions directly, this is less flexible.
        It requires 'data' to be of the right shape (meaning that it needs as
        many sub-arrays as there are channels, just like when calling send() on
        a TxStreamer object), and it will only stream the data once (again, as
        if with a TX streamer object).

        Unlike TxStreamer.send(), you cannot call this repeatedly with small
        packets. Every call to this method will overwrite the previous call's
        data.

        The timeout parameter is unused, it is only there to retain the call
        signature compatibility. Time specs are pulled from the TX metadata object.
        """
        num_chans = len(self.radio_chan_pairs)
        num_samps = len(data) if len(data.shape) == 1 else data.shape[1]
        if num_samps == 0 or any((x == 0 for x in data.shape)):
            # When streaming to radio, we sometimes send an empty burst with an
            # EOB to receive the ACK. With the DRAM replay, this doesn't work so
            # well, and we always send an EOB from here anyway, so we just
            # pretend like we did this.
            return 0
        if (num_chans > 1 and len(data.shape) != 2) or (data.shape[0] < num_chans):
            raise RuntimeError(
                f"Number of TX channels {num_chans} does not match the dimensions "
                f"of the data array ({data.shape[0] if len(data.shape) == 2 else 1})")
        # First upload the data
        if num_chans == 1:
            if len(data.shape) == 2:
                data = data[0]
            self.upload(data, self.replay_ports[0])
        else:
            for chan, _ in enumerate(self.radio_chan_pairs):
                self.upload(data[chan], self.replay_ports[chan])
        # Then trigger stream command
        stream_cmd = StreamCMD(StreamMode.num_done)
        stream_cmd.stream_now = not metadata.has_time_spec
        stream_cmd.time_spec = metadata.time_spec
        stream_cmd.num_samps = num_samps
        # This defaults to "all ports"
        self.issue_stream_cmd(stream_cmd)
        return num_samps

    def recv_async_msg(self, timeout=0.1):
        """
        This emulates TxStreamer.recv_async_msg().
        """
        return self.replay_blocks[0].get_play_async_metadata(timeout)

class DramReceiver:
    """
    Helper class to stream data from one or more radios to DRAM.

    This can be useful when setting up a USRP as a receiver.

    NOTE: This assumes we are using a single memory bank, until UHD is upgraded
    to better handle multiple memory banks.

    Arguments:
    rfnoc_graph -- The graph object
    radio_chans -- A list of radio channels to connect to the DRAM, in one of
                  these formats:
                  - "0/Radio:0#0" (single string)
                  - "0/Radio:0" (single string, channel 0 by default)
                  - ("0/Radio#0", 0) (tuple of block id and channel number)
    replay_blockid -- If specified, use this replay block
    replay_ports -- List of replay ports to use. Must be at least as long as the 
                    list of radio channels. If not specified, use all ports.
                    Therefore this should be used if TX and RX are going to be used.
    cpu_format -- Desired data format of the downloaded, received data on the host.
    mem_regions -- A list of (memory start, memory size) tuples, one per replay block port.
                   If left out, the memory is split up evenly among available replay block ports.
    throttle -- Throttle factor for the streamer. This is a value between 0 and
                1 or a percentage in the range (0%, 100%] that is passed as string.
    """
    def __init__(self,
                 rfnoc_graph,
                 radio_chans,
                 replay_blockid=None,
                 replay_ports=None,
                 cpu_format='fc32',
                 mem_regions=None,
                 throttle="0.1"
                 ):
        # We make replay_blocks a list so we can support multiple replay blocks
        # (not only on multiple motherboards) in the future without changing APIs
        self.replay_blocks = [find_replay_block(rfnoc_graph, replay_blockid)]
        self.word_size = max(x.get_word_size() for x in self.replay_blocks)
        # In the radio, we always use sc16 regardless of cpu_format
        self.bytes_per_sample = 4
        stream_args = StreamArgs(cpu_format, "sc16")
        stream_args.args['throttle'] = throttle
        if replay_ports is None:
            replay_ports = list(range(len(radio_chans)))
        self.replay_ports = replay_ports
        self.receive_metadata = None

        self.reconnect(rfnoc_graph, radio_chans, mem_regions)
        # We only use the first of the given replay ports to download the data sequentially.
        self.replay_out_port = self.replay_ports[0]

        self.rx_streamer = rfnoc_graph.create_rx_streamer(1, stream_args)

        rfnoc_graph.connect(
            self.replay_blocks[0].get_unique_id(), self.replay_out_port,
            self.rx_streamer, 0)
        rfnoc_graph.commit()

    def reconnect(self, rfnoc_graph, radio_chans, mem_regions=None):
        """
        Reconnect the replay block to a new set of radio channels.
        """
        self.replay_ports = self._sanitize_replay_ports(self.replay_ports)
        # Map requested radio_chans to the inputs of this replay block
        self.radio_chan_pairs = enumerate_radios(rfnoc_graph, radio_chans)
        if len(self.replay_ports) < len(self.radio_chan_pairs):
            raise RuntimeError(
                f"{len(self.radio_chan_pairs)} radio channels were requested to be "
                f"used with {len(self.replay_ports)} replay ports on "
                f"Replay block {self.replay_blocks[0].get_unique_id()}!"
                "There must be at least as many replay block ports as radio channels.")

        # Disconnect the ports of the replay block that we want to use later on
        active_conns = rfnoc_graph.enumerate_active_connections()
        for conn in active_conns:
            # If any of the connections that we require are taken already we will
            # disconnect them here. That is especially important if both the transmitter
            # and the receiver should be used.
            if (conn.src_blockid == self.replay_blocks[0].get_unique_id() and \
                conn.src_port in self.replay_ports) or \
               (conn.dst_blockid == self.replay_blocks[0].get_unique_id() and \
                conn.dst_port in self.replay_ports):
                if "Streamer" in conn.src_blockid:
                    # Need to use streamer disconnect method for streamers
                    rfnoc_graph.disconnect(
                        conn.dst_blockid, conn.dst_port
                    )
                elif "Streamer" in conn.dst_blockid:
                    pass
                else:
                    # Disconnect for all other blocks
                    rfnoc_graph.disconnect(
                        conn.src_blockid,
                        conn.src_port,
                        conn.dst_blockid,
                        conn.dst_port,
                    )

        self.ddc_chan_pairs = []
        for replay_port_idx, rcp in enumerate(self.radio_chan_pairs):
            edge = rfnoc.connect_through_blocks(
                rfnoc_graph,
                rcp[0].get_unique_id(), rcp[1],
                self.replay_blocks[0].get_unique_id(), self.replay_ports[replay_port_idx], True)
            ddc_edge = \
                next(filter(lambda edge: rfnoc.BlockID(edge.dst_blockid).match("DDC"), edge), None)
            if ddc_edge:
                self.ddc_chan_pairs.append((
                    rfnoc.DdcBlockControl(rfnoc_graph.get_block(ddc_edge.dst_blockid)),
                    ddc_edge.dst_port
                ))
            else:
                self.ddc_chan_pairs.append(None)

        # Assign memory regions to each input port (and leave space for potential output ports)
        if mem_regions is None:
            mem_stride = self.replay_blocks[0].get_mem_size() \
                // self.replay_blocks[0].get_num_input_ports()
            mem_stride -= mem_stride % self.word_size
            self.mem_regions = [
                (idx * mem_stride, mem_stride)
                for idx in range(self.replay_blocks[0].get_num_input_ports())
            ]
        else:
            self.mem_regions = self._sanitize_mem_regions(mem_regions)
        # This stores how much data we have currently downloaded to memory. We
        # initialize this with the full memory, not zero.
        self.download_size = [x[1] for x in self.mem_regions]


    def _sanitize_replay_ports(self, ports):
        """
        Helper function to turn ports into a valid list of ports on the replay
        block.
        """
        if ports is None:
            ports = self.replay_ports
        if isinstance(ports, int):
            ports = [ports]
        if any((port >= self.replay_blocks[0].get_num_input_ports() for port in ports)):
            raise RuntimeError(
                    f"Invalid input port on replay block! Available ports: "
                    f"{self.replay_blocks[0].get_num_input_ports()} Requested: "
                    f"{ports}")
        return ports

    def _sanitize_mem_regions(self, custom_mem_regions=None):
        """
        Helper function to make a list of memory regions valid for this replay
        block and with given custom memory regions.
        """
        # If called without custom regions just return what we already have
        if custom_mem_regions is None:
            return self.mem_regions
        mem_regions = [(0,0)] * self.replay_blocks[0].get_num_input_ports()
        # If custom region is big enough for all possible ports, use it
        if len(mem_regions) == len(custom_mem_regions):
            return custom_mem_regions
        # Otherwise we have to distribute the given regions among the available ports
        for idx, mem_region in enumerate(custom_mem_regions):
            if idx < len(self.replay_ports):
                mem_regions[self.replay_ports[idx]] = mem_region
        return mem_regions

    def _download(self, waveform, mem_start, mem_size):
        """
        Download helper function

        Arguments:
        waveform: 1-dimensional numpy array with waveform data. Must be of the
                  same data type as specified during the constructor.
        mem_start: Memory address where the data should be downloaded from
        mem_size: Amount of available memory. This is the maximum length that 
                  a captured waveform can have.
        """
        # Sanitize parameters
        assert mem_start < self.replay_blocks[0].get_mem_size(), \
            f"Invalid memory start location: {mem_start}"
        assert mem_start + mem_size <= self.replay_blocks[0].get_mem_size(), \
            f"Invalid memory range: {mem_start} - {mem_start + mem_size}"
        assert mem_start % self.word_size == 0, \
            f"Memory region start (0x{mem_start:X}) is not aligned with " \
            f"word size ({self.word_size})!"
        assert mem_size % self.word_size == 0, \
            f"Memory region size (0x{mem_size:X}) is not aligned with " \
            f"word size ({self.word_size})!"
        num_items = min(len(waveform), int(mem_size) // self.bytes_per_sample)
        num_bytes = num_items * self.bytes_per_sample
        out_port = self.replay_out_port
        stream_cmd = StreamCMD(StreamMode.num_done)
        stream_cmd.num_samps = num_items
        stream_cmd.time_spec = TimeSpec(0.0)
        self.replay_blocks[0].config_play(mem_start, num_bytes, out_port)
        self.rx_streamer.issue_stream_cmd(stream_cmd)

        if not self.receive_metadata:
            self.receive_metadata = RXMetadata()
        num_items = self.rx_streamer.recv(waveform, self.receive_metadata, 15.0)
        if self.receive_metadata.error_code != RXMetadataErrorCode.none:
            # While the error code might be overwritten by the next call to _download(),
            # returning 0 will lead to recv() returning 0, too, which indicates an error.
            return 0
        return num_items

    def download(self, waveform, ports=None, mem_regions=None):
        """
        Download a waveform from memory to host

        Arguments:
        ports: If this argument is given, then we use the mem_regions attribute
               of this class to identify where the waveform is stored. If ports
               is a list, then the waveform will be downloaded once per port from
               the corresponding memory region.
        mem_regions: If this argument is given, ports is ignored. This will
                     directly specify the memory regions stored in this object.
                     NOTE: This class attempts to keep track of how many samples
                     have been sampled into memory. If mem_regions is provided,
                     then the mapping between ports and memory regions is lost,
                     and this class cannot know how many samples are available
                     for a given port. Therefore, use this argument to
                     deliberately override where sample data should be read from.

        If port is not specified, download from all ports.
        """
        if mem_regions:
            ports = None
        else:
            ports = self._sanitize_replay_ports(ports)
            # Sanitize again in case self.mem_regions was changed from the outside
            mem_regions = self._sanitize_mem_regions(self.mem_regions)
            mem_regions = [ mem_regions[port] for port in ports]
        if len(mem_regions) == 2 and \
                isinstance(mem_regions, (tuple, list)) and \
                isinstance(mem_regions[0], int):
            mem_regions = [mem_regions]

        # If this method is called from recv() then we will work on a per-channel
        # basis. Otherwise we will walk through the mem_regions that we have put
        # together above.
        if len(waveform.shape) == 1:
            bytes_downloaded = self._download(waveform, *mem_regions[0])
            self.download_size[ports[0]] = bytes_downloaded
        else:
            for region_idx, mem_region in enumerate(mem_regions):
                if ports is None or region_idx < len(self.radio_chan_pairs):
                    bytes_downloaded = self._download(waveform[region_idx], *mem_region)
                    if ports:
                        self.download_size[ports[region_idx]] = bytes_downloaded

    def issue_stream_cmd(self, stream_cmd, ports=None):
        """
        Issue a command to start or stop the streaming to DRAM.

        If ports is not specified, issue the stream command on all ports.
        """
        assert stream_cmd.stream_mode == StreamMode.num_done, \
            f"Invalid stream mode: {stream_cmd.stream_mode}"
        ports = self._sanitize_replay_ports(ports)
        mem_regions = self._sanitize_mem_regions(self.mem_regions)
        # Create a copy of the pointer to the original stream command to be able to edit it in case
        # we have to adjust the number of samples that the radio will send.
        tmp_stream_cmd = stream_cmd
        for idx, rcp in enumerate(self.radio_chan_pairs):
            stream_cmd = tmp_stream_cmd
            # Flush data on output buffer
            flush_timeout = time.monotonic() + .25
            while time.monotonic() < flush_timeout:
                if self.replay_blocks[0].get_record_fullness(ports[idx]) == 0:
                    break
                self.replay_blocks[0].record_restart(ports[idx])
            mem_region = mem_regions[ports[idx]]
            mem_size = min(stream_cmd.num_samps * self.bytes_per_sample, mem_region[1])
            self.replay_blocks[0].record(mem_region[0], mem_size, ports[idx])
            # In case we're using a DDC, we need to adjust the number of samples that the radio
            # will send, so that after down-converting it meets what the replay block expects.
            if self.ddc_chan_pairs[idx]:
                rate_ratio = self.ddc_chan_pairs[idx][0].get_input_rate(
                    self.ddc_chan_pairs[idx][1]) / \
                    self.ddc_chan_pairs[idx][0].get_output_rate(
                    self.ddc_chan_pairs[idx][1])
                stream_cmd = StreamCMD(StreamMode.num_done)
                stream_cmd.num_samps = int(tmp_stream_cmd.num_samps * rate_ratio)
                stream_cmd.stream_now = tmp_stream_cmd.stream_now
                stream_cmd.time_spec = tmp_stream_cmd.time_spec
            rcp[0].issue_stream_cmd(stream_cmd, rcp[1])

        timeout = time.monotonic() + 15.0
        while any((self.replay_blocks[0].get_record_fullness(ports[idx]) < mem_size
            for idx, _ in enumerate(self.radio_chan_pairs))):
            time.sleep(0.200)
            if time.monotonic() > timeout:
                raise RuntimeError("Timeout while loading replay buffer!")

    def recv(self, data, metadata, timeout=0.1):
        """
        This is a wrapper around download() that can be used to use this class
        like you would use an RxStreamer object.

        Unlike RxStreamer.recv(), you cannot call this repeatedly with small
        packets. Every call to this method will overwrite the previous call's
        data.

        Unlike RxStreamer.recv(), DramReceiver.recv() streams the received data
        from the replay block to the host on a per-channel basis. Therefore it
        generates RxMetadata for each channel which is not merged into a single
        metadata object. Instead, the last received metadata object is stored.
        If no error occured this should be the same for all channels. If an error
        occurs, the _download() method will throw an error which differs from the
        behavior of the RxStreamer.recv() method.

        The timeout parameter is unused, it is only there to retain the call
        signature compatibility. Time specs are pulled from the RX metadata object.
        """
        num_chans = len(self.radio_chan_pairs)
        self.receive_metadata = metadata
        if num_chans == 1:
            self.download(data, self.replay_ports[0])
        else:
            for idx, _ in enumerate(self.radio_chan_pairs):
                self.download(data[idx], self.replay_ports[idx])
        return min(self.download_size)
