#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

# Read Register Addresses
#! Register address of the protocol version
PROTOVER_ADDR = 0 * 4
#! Register address of the port information
PORT_CNT_ADDR = 1 * 4
#! Register address of the edge information
EDGE_CNT_ADDR = 2 * 4
#! Register address of the device information
DEVICE_INFO_ADDR = 3 * 4
#! Register address of the controlport information
CTRLPORT_CNT_ADDR = 4 * 4
#! (Write) Register address of the flush and reset controls
FLUSH_RESET_ADDR = 1 * 4

#! Base address of the adjacency list
ADJACENCY_BASE_ADDR = 0x10000
#! Each port is allocated this many registers in the backend register space
REGS_PER_PORT = 16

RX_CMD_CONTINUOUS = 0x2
RX_CMD_STOP = 0x0
RX_CMD_FINITE = 0x1

RADIO_BASE_ADDR = 0x1000
REG_CHAN_OFFSET = 1024 # 0x400 in hex

# Radio block register addresses (see radio_control_impl.hpp)
REG_COMPAT_NUM = 0x00
REG_TIME_LO = 0x04
REG_TIME_HI = 0x08
REG_RADIO_WIDTH = 0x1004  # Upper 16 bits is sample width, lower 16 bits is NSPC

# General Radio Registers (at RADIO_BASE_ADDR + offset)
REG_PORT_GENERAL_ADDR_OFFSET = 0x00
REG_LOOPBACK_EN = REG_PORT_GENERAL_ADDR_OFFSET + 0x00  # Loopback enable
REG_FEATURES_PRESENT = REG_PORT_GENERAL_ADDR_OFFSET + 0x08  # Feature flags

# RX Core Control Registers (per-channel offsets from RADIO_BASE_ADDR + chan*REG_CHAN_OFFSET)
REG_PORT_RX_ADDR_OFFSET = 0x100
REG_RX_STATUS = REG_PORT_RX_ADDR_OFFSET + 0x00
REG_RX_CMD = REG_PORT_RX_ADDR_OFFSET + 0x04
REG_RX_CMD_NUM_WORDS_LO = REG_PORT_RX_ADDR_OFFSET + 0x08
REG_RX_CMD_NUM_WORDS_HI = REG_PORT_RX_ADDR_OFFSET + 0x0C
REG_RX_CMD_TIME_LO = REG_PORT_RX_ADDR_OFFSET + 0x10
REG_RX_CMD_TIME_HI = REG_PORT_RX_ADDR_OFFSET + 0x14
REG_RX_MAX_WORDS_PER_PKT = REG_PORT_RX_ADDR_OFFSET + 0x18
REG_RX_DATA = REG_PORT_RX_ADDR_OFFSET + 0x2C
REG_RX_HAS_TIME = REG_PORT_RX_ADDR_OFFSET + 0x30

# TX Core Control Registers (per-channel offsets)
REG_PORT_TX_ADDR_OFFSET = 0x200
REG_TX_IDLE_VALUE = REG_PORT_TX_ADDR_OFFSET + 0x00 # Value to output when transmitter is idle

# Feature Control Registers (per-channel offsets)
REG_PORT_FEAT_ADDR_OFFSET = 0x300

# Register address space widths and sizes (see radio_control_impl.hpp)
REG_PORT_RX_ADDR_W = 8      # Address space width in bits for RX registers
REG_PORT_TX_ADDR_W = 8      # Address space width in bits for TX registers
REG_PORT_FEAT_ADDR_W = 8    # Address space width in bits for Feature registers
REG_PORT_RX_SIZE = 2**REG_PORT_RX_ADDR_W    # 0x100, address space size for RX registers
REG_PORT_TX_SIZE = 2**REG_PORT_TX_ADDR_W    # 0x100, address space size for TX registers
REG_PORT_FEAT_SIZE = 2**REG_PORT_FEAT_ADDR_W  # 0x100, address space size for Feature registers

# Radio block compatibility version
RADIO_MAJOR_COMPAT = 1
RADIO_MINOR_COMPAT = 0


class StreamEndpointPort:
    """Represents a port on a Stream Endpoint

    inst should be the same as the stream endpoint's node_inst
    """
    def __init__(self, inst, port):
        self.inst = inst
        self.port = port
    def to_tuple(self, num_stream_ep):
        # The entry in an adjacency list is (blk_id, block_port)
        # where blk_id for stream endpoints starts at 1
        # and Noc Blocks are addressed after the last stream endpoint
        # See rfnoc_graph.cpp
        return (1 + self.inst, self.port)

class NocBlockPort:
    """Represents a port on a Noc Block"""
    def __init__(self, inst, port):
        self.inst = inst
        self.port = port

    def to_tuple(self, num_stream_ep):
        # The entry in an adjacency list is (blk_id, block_port)
        # where blk_id for stream endpoints starts at 1
        # and Noc Blocks are addressed after the last stream endpoint
        # See rfnoc_graph.cpp
        return (1 + num_stream_ep + self.inst, self.port)

class NocBlock:
    """Represents a NocBlock

    see client_zero.hpp:block_config_info

    NOTE: The mtu in bytes is calculated by (2**data_mtu * CHDR_W)
    """
    def __init__(self, protover, num_inputs, num_outputs, ctrl_fifo_size,
                 ctrl_max_async_msgs, noc_id, data_mtu):
        self.protover = protover
        self.num_inputs = num_inputs
        self.num_outputs = num_outputs
        self.ctrl_fifo_size = ctrl_fifo_size
        self.ctrl_max_async_msgs = ctrl_max_async_msgs
        self.noc_id = noc_id
        self.data_mtu = data_mtu

    def read_reg(self, reg_num):
        # See client_zero.cpp
        if reg_num == 0:
            return self.read_config()
        elif reg_num == 1:
            return self.read_noc_id()
        elif reg_num == 2:
            return self.read_data()
        else:
            raise RuntimeError("NocBlock doesn't have a register #{}".format(reg_num))

    def read_config(self):
        return (self.protover & 0x3F) | \
            ((self.num_inputs & 0x3F) << 6) | \
            ((self.num_outputs & 0x3F) << 12) | \
            ((self.ctrl_fifo_size & 0x3F) << 18) | \
            ((self.ctrl_max_async_msgs & 0xFF) << 24)

    def read_data(self):
        return (self.data_mtu & 0x3F) << 2 | \
            (1 << 1) # Permanently Set flush done

    def read_noc_id(self):
        return self.noc_id & 0xFFFFFFFF

class NocBlockRegs:
    """Represents registers associated whith a group of NoCBlocks
    roughly similar to UHD's client_zero

    NOTE: Many write operations are currently unimplemented and simply no-op
    """
    def __init__(self, log, protover, has_xbar, num_xports, blocks, num_stream_ep, num_ctrl_ep,
                 device_type, adjacency_list, sample_width, samples_per_cycle, get_stream_spec,
                 create_tx_stream, stop_tx_stream):
        """ Args:
        protover -> FPGA Compat number
        has_xbar -> Is there a chdr xbar?
        num_xports -> how many xports
        blocks -> list of NocBlock objects
        num_stream_ep -> how many stream endpoints
        num_ctrl_ep -> how many ctrl endpoints
        device_type -> the device type (see defaults.hpp:device_type_t in UHD)
        adjacency_list -> List of (Port, Port) tuples where
            Port is either StreamEndpointPort or NocBlockPort
        sample_width -> Sample width of radio
        samples_per_cycle -> Samples produced by a radio cycle
        get_stream_spec -> Callback which returns the current stream spec
        create_tx_stream -> Callback which takes a block_index and starts a tx stream
        stop_tx_stream -> Callback which takes a block_index and stops a tx stream
        """
        self.log = log.getChild("Regs")
        self.protover = protover
        self.has_xbar = has_xbar
        self.num_xports = num_xports
        self.blocks = blocks
        self.num_blocks = len(blocks)
        self.num_stream_ep = num_stream_ep
        self.num_ctrl_ep = num_ctrl_ep
        self.device_type = device_type
        self.adjacency_list = [(src_blk.to_tuple(num_stream_ep), dst_blk.to_tuple(num_stream_ep))
                               for src_blk, dst_blk in adjacency_list]
        self.adjacency_list_reg = NocBlockRegs._parse_adjacency_list(self.adjacency_list)
        self.sample_width = sample_width
        self.samples_per_cycle = samples_per_cycle
        self.radio_reg = {}
        self.get_stream_spec = get_stream_spec
        self.create_tx_stream = create_tx_stream
        self.stop_tx_stream = stop_tx_stream

    def read(self, addr):
        # See client_zero.cpp and radio_control_impl.cpp
        if addr == REG_COMPAT_NUM:
            return self.read_reg_compat_num()
        elif addr == PROTOVER_ADDR:
            return self.read_protover()
        elif addr == PORT_CNT_ADDR:
            return self.read_port_cnt()
        elif addr == EDGE_CNT_ADDR:
            return self.read_edge_cnt()
        elif addr == DEVICE_INFO_ADDR:
            return self.read_device_info()
        elif addr == CTRLPORT_CNT_ADDR:
            return self.read_ctrlport_cnt()
        elif addr >= 0x40 and addr < 0x1000:
            return self.read_port_reg(addr)
        # See radio_control_impl.cpp
        elif addr >= RADIO_BASE_ADDR and addr < ADJACENCY_BASE_ADDR:
            return self.read_radio(addr)
        # See client_zero.cpp
        elif addr >= 0x10000:
            return self.read_adjacency_list(addr)
        else:
            raise RuntimeError("Unsupported register addr: 0x{:08X}".format(addr))

    def read_radio(self, addr):
        """Read values from radio registers.
        """
        # General Radio Registers (not per-channel)
        if addr == RADIO_BASE_ADDR + REG_LOOPBACK_EN:
            self.log.trace(f"Not implemented: Loopback enable register at 0x{addr:08X}")
            return 0
        elif addr == REG_RADIO_WIDTH:
            return self.read_radio_width()
        elif addr == RADIO_BASE_ADDR + REG_FEATURES_PRESENT:
            self.log.trace(f"Not implemented: Features present register at 0x{addr:08X}")
            return 0
        else:
            # Per-channel register access
            offset = addr - RADIO_BASE_ADDR
            chan = offset // REG_CHAN_OFFSET
            radio_offset = offset % REG_CHAN_OFFSET
            
            # Handle loopback test register reads (REG_TX_IDLE_VALUE for all channels)            
            if radio_offset == REG_TX_IDLE_VALUE:
                return self.radio_reg.get(chan, 0)

            # Check if this is in the RX register space (0x100-0x1FF)
            elif REG_PORT_RX_ADDR_OFFSET <= radio_offset < REG_PORT_RX_ADDR_OFFSET + REG_PORT_RX_SIZE:
                # Handle Loopback test register reads (REG_RX_DATA for all channels)
                if radio_offset == REG_RX_DATA:
                    return self.radio_reg.get(chan, 0)
                else:
                    self.log.trace(f"Not implemented: RX register at 0x{addr:08X}")
                    return 0  # Default for unimplemented RX registers
            # Check if this is in the TX register space (0x200-0x2FF)
            elif REG_PORT_TX_ADDR_OFFSET <= radio_offset < REG_PORT_TX_ADDR_OFFSET + REG_PORT_TX_SIZE:
                self.log.trace(f"Not implemented: TX register at 0x{addr:08X}")
                return 0
            # Check if this is in the Feature register space (0x300-0x3FF)
            elif REG_PORT_FEAT_ADDR_OFFSET <= radio_offset < REG_PORT_FEAT_ADDR_OFFSET + REG_PORT_FEAT_SIZE:
                self.log.trace(f"Not implemented: Feature register at 0x{addr:08X}")
                return 0
            else:
                raise NotImplementedError("Radio addr 0x{:08X} not implemented".format(addr))

    def write(self, addr, value):
        """Write value to a register address.
        
        Assuming upto 2 channels, out of bounds is ignored for now.
        which is RADIO_BASE_ADDR + 2*REG_CHAN_OFFSET
        e.g., 0x1000 + 2*0x400 = 0x1800
        """
        if RADIO_BASE_ADDR <= addr < RADIO_BASE_ADDR + 2 * REG_CHAN_OFFSET:
            offset = addr - RADIO_BASE_ADDR
            chan = offset // REG_CHAN_OFFSET
            reg = offset % REG_CHAN_OFFSET
            # Handle loopback test register writes (REG_TX_IDLE_VALUE for all channels)
            if reg == REG_TX_IDLE_VALUE:
                self.log.trace(f"Storing value: 0x{value:08X} to self.radio_reg[{chan}] for data loopback test")
                self.radio_reg[chan] = value
            # Route other radio register writes to write_radio()
            else:
                self.write_radio(addr, value)

    def write_radio(self, addr, value):
        """Write a value to radio registers

        See radio_control_impl.cpp
        """
        offset = addr - RADIO_BASE_ADDR
        assert offset >= 0
        chan = offset // REG_CHAN_OFFSET
        # Removed channel >0 check to allow multi-channel operation
        # This needs to be tested further

        reg = offset % REG_CHAN_OFFSET
        if reg == REG_RX_MAX_WORDS_PER_PKT:
            self.get_stream_spec().packet_samples = value
        elif reg == REG_RX_CMD_NUM_WORDS_HI:
            self.get_stream_spec().set_num_words_hi(value)
        elif reg == REG_RX_CMD_NUM_WORDS_LO:
            self.get_stream_spec().set_num_words_lo(value)
        elif reg == REG_RX_CMD_TIME_HI:
            self.get_stream_spec().set_timestamp_hi(value)
        elif reg == REG_RX_CMD_TIME_LO:
            self.get_stream_spec().set_timestamp_lo(value)
        elif reg == REG_RX_CMD:
            if value & (1 << 31) != 0:
                value = value & ~(1 << 31) # Clear the flag
                self.get_stream_spec().is_timed = True
            if value == RX_CMD_STOP:
                sep_block_id = self.resolve_ep_towards_outputs((self.get_radio_port(), chan))
                self.stop_tx_stream(sep_block_id)
                return
            elif value == RX_CMD_CONTINUOUS:
                self.get_stream_spec().is_continuous = True
            elif value == RX_CMD_FINITE:
                self.get_stream_spec().is_continuous = False
            else:
                raise RuntimeError("Unknown Stream RX_CMD: {:08X}".format(value))
            sep_block_id = self.resolve_ep_towards_outputs((self.get_radio_port(), chan))
            self.create_tx_stream(sep_block_id)

    def resolve_ep_towards_outputs(self, block_id):
        """Follow dataflow downstream through the adjacency list until
        a stream_endpoint is encountered
        """
        for src_blk, dst_blk in self.adjacency_list:
            if src_blk == block_id:
                dst_index, dst_port = dst_blk
                if dst_index <= self.num_stream_ep:
                    return dst_blk
                else:
                    return self.resolve_ep_towards_outputs(dst_blk)

    def get_radio_port(self):
        """Returns the block_id of the radio block"""
        radio_noc_id = 0x12AD1000
        for i, block in enumerate(self.blocks):
            if block.noc_id == radio_noc_id:
                return i + 1 + self.num_stream_ep

    # This is the FPGA compat number
    def read_protover(self):
        return 0xFFFF & self.protover

    def read_reg_compat_num(self):
        # Register compatibility number: (MAJOR << 16) | MINOR
        return (RADIO_MAJOR_COMPAT << 16) | RADIO_MINOR_COMPAT

    def read_port_cnt(self):
        return (self.num_stream_ep & 0x3FF) | \
            ((self.num_blocks & 0x3FF) << 10) | \
            ((self.num_xports & 0x3FF) << 20) | \
            ((1 if self.has_xbar else 0) << 31)

    def read_edge_cnt(self):
        return len(self.adjacency_list)

    def read_device_info(self):
        return (self.device_type & 0xFFFF) << 16

    def read_ctrlport_cnt(self):
        return (self.num_ctrl_ep & 0x3FF)

    def read_adjacency_list(self, addr):
        offset = addr & 0xFFFF
        if offset == 0:
            self.log.debug("Adjacency List has {} entries".format(len(self.adjacency_list_reg)))
            return len(self.adjacency_list_reg)
        else:
            assert(offset % 4 == 0)
            index = (offset // 4) - 1
            return self.adjacency_list_reg[index]

    def read_port_reg(self, addr):
        port = addr // 0x40
        if port < self.num_stream_ep:
            raise NotImplementedError()
        else:
            block = port - self.num_stream_ep - 1
            offset = (addr % 0x40) // 4
            return self.blocks[block].read_reg(offset)

    def read_radio_width(self):
        return (self.samples_per_cycle & 0xFFFF) | \
            ((self.sample_width & 0xFFFF) << 16)

    @staticmethod
    def _parse_adjacency_list(adj_list):
        """Serialize an adjacency list from the form of
        [((src_blk, src_port), (dst_blk, dst_port))]

        See client_zero.cpp:client_zero#_get_adjacency_list()
        """
        def pack(blocks):
            src_blk, src_port = blocks[0]
            dst_blk, dst_port = blocks[1]
            return ((src_blk & 0x3FF) << 22) | \
                ((src_port & 0x3F) << 16) | \
                ((dst_blk & 0x3FF) << 6) | \
                ((dst_port & 0x3F) << 0)
        return [pack(blocks) for blocks in adj_list]

