//
// Copyright 2014-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

// This file contains the block control functions for block controller classes.
// See block_ctrl_base_factory.cpp for discovery and factory functions.

#include "nocscript/block_iface.hpp"

#include <uhd/utils/log.hpp>
#include <uhd/convert.hpp>
#include <uhd/rfnoc/block_ctrl_base.hpp>
#include <uhd/rfnoc/constants.hpp>
#include <uhdlib/utils/compat_check.hpp>
#include <uhdlib/rfnoc/ctrl_iface.hpp>
#include <uhdlib/rfnoc/wb_iface_adapter.hpp>
#include <boost/format.hpp>
#include <boost/bind.hpp>

using namespace uhd;
using namespace uhd::rfnoc;
using std::string;

/***********************************************************************
 * Structors
 **********************************************************************/
block_ctrl_base::block_ctrl_base(
        const make_args_t &make_args
) : _tree(make_args.tree),
    _ctrl_ifaces(make_args.ctrl_ifaces),
    _base_address(make_args.base_address & 0xFFF0),
    _noc_id(sr_read64(SR_READBACK_REG_ID)),
    _compat_num(sr_read64(SR_READBACK_COMPAT))
{
    /*** Identify this block (NoC-ID, block-ID, and block definition) *******/
    // Read NoC-ID (name is passed in through make_args):
    _block_def = blockdef::make_from_noc_id(_noc_id);
    if (not _block_def) {
        UHD_LOG_DEBUG("RFNOC",
            "No block definition found, using default block configuration "
            "for block with NOC ID: " + str(boost::format("0x%08X") % _noc_id)
        );
        _block_def = blockdef::make_from_noc_id(DEFAULT_NOC_ID);
    }
    UHD_ASSERT_THROW(_block_def);
    // For the block ID, we start with block count 0 and increase until
    // we get a block ID that's not already registered:
    _block_id.set(make_args.device_index, make_args.block_name, 0);
    while (_tree->exists("xbar/" + _block_id.get_local())) {
        _block_id++;
    }
    UHD_LOG_INFO(unique_id(), str(
            boost::format("Initializing block control (NOC ID: 0x%016X)")
            % _noc_id
    ));

    /*** Check compat number ************************************************/
    assert_fpga_compat(
            NOC_SHELL_COMPAT_MAJOR,
            NOC_SHELL_COMPAT_MINOR,
            _compat_num,
            "noc_shell",
            unique_id(),
            false /* fail_on_minor_behind */
    );

    /*** Initialize property tree *******************************************/
    _root_path = "xbar/" + _block_id.get_local();
    _tree->create<uint64_t>(_root_path / "noc_id").set(_noc_id);

    /*** Reset block state *******************************************/
    clear();

    /*** Configure ports ****************************************************/
    size_t n_valid_input_buffers = 0;
    for (const size_t ctrl_port : get_ctrl_ports()) {
        // Set command times to sensible defaults
        set_command_tick_rate(1.0, ctrl_port);
        set_command_time(time_spec_t(0.0), ctrl_port);
        // Set source addresses:
        sr_write(SR_BLOCK_SID, get_address(ctrl_port), ctrl_port);
        // Set sink buffer sizes:
        settingsbus_reg_t reg = SR_READBACK_REG_FIFOSIZE;
        uint64_t value = sr_read64(reg, ctrl_port);
        size_t buf_size_log2 = value & 0xFF;
        size_t buf_size_bytes = BYTES_PER_LINE * (1 << buf_size_log2); // Bytes == 8 * 2^x
        if (buf_size_bytes > 0) n_valid_input_buffers++;
        _tree->create<size_t>(_root_path / "input_buffer_size" / ctrl_port).set(buf_size_bytes);
    }

    /*** Register names *****************************************************/
    blockdef::registers_t sregs = _block_def->get_settings_registers();
    for(const std::string &reg_name:  sregs.keys()) {
        if (DEFAULT_NAMED_SR.has_key(reg_name)) {
            throw uhd::runtime_error(str(
                    boost::format("Register name %s is already defined!")
                    % reg_name
            ));
        }
        _tree->create<size_t>(_root_path / "registers" / "sr" / reg_name)
            .set(sregs.get(reg_name));
    }
    blockdef::registers_t rbacks = _block_def->get_readback_registers();
    for(const std::string &reg_name:  rbacks.keys()) {
        _tree->create<size_t>(_root_path / "registers"/ "rb" / reg_name)
            .set(rbacks.get(reg_name));
    }

    /*** Init I/O port definitions ******************************************/
    _init_port_defs("in",  _block_def->get_input_ports());
    _init_port_defs("out", _block_def->get_output_ports());
    // FIXME this warning always fails until the input buffer code above is fixed
    //if (_tree->list(_root_path / "ports/in").size() != n_valid_input_buffers) {
    //    UHD_LOGGER_WARNING(unique_id()) <<
    //        boost::format("[%s] defines %d input buffer sizes, but %d input ports")
    //        % get_block_id().get()
    //        % n_valid_input_buffers
    //        % _tree->list(_root_path / "ports/in").size()
    //    ;
    //}

    /*** Init default block args ********************************************/
    _nocscript_iface = nocscript::block_iface::make(this);
    _init_block_args();
}

block_ctrl_base::~block_ctrl_base()
{
    _tree->remove(_root_path);
}

void block_ctrl_base::_init_port_defs(
            const std::string &direction,
            blockdef::ports_t ports,
            const size_t first_port_index
) {
    size_t port_index = first_port_index;
    for(const blockdef::port_t &port_def:  ports) {
        fs_path port_path = _root_path / "ports" / direction / port_index;
        if (not _tree->exists(port_path)) {
            _tree->create<blockdef::port_t>(port_path);
        }
        UHD_LOGGER_TRACE(unique_id())
            << "Adding port definition at " << port_path
            << boost::format(": type = '%s' pkt_size = '%s' vlen = '%s'")
                % port_def["type"]
                % port_def["pkt_size"]
                % port_def["vlen"]
        ;
        _tree->access<blockdef::port_t>(port_path).set(port_def);
        port_index++;
    }
}

void block_ctrl_base::_init_block_args()
{
    blockdef::args_t args = _block_def->get_args();
    fs_path arg_path = _root_path / "args";
    for (const size_t port : get_ctrl_ports()) {
        _tree->create<std::string>(arg_path / port);
    }

    // First, create all nodes.
    for (const auto& arg : args) {
        fs_path arg_type_path = arg_path / arg["port"] / arg["name"] / "type";
        _tree->create<std::string>(arg_type_path).set(arg["type"]);
        fs_path arg_val_path  = arg_path / arg["port"] / arg["name"] / "value";
        if (arg["type"] == "int_vector") { throw uhd::runtime_error("not yet implemented: int_vector"); }
        else if (arg["type"] == "int") { _tree->create<int>(arg_val_path); }
        else if (arg["type"] == "double") { _tree->create<double>(arg_val_path); }
        else if (arg["type"] == "string") { _tree->create<string>(arg_val_path); }
        else { UHD_THROW_INVALID_CODE_PATH(); }
    }
    // Next: Create all the subscribers and coercers.
    // TODO: Add coercer
#define _SUBSCRIBE_CHECK_AND_RUN(type, arg_tag, error_message) \
    _tree->access<type>(arg_val_path).add_coerced_subscriber(boost::bind((&nocscript::block_iface::run_and_check), _nocscript_iface, arg[#arg_tag], error_message))
    for (const auto& arg : args) {
        fs_path arg_val_path = arg_path / arg["port"] / arg["name"] / "value";
        if (not arg["check"].empty()) {
            if (arg["type"] == "string") { _SUBSCRIBE_CHECK_AND_RUN(string, check, arg["check_message"]); }
            else if (arg["type"] == "int") { _SUBSCRIBE_CHECK_AND_RUN(int, check, arg["check_message"]); }
            else if (arg["type"] == "double") { _SUBSCRIBE_CHECK_AND_RUN(double, check, arg["check_message"]); }
            else if (arg["type"] == "int_vector") { throw uhd::runtime_error("not yet implemented: int_vector"); }
            else { UHD_THROW_INVALID_CODE_PATH(); }
        }
        if (not arg["action"].empty()) {
            if (arg["type"] == "string") { _SUBSCRIBE_CHECK_AND_RUN(string, action, ""); }
            else if (arg["type"] == "int") { _SUBSCRIBE_CHECK_AND_RUN(int, action, ""); }
            else if (arg["type"] == "double") { _SUBSCRIBE_CHECK_AND_RUN(double, action, ""); }
            else if (arg["type"] == "int_vector") { throw uhd::runtime_error("not yet implemented: int_vector"); }
            else { UHD_THROW_INVALID_CODE_PATH(); }
        }
    }

    // Finally: Set the values. This will call subscribers, if we have any.
    for (const auto& arg : args) {
        fs_path arg_val_path = arg_path / arg["port"] / arg["name"] / "value";
        if (not arg["value"].empty()) {
            if (arg["type"] == "int_vector") { throw uhd::runtime_error("not yet implemented: int_vector"); }
            else if (arg["type"] == "int") { _tree->access<int>(arg_val_path).set(std::stoi(arg["value"])); }
            else if (arg["type"] == "double") { _tree->access<double>(arg_val_path).set(std::stod(arg["value"])); }
            else if (arg["type"] == "string") { _tree->access<string>(arg_val_path).set(arg["value"]); }
            else { UHD_THROW_INVALID_CODE_PATH(); }
        }
    }
}

/***********************************************************************
 * FPGA control & communication
 **********************************************************************/
timed_wb_iface::sptr block_ctrl_base::get_ctrl_iface(const size_t block_port)
{
    return boost::make_shared<wb_iface_adapter>(
        _ctrl_ifaces[block_port],
        boost::bind(&block_ctrl_base::get_command_tick_rate, this, block_port),
        boost::bind(&block_ctrl_base::set_command_time, this, _1, block_port),
        boost::bind(&block_ctrl_base::get_command_time, this, block_port)
    );
}

std::vector<size_t> block_ctrl_base::get_ctrl_ports() const
{
    std::vector<size_t> ctrl_ports;
    ctrl_ports.reserve(_ctrl_ifaces.size());
    std::pair<size_t, ctrl_iface::sptr> it;
    for(auto it:  _ctrl_ifaces) {
        ctrl_ports.push_back(it.first);
    }
    return ctrl_ports;
}

void block_ctrl_base::sr_write(const uint32_t reg, const uint32_t data, const size_t port)
{
    if (not _ctrl_ifaces.count(port)) {
        throw uhd::key_error(str(boost::format("[%s] sr_write(): No such port: %d") % get_block_id().get() % port));
    }
    try {
        _ctrl_ifaces[port]->send_cmd_pkt(
            reg, data, false,
            _cmd_timespecs[port].to_ticks(_cmd_tickrates[port])
        );
    }
    catch(const std::exception &ex) {
        throw uhd::io_error(str(boost::format("[%s] sr_write() failed: %s") % get_block_id().get() % ex.what()));
    }
}

void block_ctrl_base::sr_write(const std::string &reg, const uint32_t data, const size_t port)
{
    uint32_t reg_addr = 255;
    if (DEFAULT_NAMED_SR.has_key(reg)) {
        reg_addr = DEFAULT_NAMED_SR[reg];
    } else {
        if (not _tree->exists(_root_path / "registers" / "sr" / reg)) {
            throw uhd::key_error(str(
                    boost::format("Unknown settings register name: %s")
                    % reg
            ));
        }
        reg_addr = uint32_t(_tree->access<size_t>(_root_path / "registers" / "sr" / reg).get());
    }
    return sr_write(reg_addr, data, port);
}

uint64_t block_ctrl_base::sr_read64(const settingsbus_reg_t reg, const size_t port)
{
    if (not _ctrl_ifaces.count(port)) {
        throw uhd::key_error(str(boost::format("[%s] sr_read64(): No such port: %d") % get_block_id().get() % port));
    }
    try {
        return _ctrl_ifaces[port]->send_cmd_pkt(
                SR_READBACK, reg,
                true,
                _cmd_timespecs[port].to_ticks(_cmd_tickrates[port])
        );
    }
    catch(const std::exception &ex) {
        throw uhd::io_error(str(boost::format("[%s] sr_read64() failed: %s") % get_block_id().get() % ex.what()));
    }
}

uint32_t block_ctrl_base::sr_read32(const settingsbus_reg_t reg, const size_t port)
{
    if (not _ctrl_ifaces.count(port)) {
        throw uhd::key_error(str(boost::format("[%s] sr_read32(): No such port: %d") % get_block_id().get() % port));
    }
    try {
        return uint32_t(_ctrl_ifaces[port]->send_cmd_pkt(
                SR_READBACK, reg,
                true,
                _cmd_timespecs[port].to_ticks(_cmd_tickrates[port])
        ));
    }
    catch(const std::exception &ex) {
        throw uhd::io_error(str(boost::format("[%s] sr_read32() failed: %s") % get_block_id().get() % ex.what()));
    }
}

uint64_t block_ctrl_base::user_reg_read64(const uint32_t addr, const size_t port)
{
    try {
        // TODO: When timed readbacks are used, time the second, but not the first
        // Set readback register address
        sr_write(SR_READBACK_ADDR, addr, port);
        // Read readback register via RFNoC
        return sr_read64(SR_READBACK_REG_USER, port);
    }
    catch(const std::exception &ex) {
        throw uhd::io_error(str(boost::format("%s user_reg_read64() failed: %s") % get_block_id().get() % ex.what()));
    }
}

uint64_t block_ctrl_base::user_reg_read64(const std::string &reg, const size_t port)
{
    if (not _tree->exists(_root_path / "registers" / "rb" / reg)) {
        throw uhd::key_error(str(
                boost::format("Invalid readback register name: %s")
                % reg
        ));
    }
    return user_reg_read64(uint32_t(
        _tree->access<size_t>(_root_path / "registers" / "rb" / reg).get()
    ), port);
}

uint32_t block_ctrl_base::user_reg_read32(const uint32_t addr, const size_t port)
{
    try {
        // Set readback register address
        sr_write(SR_READBACK_ADDR, addr, port);
        // Read readback register via RFNoC
        return sr_read32(SR_READBACK_REG_USER, port);
    }
    catch(const std::exception &ex) {
        throw uhd::io_error(str(boost::format("[%s] user_reg_read32() failed: %s") % get_block_id().get() % ex.what()));
    }
}

uint32_t block_ctrl_base::user_reg_read32(const std::string &reg, const size_t port)
{
    if (not _tree->exists(_root_path / "registers" / "rb" / reg)) {
        throw uhd::key_error(str(
                boost::format("Invalid readback register name: %s")
                % reg
        ));
    }
    return user_reg_read32(uint32_t(
        _tree->access<size_t>(_root_path / "registers" / "rb" / reg).get()
    ), port);
}

void block_ctrl_base::set_command_time(
        const time_spec_t &time_spec,
        const size_t port
) {
    if (port == ANY_PORT) {
        for(const size_t specific_port:  get_ctrl_ports()) {
            set_command_time(time_spec, specific_port);
        }
        return;
    }

    _cmd_timespecs[port] = time_spec;
    _set_command_time(time_spec, port);
}

time_spec_t block_ctrl_base::get_command_time(
        const size_t port
) {
    return _cmd_timespecs[port];
}

void block_ctrl_base::set_command_tick_rate(
        const double tick_rate,
        const size_t port
) {
    if (port == ANY_PORT) {
        for(const size_t specific_port:  get_ctrl_ports()) {
            set_command_tick_rate(tick_rate, specific_port);
        }
        return;
    }

    _cmd_tickrates[port] = tick_rate;
}

double block_ctrl_base::get_command_tick_rate(const size_t port)
{
    return _cmd_tickrates[port];
}

void block_ctrl_base::clear_command_time(const size_t port)
{
    _cmd_timespecs[port] = time_spec_t(0.0);
}

void block_ctrl_base::clear()
{
    UHD_LOG_TRACE(unique_id(), "block_ctrl_base::clear()");
    // Call parent...
    node_ctrl_base::clear();
    // ...then child
    for (const size_t port_index : get_ctrl_ports()) {
        _clear(port_index);
    }
}

uint32_t block_ctrl_base::get_address(size_t block_port) {
    UHD_ASSERT_THROW(block_port < 16);
    return (_base_address & 0xFFF0) | (block_port & 0xF);
}

/***********************************************************************
 * Argument handling
 **********************************************************************/
void block_ctrl_base::set_args(const uhd::device_addr_t &args, const size_t port)
{
    for(const std::string &key:  args.keys()) {
        if (_tree->exists(get_arg_path(key, port))) {
            set_arg(key, args.get(key), port);
        }
    }
}

void block_ctrl_base::set_arg(const std::string &key, const std::string &val, const size_t port)
{
    fs_path arg_path = get_arg_path(key, port);
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
            _tree->access<int>(arg_val_path).set(std::stoi(val));
        }
        else if (type == "double") {
            _tree->access<double>(arg_val_path).set(std::stod(val));
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

device_addr_t block_ctrl_base::get_args(const size_t port) const
{
    device_addr_t args;
    for(const std::string &key:  _tree->list(_root_path / "args" / port)) {
        args[key] = get_arg(key);
    }
    return args;
}

std::string block_ctrl_base::get_arg(const std::string &key, const size_t port) const
{
    fs_path arg_path = get_arg_path(key, port);
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
        return std::to_string(_tree->access<int>(arg_val_path).get());
    }
    else if (type == "double") {
        return std::to_string(_tree->access<double>(arg_val_path).get());
    }
    else if (type == "int_vector") {
        throw uhd::runtime_error("not yet implemented: int_vector");
    }

    UHD_THROW_INVALID_CODE_PATH();
}

std::string block_ctrl_base::get_arg_type(const std::string &key, const size_t port) const
{
    fs_path arg_type_path = _root_path / "args" / port / key / "type";
    return _tree->access<std::string>(arg_type_path).get();
}

stream_sig_t block_ctrl_base::_resolve_port_def(const blockdef::port_t &port_def) const
{
    if (not port_def.is_valid()) {
        throw uhd::runtime_error(str(
                boost::format("Invalid port definition: %s") % port_def.to_string()
        ));
    }

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

    // Vector length
    if (port_def.is_variable("vlen")) {
        std::string var_name = port_def["vlen"].substr(1);
        stream_sig.vlen = boost::lexical_cast<size_t>(get_arg(var_name));
    } else if (port_def.is_keyword("vlen")) {
        throw uhd::runtime_error("keywords resolution for vlen not yet implemented");
    } else {
        stream_sig.vlen = boost::lexical_cast<size_t>(port_def["vlen"]);
    }

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

    return stream_sig;
}


/***********************************************************************
 * Hooks & Derivables
 **********************************************************************/
void block_ctrl_base::_clear(const size_t port)
{
    UHD_LOG_TRACE(unique_id(), "block_ctrl_base::_clear()");
    sr_write(SR_CLEAR_TX_FC, 0x00C1EA12, port); // 'CLEAR', but we can write anything, really
    sr_write(SR_CLEAR_RX_FC, 0x00C1EA12, port); // 'CLEAR', but we can write anything, really
}

void block_ctrl_base::_set_command_time(const time_spec_t & /*time_spec*/, const size_t /*port*/)
{
    UHD_LOG_TRACE(unique_id(), "block_ctrl_base::_set_command_time()");
}
// vim: sw=4 et:
