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
#include <uhd/convert.hpp>
#include <uhd/usrp/rfnoc/block_ctrl_base.hpp>
#include <uhd/usrp/rfnoc/constants.hpp>

using namespace uhd;
using namespace uhd::rfnoc;

/***********************************************************************
 * Helpers
 **********************************************************************/
//! Convert register to a peek/poke compatible address
inline boost::uint32_t _sr_to_addr(boost::uint32_t reg) { return reg * 4; };
inline boost::uint32_t _sr_to_addr64(boost::uint32_t reg) { return reg * 8; }; // for peek64

/***********************************************************************
 * Structors
 **********************************************************************/
block_ctrl_base::block_ctrl_base(
        const make_args_t &make_args
) : _ctrl_iface(make_args.ctrl_iface),
    _tree(make_args.tree),
    _transport_is_big_endian(make_args.is_big_endian),
    _ctrl_sid(make_args.ctrl_sid)
{
    UHD_MSG(status) << "block_ctrl_base()" << std::endl;

    /*** Identify this block (NoC-ID, block-ID, and block definition) *******/
    // Read NoC-ID (name is passed in through make_args):
    boost::uint64_t noc_id = sr_read64(SR_READBACK_REG_ID);
    _block_def = blockdef::make_from_noc_id(noc_id);
    if (_block_def) UHD_MSG(status) <<  "Found valid blockdef" << std::endl;
    if (not _block_def)
        _block_def = blockdef::make_from_noc_id(DEFAULT_NOC_ID);
    UHD_ASSERT_THROW(_block_def);
    // For the block ID, we start with block count 0 and increase until
    // we get a block ID that's not already registered:
    _block_id.set(make_args.device_index, make_args.block_name, 0);
    while (_tree->exists("xbar/" + _block_id.get_local())) {
        _block_id++;
    }
    UHD_MSG(status)
        << "NOC ID: " << str(boost::format("0x%016X  ") % noc_id)
        << "Block ID: " << _block_id << std::endl;

    /*** Initialize property tree *******************************************/
    _root_path = "xbar/" + _block_id.get_local();
    _tree->create<boost::uint64_t>(_root_path / "noc_id").set(noc_id);

    /*** Input buffer sizes *************************************************/
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

    /*** Register names *****************************************************/
    blockdef::registers_t sregs = _block_def->get_settings_registers();
    BOOST_FOREACH(const std::string &reg_name, sregs.keys()) {
        _tree->create<size_t>(_root_path / "registers" / "sr" / reg_name).set(sregs.get(reg_name));
    }
    blockdef::registers_t rbacks = _block_def->get_readback_registers();
    BOOST_FOREACH(const std::string &reg_name, rbacks.keys()) {
        _tree->create<size_t>(_root_path / "registers"/ "rb" / reg_name).set(rbacks.get(reg_name));
    }

    /*** Init default block args ********************************************/
    blockdef::args_t args = _block_def->get_args();
    fs_path arg_path = _root_path / "args";
    _tree->create<std::string>(arg_path);
    // TODO: Add coercer
    // TODO: Add subscribers
    BOOST_FOREACH(const blockdef::arg_t &arg, args) {
        fs_path arg_type_path = arg_path / arg["name"] / "type";
        _tree->create<std::string>(arg_type_path).set(arg["type"]);
        fs_path arg_val_path = arg_path / arg["name"] / "value";
        if (arg["type"] == "string") {
            _tree->create<std::string>(arg_val_path);
            if (not arg["value"].empty()) {
                _tree->access<std::string>(arg_val_path).set(arg["value"]);
            }
        }
        else if (arg["type"] == "int") {
            _tree->create<int>(arg_val_path);
            if (not arg["value"].empty()) {
                _tree->access<int>(arg_val_path).set(boost::lexical_cast<int>(arg["value"]));
            }
        }
        else if (arg["type"] == "double") {
            _tree->create<double>(arg_val_path);
            if (not arg["value"].empty()) {
                _tree->access<double>(arg_val_path).set(boost::lexical_cast<double>(arg["value"]));
            }
        }
        else if (arg["type"] == "int_vector") {
            throw uhd::runtime_error("not yet implemented: int_vector");
        }
    }

    /*** Init I/O port definitions ******************************************/
    _init_port_defs("in",  _block_def->get_input_ports());
    _init_port_defs("out", _block_def->get_output_ports());
    // TODO: It's possible that the number of input sigs doesn't match the
    // number of input buffers. We should probably warn about that or
    // something.
}

block_ctrl_base::~block_ctrl_base()
{
    // nop
}

void block_ctrl_base::_init_port_defs(
            const std::string &direction,
            blockdef::ports_t ports,
            const size_t first_port_index
) {
    size_t port_index = first_port_index;
    BOOST_FOREACH(const blockdef::port_t &port_def, ports) {
        fs_path port_path = _root_path / "ports" / direction / port_index;
        if (not _tree->exists(port_path)) {
            _tree->create<blockdef::port_t>(port_path);
        }
        UHD_RFNOC_BLOCK_TRACE()  << "Adding port definition at " << port_path
            << boost::format("type = '%s' pkt_size = '%s' vlen = '%s'") % port_def["type"] % port_def["pkt_size"] % port_def["vlen"]
            << std::endl;
        _tree->access<blockdef::port_t>(port_path).set(port_def);
        port_index++;
    }
}

/***********************************************************************
 * FPGA control & communication
 **********************************************************************/
void block_ctrl_base::sr_write(const boost::uint32_t reg, const boost::uint32_t data)
{
    UHD_MSG(status) << "  ";
    UHD_RFNOC_BLOCK_TRACE() << boost::format("sr_write(%d, %08X)") % reg % data << std::endl;
    try {
        _ctrl_iface->poke32(_sr_to_addr(reg), data);
    }
    catch(const std::exception &ex) {
        throw uhd::io_error(str(boost::format("[%s] sr_write() failed: %s") % get_block_id().get() % ex.what()));
    }
}

void block_ctrl_base::sr_write(const std::string &reg, const boost::uint32_t data)
{
    if (not _tree->exists(_root_path / "registers" / "sr" / reg)) {
        throw uhd::key_error(str(
                boost::format("Invalid settings register name: %s")
                % reg
        ));
    }
    return sr_write(
            boost::uint32_t(_tree->access<size_t>(_root_path / "registers" / "sr" / reg).get()),
            data
    );
}

boost::uint64_t block_ctrl_base::sr_read64(const settingsbus_reg_t reg)
{
    try {
        return _ctrl_iface->peek64(_sr_to_addr64(reg));
    }
    catch(const std::exception &ex) {
        throw uhd::io_error(str(boost::format("[%s] sr_read64() failed: %s") % get_block_id().get() % ex.what()));
    }
}

boost::uint32_t block_ctrl_base::sr_read32(const settingsbus_reg_t reg) {
    try {
        return _ctrl_iface->peek32(_sr_to_addr(reg));
    }
    catch(const std::exception &ex) {
        throw uhd::io_error(str(boost::format("[%s] sr_read32() failed: %s") % get_block_id().get() % ex.what()));
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

boost::uint64_t block_ctrl_base::user_reg_read64(const std::string &reg)
{
    if (not _tree->exists(_root_path / "registers" / "rb" / reg)) {
        throw uhd::key_error(str(
                boost::format("Invalid readback register name: %s")
                % reg
        ));
    }
    return user_reg_read64(boost::uint32_t(
        _tree->access<size_t>(_root_path / "registers" / "rb" / reg).get()
    ));
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
        throw uhd::io_error(str(boost::format("[%s] user_reg_read32() failed: %s") % get_block_id().get() % ex.what()));
    }
}

boost::uint32_t block_ctrl_base::user_reg_read32(const std::string &reg)
{
    if (not _tree->exists(_root_path / "registers" / "rb" / reg)) {
        throw uhd::key_error(str(
                boost::format("Invalid readback register name: %s")
                % reg
        ));
    }
    return user_reg_read32(boost::uint32_t(
        _tree->access<size_t>(_root_path / "registers" / "sr" / reg).get()
    ));
}

void block_ctrl_base::clear()
{
    UHD_RFNOC_BLOCK_TRACE() << "block_ctrl_base::clear() " << std::endl;
    // Call parent...
    node_ctrl_base::clear();
    // TODO: Reset stream signatures to defaults from block definition
    // Call block-specific reset
    // ...then child
    _clear();
}

boost::uint32_t block_ctrl_base::get_address(size_t block_port) {
    return (_ctrl_sid.get_dst() & 0xFFF0) | (block_port & 0xF);
}

/***********************************************************************
 * Argument handling
 **********************************************************************/
void block_ctrl_base::set_args(const uhd::device_addr_t &args)
{
    BOOST_FOREACH(const std::string &key, _tree->list(_root_path / "args")) {
        if (args.has_key(key)) {
            set_arg(key, args.get(key));
        }
    }
}

void block_ctrl_base::set_arg(const std::string &key, const std::string &val)
{
    fs_path arg_path = _root_path / "args" / key;
    if (not _tree->exists(arg_path / "value")) {
        throw uhd::runtime_error(str(
                boost::format("Attempting to set uninitialized argument '%s' on block '%s'")
                % key % unique_id()
        ));
    }

    std::string type = _tree->access<std::string>(arg_path / "type").get();
    fs_path arg_val_path = arg_path / "value";
    try {
        if (type == "string") {
            _tree->access<std::string>(arg_val_path).set(val);
        }
        else if (type == "int") {
            _tree->access<int>(arg_val_path).set(boost::lexical_cast<int>(val));
        }
        else if (type == "double") {
            _tree->access<double>(arg_val_path).set(boost::lexical_cast<double>(val));
        }
        else if (type == "int_vector") {
            throw uhd::runtime_error("not yet implemented: int_vector");
        }
    } catch (const boost::bad_lexical_cast &) {
        throw uhd::value_error(str(
                    boost::format("Error trying to cast value %s == '%s' to type '%s'")
                    % key % val % type
        ));
    }
}

device_addr_t block_ctrl_base::get_args() const
{
    device_addr_t args;
    BOOST_FOREACH(const std::string &key, _tree->list(_root_path / "args")) {
        args[key] = get_arg(key);
    }
    return args;
}

std::string block_ctrl_base::get_arg(const std::string &key) const
{
    fs_path arg_path = _root_path / "args" / key;
    if (not _tree->exists(arg_path / "value")) {
        throw uhd::runtime_error(str(
                boost::format("Attempting to get uninitialized argument '%s' on block '%s'")
                % key % unique_id()
        ));
    }

    std::string type = _tree->access<std::string>(arg_path / "type").get();
    fs_path arg_val_path = arg_path / "value";
    if (type == "string") {
        return _tree->access<std::string>(arg_val_path).get();
    }
    else if (type == "int") {
        return boost::lexical_cast<std::string>(_tree->access<int>(arg_val_path).get());
    }
    else if (type == "double") {
        return boost::lexical_cast<std::string>(_tree->access<double>(arg_val_path).get());
    }
    else if (type == "int_vector") {
        throw uhd::runtime_error("not yet implemented: int_vector");
    }

    UHD_THROW_INVALID_CODE_PATH();
    return "";
}

stream_sig_t block_ctrl_base::_resolve_port_def(const blockdef::port_t &port_def) const
{
    if (not port_def.is_valid()) {
        throw uhd::runtime_error(str(
                boost::format("Invalid port definition: %s") % port_def.to_string()
        ));
    }
    UHD_RFNOC_BLOCK_TRACE() << "_resolve_port_def()" << std::endl;

    // TODO this entire section is pretty dumb at this point. Needs better
    // checks.
    stream_sig_t stream_sig;
    // Item Type
    if (port_def.is_variable("type")) {
        std::string var_name = port_def["type"].substr(1);
        // TODO check this is even a string
        stream_sig.item_type = get_arg(var_name);
    } else if (port_def.is_keyword("type")) {
        throw uhd::runtime_error("keywords resolution for type not yet implemented");
    } else {
        stream_sig.item_type = port_def["type"];
    }
    UHD_RFNOC_BLOCK_TRACE() << "  item type: " << stream_sig.item_type << std::endl;

    // Vector length
    if (port_def.is_variable("vlen")) {
        std::string var_name = port_def["vlen"].substr(1);
        stream_sig.vlen = boost::lexical_cast<size_t>(get_arg(var_name));
    } else if (port_def.is_keyword("vlen")) {
        throw uhd::runtime_error("keywords resolution for vlen not yet implemented");
    } else {
        stream_sig.vlen = boost::lexical_cast<size_t>(port_def["vlen"]);
    }
    UHD_RFNOC_BLOCK_TRACE() << "  vector length: " << stream_sig.vlen << std::endl;

    // Packet size
    if (port_def.is_variable("pkt_size")) {
        std::string var_name = port_def["pkt_size"].substr(1);
        stream_sig.packet_size = boost::lexical_cast<size_t>(get_arg(var_name));
    } else if (port_def.is_keyword("pkt_size")) {
        if (port_def["pkt_size"] != "%vlen") {
            throw uhd::runtime_error("generic keywords resolution for pkt_size not yet implemented");
        }
        if (stream_sig.vlen == 0) {
            stream_sig.packet_size = 0;
        } else {
            if (stream_sig.item_type.empty()) {
                throw uhd::runtime_error("cannot resolve pkt_size if item type is not given");
            }
            size_t bpi = uhd::convert::get_bytes_per_item(stream_sig.item_type);
            stream_sig.packet_size = stream_sig.vlen * bpi;
        }
    } else {
        stream_sig.packet_size = boost::lexical_cast<size_t>(port_def["pkt_size"]);
    }
    UHD_RFNOC_BLOCK_TRACE() << "  packet size: " << stream_sig.vlen << std::endl;

    return stream_sig;
}


/***********************************************************************
 * Hooks & Derivables
 **********************************************************************/
void block_ctrl_base::_clear()
{
    UHD_RFNOC_BLOCK_TRACE() << "block_ctrl_base::_clear() " << std::endl;
    sr_write(SR_FLOW_CTRL_CLR_SEQ, 0x00C1EA12); // 'CLEAR', but we can write anything, really
}

// vim: sw=4 et:
