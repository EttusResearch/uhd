//
// Copyright 2014-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/node_ctrl_base.hpp>
#include <uhd/utils/log.hpp>
#include <boost/range/adaptor/map.hpp>

using namespace uhd::rfnoc;

std::string node_ctrl_base::unique_id() const
{
    // Most instantiations will override this, so we don't need anything
    // more elegant here.
    return str(boost::format("%08X") % size_t(this));
}

void node_ctrl_base::clear()
{
    UHD_LOG_TRACE(unique_id(), "node_ctrl_base::clear()");
    // Reset connections:
    _upstream_nodes.clear();
    _downstream_nodes.clear();
}

void node_ctrl_base::_register_downstream_node(
    node_ctrl_base::sptr,
    size_t
) {
    throw uhd::runtime_error("Attempting to register a downstream block on a non-source node.");
}

void node_ctrl_base::_register_upstream_node(
    node_ctrl_base::sptr,
    size_t
) {
    throw uhd::runtime_error("Attempting to register an upstream block on a non-sink node.");
}

void node_ctrl_base::set_downstream_port(
        const size_t this_port,
        const size_t remote_port
) {
    if (not _downstream_nodes.count(this_port) and remote_port != ANY_PORT) {
        throw uhd::value_error(str(
            boost::format("[%s] Cannot set remote downstream port: Port %d not connected.")
            % unique_id() % this_port
        ));
    }
    _downstream_ports[this_port] = remote_port;
}

size_t node_ctrl_base::get_downstream_port(const size_t this_port)
{
    if (not _downstream_ports.count(this_port)
        or not _downstream_nodes.count(this_port)
        or _downstream_ports[this_port] == ANY_PORT) {
        throw uhd::value_error(str(
            boost::format("[%s] Cannot retrieve remote downstream port: Port %d not connected.")
            % unique_id() % this_port
        ));
    }
    return _downstream_ports[this_port];
}

void node_ctrl_base::set_upstream_port(
        const size_t this_port,
        const size_t remote_port
) {
    if (not _upstream_nodes.count(this_port) and remote_port != ANY_PORT) {
        throw uhd::value_error(str(
            boost::format("[%s] Cannot set remote upstream port: Port %d not connected.")
            % unique_id() % this_port
        ));
    }
    _upstream_ports[this_port] = remote_port;
}

size_t node_ctrl_base::get_upstream_port(const size_t this_port)
{
    if (not _upstream_ports.count(this_port)
        or not _upstream_nodes.count(this_port)
        or _upstream_ports[this_port] == ANY_PORT) {
        throw uhd::value_error(str(
            boost::format("[%s] Cannot retrieve remote upstream port: Port %d not connected.")
            % unique_id() % this_port
        ));
    }
    return _upstream_ports[this_port];
}

void node_ctrl_base::disconnect()
{
    // Notify neighbours:
    for (node_map_t::iterator i = _downstream_nodes.begin(); i != _downstream_nodes.end(); ++i) {
        sptr downstream_node = i->second.lock();
        if (not downstream_node) {
            // Actually this is not OK
            continue;
        }
        downstream_node->disconnect_input_port(_downstream_ports[i->first]);
    }
    for (node_map_t::iterator i = _upstream_nodes.begin(); i != _upstream_nodes.end(); ++i) {
        sptr upstream_node = i->second.lock();
        if (not upstream_node) {
            // Actually this is not OK
            continue;
        }
        upstream_node->disconnect_output_port(_upstream_ports[i->first]);
    }
    // Clear own maps:
    _downstream_nodes.clear();
    _downstream_ports.clear();
    _upstream_nodes.clear();
    _upstream_ports.clear();
}

void node_ctrl_base::disconnect_output_port(const size_t output_port)
{
    if (_downstream_nodes.count(output_port) == 0 or
        _downstream_ports.count(output_port) == 0) {
        throw uhd::assertion_error(str(boost::format("[%s] Attempting to disconnect output port %u, which is not registered as connected!") % unique_id() % output_port));
    }
    _downstream_nodes.erase(output_port);
    _downstream_ports.erase(output_port);
}

void node_ctrl_base::disconnect_input_port(const size_t input_port)
{
    if (_upstream_nodes.count(input_port) == 0 or
        _upstream_ports.count(input_port) == 0) {
        throw uhd::assertion_error(str(boost::format("[%s] Attempting to disconnect input port %u, which is not registered as connected!") % unique_id() % input_port));
    }
    _upstream_nodes.erase(input_port);
    _upstream_ports.erase(input_port);
}

