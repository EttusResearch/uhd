//
// Copyright 2014 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

// This file contains the block control functions for block controller classes.
// See block_ctrl_base_factory.cpp for discovery and factory functions.

#include <boost/format.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/usrp/rfnoc/block_ctrl_base.hpp>

using namespace uhd;
using namespace uhd::rfnoc;

//! Convert register to a peek/poke compatible address
inline boost::uint32_t _sr_to_addr(boost::uint32_t reg) { return reg * 4; };
inline boost::uint32_t _sr_to_addr64(boost::uint32_t reg) { return reg * 8; }; // for peek64

// One line in FPGA is 64 Bits
static const size_t BYTES_PER_LINE = 8;

block_ctrl_base::block_ctrl_base(
        const make_args_t &make_args
) : _ctrl_iface(make_args.ctrl_iface),
    _tree(make_args.tree),
    _transport_is_big_endian(make_args.is_big_endian),
    _ctrl_sid(make_args.ctrl_sid)
{
    UHD_MSG(status) << "block_ctrl_base()" << std::endl;
    // Read NoC-ID
    boost::uint64_t noc_id = sr_read64(SR_READBACK_REG_ID);
    UHD_MSG(status) << "NOC ID: " << str(boost::format("0x%016X") % noc_id) << std::endl;

    std::string blockname = make_args.block_name;

    _block_id.set(make_args.device_index, blockname, 0);
    while (_tree->exists("xbar/" + _block_id.get_local())) {
        _block_id++;
    }
    UHD_MSG(status) << "Using block ID: " << _block_id << std::endl;

    // Populate property tree
    _root_path = "xbar/" + _block_id.get_local();
    _tree->create<boost::uint64_t>(_root_path / "noc_id").set(noc_id);

    // Read buffer sizes (also, identifies which ports may receive connections)
    std::vector<size_t> buf_sizes(16, 0);
    for (size_t port_offset = 0; port_offset < 16; port_offset += 8) {
        // FIXME: Eventually need to implement per block port buffers
        // settingsbus_reg_t reg =
        //     (port_offset == 0) ? SR_READBACK_REG_BUFFALLOC0 : SR_READBACK_REG_BUFFALLOC1;
        settingsbus_reg_t reg = SR_READBACK_REG_BUFFALLOC0;
        boost::uint64_t value = sr_read64(reg);
        for (size_t i = 0; i < 8; i++) {
            // FIXME: See above
            // size_t buf_size_log2 = (value >> (i * 8)) & 0xFF; // Buffer size in x = log2(lines)
            size_t buf_size_log2 = value & 0xFF;
            size_t buf_size_bytes = BYTES_PER_LINE * (1 << buf_size_log2); // Bytes == 8 * 2^x
            buf_sizes[i + port_offset] = BYTES_PER_LINE * (1 << buf_size_log2); // Bytes == 8 * 2^x
            _tree->create<size_t>(
                    _root_path / str(boost::format("input_buffer_size/%d") % size_t(i + port_offset))
            ).set(buf_size_bytes);
        }
    }

    // Add I/O signature
    // TODO actually use values from the block definition
    _tree->create<stream_sig_t>(_root_path / "input_sig/0").set(stream_sig_t("sc16", 0));
    // FIXME default packet size?
    _tree->create<stream_sig_t>(_root_path / "output_sig/0").set(stream_sig_t("sc16", 0, DEFAULT_PACKET_SIZE));
}

block_ctrl_base::~block_ctrl_base()
{
    // nop
}


void block_ctrl_base::sr_write(const boost::uint32_t reg, const boost::uint32_t data)
{
    UHD_MSG(status) << "[" << get_block_id() << "]: " << str(boost::format("sr_write(%d, %08X)") % reg % data) << std::endl;
    try {
        _ctrl_iface->poke32(_sr_to_addr(reg), data);
    }
    catch(const std::exception &ex) {
        throw uhd::io_error(str(boost::format("%s sr_write() failed: %s") % get_block_id().get() % ex.what()));
    }
}

boost::uint64_t block_ctrl_base::sr_read64(const settingsbus_reg_t reg)
{
    try {
        return _ctrl_iface->peek64(_sr_to_addr64(reg));
    }
    catch(const std::exception &ex) {
        throw uhd::io_error(str(boost::format("%s sr_write() failed: %s") % get_block_id().get() % ex.what()));
    }
}

boost::uint32_t block_ctrl_base::sr_read32(const settingsbus_reg_t reg) {
    try {
        return _ctrl_iface->peek32(_sr_to_addr(reg));
    }
    catch(const std::exception &ex) {
        throw uhd::io_error(str(boost::format("%s sr_write() failed: %s") % get_block_id().get() % ex.what()));
    }
}

boost::uint64_t block_ctrl_base::user_reg_read64(const boost::uint32_t addr)
{
    try {
        // Set readback register address
        sr_write(SR_READBACK_ADDR, addr);
        // Read readback register via RFNoC
        return sr_read64(SR_READBACK_REG_USER);
    }
    catch(const std::exception &ex) {
        throw uhd::io_error(str(boost::format("%s user_reg_read64() failed: %s") % get_block_id().get() % ex.what()));
    }
}

boost::uint32_t block_ctrl_base::user_reg_read32(const boost::uint32_t addr)
{
    try {
        // Set readback register address
        sr_write(SR_READBACK_ADDR, addr);
        // Read readback register via RFNoC
        return sr_read32(SR_READBACK_REG_USER);
    }
    catch(const std::exception &ex) {
        throw uhd::io_error(str(boost::format("%s user_reg_read64() failed: %s") % get_block_id().get() % ex.what()));
    }
}

size_t block_ctrl_base::get_fifo_size(size_t block_port) const {
    if (_tree->exists(_root_path / "input_buffer_size" / str(boost::format("%d") % block_port))) {
        return _tree->access<size_t>(_root_path / "input_buffer_size" / str(boost::format("%d") % block_port)).get();
    }
    return 0;
}

boost::uint32_t block_ctrl_base::get_address(size_t block_port) {
    return (_ctrl_sid.get_dst() & 0xFFF0) | (block_port & 0xF);
}

void block_ctrl_base::configure_flow_control_in(
        size_t cycles,
        size_t packets,
        size_t block_port)
  {
    UHD_MSG(status) << "block_ctrl_base::configure_flow_control_in() " << cycles << " " << packets << std::endl;
    boost::uint32_t cycles_word = 0;
    if (cycles) {
        cycles_word = (1<<31) | cycles;
    }
    sr_write(SR_FLOW_CTRL_CYCS_PER_ACK_BASE + block_port, cycles_word);

    boost::uint32_t packets_word = 0;
    if (packets) {
        packets_word = (1<<31) | packets;
    }
    sr_write(SR_FLOW_CTRL_PKTS_PER_ACK_BASE + block_port, packets_word);
}

void block_ctrl_base::configure_flow_control_out(
            size_t buf_size_pkts,
            size_t block_port,
            UHD_UNUSED(const uhd::sid_t &sid)
) {
    UHD_MSG(status) << "block_ctrl_base::configure_flow_control_out() " << buf_size_pkts << std::endl;
    // This actually takes counts between acks. So if the buffer size is 1 packet, we
    // set this to zero.
    sr_write(SR_FLOW_CTRL_WINDOW_SIZE_BASE + block_port, (buf_size_pkts == 0) ? 0 : buf_size_pkts-1);
    sr_write(SR_FLOW_CTRL_WINDOW_EN_BASE + block_port, (buf_size_pkts != 0));
}

void block_ctrl_base::clear()
{
    UHD_MSG(status) << "block_ctrl_base::clear() " << std::endl;
    // Call parent...
    node_ctrl_base::clear();
    // TODO: Reset stream signatures to defaults from block definition
    // Call block-specific reset
    // ...then child
    _clear();
}

void block_ctrl_base::_clear()
{
    UHD_MSG(status) << "block_ctrl_base::_clear() " << std::endl;
    sr_write(SR_FLOW_CTRL_CLR_SEQ, 0x00C1EA12); // 'CLEAR', but we can write anything, really
}


stream_sig_t block_ctrl_base::get_input_signature(size_t block_port) const
{
    UHD_MSG(status) << "block_ctrl_base::get_input_signature() " << std::endl;
    if (not _tree->exists(_root_path / "input_sig" / str(boost::format("%d") % block_port))) {
        throw uhd::runtime_error(str(
            boost::format("Can't query input signature on block %s: Port %d is not defined.")
            % get_block_id().to_string() % block_port
        ));
    }
    return _tree->access<stream_sig_t>(_root_path / "input_sig" / str(boost::format("%d") % block_port)).get();
}

stream_sig_t block_ctrl_base::get_output_signature(size_t block_port) const
{
    UHD_MSG(status) << "block_ctrl_base::get_output_signature() " << std::endl;
    if (not _tree->exists(_root_path / "output_sig" / str(boost::format("%d") % block_port))) {
        throw uhd::runtime_error(str(
            boost::format("Can't query output signature on block %s: Port %d is not defined.")
            % get_block_id().to_string() % block_port
        ));
    }
    return _tree->access<stream_sig_t>(_root_path / "output_sig" / str(boost::format("%d") % block_port)).get();
}

bool block_ctrl_base::set_input_signature(const stream_sig_t &in_sig, size_t block_port)
{
    UHD_MSG(status) << "block_ctrl_base::set_input_signature() " << in_sig << " " << block_port << std::endl;
    if (not _tree->exists(_root_path / "input_sig" / str(boost::format("%d") % block_port))) {
        throw uhd::runtime_error(str(
            boost::format("Can't modify input signature on block %s: Port %d is not defined.")
            % get_block_id().to_string() % block_port
        ));
    }

    if (not
        _tree->access<stream_sig_t>(_root_path / "input_sig" / str(boost::format("%d") % block_port))
        .get().is_compatible(in_sig)
    ) {
        return false;
    }

    // TODO more and better rules, check block definition
    if (in_sig.packet_size % BYTES_PER_LINE) {
        return false;
    }

    _tree->access<stream_sig_t>(_root_path / "input_sig" / str(boost::format("%d") % block_port)).set(in_sig);
    // FIXME figure out good rules to propagate the signature
    _tree->access<stream_sig_t>(_root_path / "output_sig" / str(boost::format("%d") % block_port)).set(in_sig);
    return true;
}

bool block_ctrl_base::set_output_signature(const stream_sig_t &out_sig, size_t block_port)
{
    UHD_MSG(status) << "block_ctrl_base::set_output_signature() " << out_sig << " " << block_port << std::endl;

    if (not _tree->exists(_root_path / "output_sig" / str(boost::format("%d") % block_port))) {
        throw uhd::runtime_error(str(
            boost::format("Can't modify output signature on block %s: Port %d is not defined.")
            % get_block_id().to_string() % block_port
        ));
    }

    // TODO more and better rules, check block definition
    if (out_sig.packet_size % BYTES_PER_LINE) {
        return false;
    }

    _tree->access<stream_sig_t>(_root_path / "output_sig" / str(boost::format("%d") % block_port)).set(out_sig);
    return true;
}

void block_ctrl_base::set_destination(
        boost::uint32_t next_address,
        size_t output_block_port
) {
    UHD_MSG(status) << "block_ctrl_base::set_destination() " << uhd::sid_t(next_address) << std::endl;
    sid_t new_sid(next_address);
    new_sid.set_src_addr(_ctrl_sid.get_dst_addr());
    new_sid.set_src_endpoint(_ctrl_sid.get_dst_endpoint() + output_block_port);
    UHD_LOG << "In block: " << _block_id << "Setting SID: " << new_sid << std::endl;
    sr_write(SR_NEXT_DST_BASE+output_block_port, (1<<16) | next_address);
}

// vim: sw=4 et:
