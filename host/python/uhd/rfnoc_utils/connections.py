"""Image builder connections module.

Copyright 2019 Ettus Research, A National Instrument Brand

SPDX-License-Identifier: GPL-3.0-or-later

Functions, helpers, and other code to handle connections in the image builder.
"""

import copy
import re
import sys

from .common import NONE_PORT


def con2str(con):
    """Pretty-print a single connection."""
    return f"{con['srcblk']}:{con['srcport']} → {con['dstblk']}:{con['dstport']}"


def _check_duplicate_connections(config):
    """Check that ports don't have duplicate connections.

    Checks that a port that is referenced in a connection is not referenced in
    any other connection. The exception are broadcasters, which can have multiple
    destinations.

    As a reminder, the connections object looks something like this:
    [
        {'srcblk': src1, 'srcport': srcport1, 'dstblk': dst1, 'dstport': dstport1},
        {'srcblk': src2, 'srcport': srcport2, 'dstblk': dst2, 'dstport': dstport2},
        ...
    ]

    A duplicate connection is any tuple of connections where either the srcblk
    and srcport or the dstblk and dstport are the same (again, with the exception
    of broadcasters).
    """
    for con_idx, con in enumerate(config.connections):
        for s_d in ("src", "dst"):

            def is_broadcast(c):
                return s_d == "src" and c["srctype"] == "broadcaster"

            def port_match(c):
                return con[f"{s_d}blk"] == c[f"{s_d}blk"] and con[f"{s_d}port"] == c[f"{s_d}port"]

            dupes = [
                other
                for other in config.connections[con_idx + 1 :]
                if port_match(other) and not is_broadcast(other)
            ]
            if dupes:
                error = (
                    f"The {'source' if s_d == 'src' else 'destination'} port {con[f'{s_d}blk']}:{con[f'{s_d}port']} is "
                    f"connected to multiple {'destinations' if s_d == 'src' else 'sources'} via the following connections:\n"
                    f"{con2str(con)}\n"
                )
                for dupe in dupes:
                    error += con2str(dupe) + "\n"
                config.log.error(error)


def _check_io_port_compatibility(con, log):
    """Check if source and destination IO ports are compatible."""
    if con["src_iosig"].get("type") != con["dst_iosig"].get("type"):
        log.error(
            f"IO port type mismatch: {con['srcblk']}.{con['srcport']} "
            f"(type: {con['src_iosig'].get('type')}) → "
            f"{con['dstblk']}.{con['dstport']} "
            f"(type: {con['dst_iosig'].get('type')})"
        )
    src_ports = con["src_iosig"].get("wires", [])
    dst_ports = con["dst_iosig"].get("wires", [])
    if len(src_ports) != len(dst_ports):
        log.warning(
            f"Connection {con2str(con)} has a port number mismatch. "
            f"Counting {len(src_ports)} ports on source and {len(dst_ports)} ports on destination."
        )
    for src_port, dst_port in zip(src_ports, dst_ports):
        src_width = src_port.get("width", 1)
        dst_width = dst_port.get("width", 1)
        if src_width != dst_width:
            log.warning(
                f"Connection {con2str(con)} has a wire width mismatch. "
                f"Source port {src_port['name']} has width {src_width}, destination port {dst_port['name']} has width {dst_width}."
            )


def check_and_sanitize(config):
    """Check and sanitize connections of an image builder configuration.

    This will run through all the connections on a configuration object and
    check if they're valid, but it will also modify them:

    - Add annotations to the connections about domain, type, and IO signature
    - Provide error messages if not valid
    """
    failure = ""

    def sanitize_port(con, s_d, failure):
        """Unpack port names (separate slice from port name)."""
        port_match = re.match(r"^([a-z0-9_]+)(?:\[([^]])\])?$", con[f"{s_d}port"])
        if not port_match:
            failure += f"Invalid port name: {con[f'{s_d}port']}\n"
            return con
        con[f"{s_d}port"], con[f"{s_d}slice"] = port_match.groups()
        return con

    def check_port_is_valid(blk_key, blk, port, s_d):
        """Return true if the port is valid for the block."""
        io_ports = getattr(blk, "io_ports", {})
        valid_io_port_drives = ("master", "broadcaster") if s_d == "src" else ("slave", "listener")
        return (
            ((blk_key, port, "output" if s_d == "src" else "input") in config.block_ports)
            or (io_ports.get(port, {}).get("drive") in valid_io_port_drives)
            or (port == NONE_PORT)
        )

    def get_ports(blk_key, blk, s_d):
        """Return list of all valid ports for a block."""
        data_ports = [
            p[1]
            for p in config.block_ports
            if p[0] == blk_key and p[2] == ("output" if s_d == "src" else "input")
        ]
        valid_io_port_drives = ("master", "broadcaster") if s_d == "src" else ("slave", "listener")
        io_ports = [
            k for k, v in getattr(blk, "io_ports", {}).items() if v["drive"] in valid_io_port_drives
        ]
        return ", ".join(data_ports + io_ports)

    ## Phase 1: We go through the list of connections and provide annotations
    for conn_idx, con in enumerate(config.connections):
        for s_d in ("src", "dst"):
            con = sanitize_port(con, s_d, failure)
        src_blk = config.get_module(con["srcblk"])
        if src_blk is None:
            failure += f"Source block '{con['srcblk']}' not found\n"
            continue
        dst_blk = config.get_module(con["dstblk"])
        if dst_blk is None:
            failure += f"Destination block '{con['dstblk']}' not found\n"
            continue
        if not check_port_is_valid(con["srcblk"], src_blk, con["srcport"], "src"):
            failure += f"Source port '{con['srcblk']}.{con['srcport']}' not found. Available ports on '{con['srcblk']}': {get_ports(con['srcblk'], src_blk, 'src')}\n"
            continue
        if not check_port_is_valid(con["dstblk"], dst_blk, con["dstport"], "dst"):
            failure += f"Destination port '{con['dstblk']}.{con['dstport']}' not found. Available ports on '{con['dstblk']}': {get_ports(con['dstblk'], dst_blk, 'dst')}\n"
            continue
        try:
            # If dummy connection, we just skip this.
            if con["srcport"] == NONE_PORT or con["dstport"] == NONE_PORT:
                pass
            # If this is a connection between blocks, or between stream endpoints and
            # blocks, then we can determine the connection type.
            elif (con["srcblk"], con["srcport"], "output") in config.block_ports and (
                con["dstblk"],
                con["dstport"],
                "input",
            ) in config.block_ports:
                con["srctype"] = "output"
                con["dsttype"] = "input"
            # If it's none of the above, it's either an IO port connection, or it's
            # not valid.
            elif (
                src_blk.io_ports.get(con["srcport"], {}).get("drive") == "master"
                and dst_blk.io_ports.get(con["dstport"], {}).get("drive") == "slave"
            ) or (
                src_blk.io_ports.get(con["srcport"], {}).get("drive") == "broadcaster"
                and dst_blk.io_ports.get(con["dstport"], {}).get("drive") == "listener"
            ):
                # In this case, we have an IO port. Make sure we have the IO
                # signature copied into the connection object.
                con["srctype"] = src_blk.io_ports[con["srcport"]]["drive"]
                con["dsttype"] = dst_blk.io_ports[con["dstport"]]["drive"]
                con["src_iosig"] = copy.deepcopy(src_blk.io_ports[con["srcport"]])
                con["dst_iosig"] = copy.deepcopy(dst_blk.io_ports[con["dstport"]])
                _check_io_port_compatibility(con, config.log)
            # If we have IO ports, but the previous clause didn't take, then
            # the connection endpoints are not compatible.
            elif (con["srcport"] in src_blk.io_ports) and (con["dstport"] in dst_blk.io_ports):
                con["srctype"] = src_blk.io_ports[con["srcport"]]["drive"]
                con["dsttype"] = dst_blk.io_ports[con["dstport"]]["drive"]
                failure += (
                    f"IO port {con['srcblk']}.{con['srcport']} "
                    f"({src_blk.io_ports[con['srcport']]['drive']}) "
                    f"cannot connect to {con['dstblk']}.{con['dstport']} "
                    f"({dst_blk.io_ports[con['dstport']]['drive']})\n"
                    f"Make sure io ports are connected master      (src) to slave    (dst)\n"
                    f"                              or broadcaster (src) to listener (dst)\n"
                )
            else:
                failure += "Unresolved connection: " + con2str(con) + "\n"
            config.connections[conn_idx] = con
        except AttributeError as ex:
            config.log.error(
                "Error parsing connection: %s. No io_ports attribute on source or dest:\n%s\n%s (%s)",
                con,
                src_blk,
                dst_blk,
                str(ex),
            )
            raise

    # Go through all the modules and check IO ports are connected
    for module_name, module in config.get_module_list("all").items():
        for io_port_name, io_port in module.io_ports.items():
            required = io_port.get("required")
            if required:
                is_connected = any(
                    con["srcblk"] == module_name
                    and con["srcport"] == io_port_name
                    or con["dstblk"] == module_name
                    and con["dstport"] == io_port_name
                    for con in config.connections
                )
                if not is_connected:
                    available_ports = config._get_available_ports(io_port)
                    msg = f"IO port {module_name}.{io_port_name} is not connected. "
                    if len(available_ports) == 1:
                        msg += (
                            f"Suggested connection: "
                            f"{available_ports[0][0]}.{available_ports[0][1]}\n"
                        )
                        msg += (
                            f"Add the following connection to the image core file to "
                            f"use this connection:\n"
                        )
                        if available_ports[0][2] == "src":
                            msg += (
                                f"{{ srcblk: {available_ports[0][0]}, "
                                f"srcport: {available_ports[0][1]}, dstblk: "
                                f"{module_name}, dstport: {io_port_name} }}"
                            )
                        else:
                            msg += (
                                f"{{ srcblk: {module_name}, srcport: {io_port_name}, "
                                f"dstblk: {available_ports[0][0]}, dstport: "
                                f"{available_ports[0][1]} }}"
                            )
                    elif len(available_ports) > 1:
                        msg += f"Available connections:\n"
                        for port in available_ports:
                            msg += f"    {port[0]}.{port[1]}\n"
                        msg += (
                            "Add a connection line to the image core file to enable "
                            "this connection:\n"
                        )
                        if available_ports[0][2] == "src":
                            msg += (
                                f"{{ srcblk: ..., srcport: ..., dstblk: "
                                f"{module_name}, dstport: {io_port_name} }}"
                            )
                        else:
                            msg += (
                                f"{{ srcblk: {module_name}, srcport: {io_port_name}, "
                                f"dstblk: ..., dstport: ... }}"
                            )
                    else:
                        msg += f"No available IO ports found!"
                    if required == "recommended":
                        config.log.warning(msg)
                    else:
                        failure += msg
    # Go through blocks and check all ports are connected
    for block_name, block in config.noc_blocks.items():
        for direction in ("inputs", "outputs"):
            for port_name in block["data"][direction]:
                if not any(
                    con["srcblk"] == block_name
                    and con["srcport"] == port_name
                    or con["dstblk"] == block_name
                    and con["dstport"] == port_name
                    for con in config.connections
                ):
                    config.log.warning("Block port %s.%s is not connected", block_name, port_name)
    # Drop empty connections
    config.connections = [
        c for c in config.connections if c["srcport"] != NONE_PORT and c["dstport"] != NONE_PORT
    ]
    # Now make sure there are no duplicate connections
    _check_duplicate_connections(config)

    if failure:
        config.log.error(
            "There are errors in the image core file's connection settings:\n" + failure
        )
        sys.exit(1)


def get_num_ports_from_data_port(port_info, block):
    """Resolve number of ports from data port info.

    Rules:
    - If num_ports is not in port_info, return 1
    - If it's an integer, return that.
    - If num_ports isn't an integer, it could be an expression
      using values from the parameters section (e.g.,
      NUM_PORTS*NUM_BRANCHES for a stream-splitting block).
      If the parameter doesn't resolve to an integer, treat it
      as an expression that needs to be evaluated, hopefully to
      an integer.
    """
    if "num_ports" not in port_info:
        return 1
    num_ports = port_info["num_ports"]
    if isinstance(num_ports, int):
        return num_ports

    # Create a regex to find identifiers.
    regex_ident = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")

    # Get a list of all identifiers in the num_ports
    # expression and iterate over them all
    idents = re.finditer(regex_ident, num_ports)
    for ident in idents:
        # If the identifier represents a valid parameter
        # in the block, replace the identifier text with
        # the value of the parameter. If no matching
        # parameter is found, just leave the text in
        # place. That may result in an exception being
        # thrown from eval(), but we'll catch it and
        # report an error a bit later on.
        if ident[0] in block["parameters"]:
            val = str(block["parameters"][ident[0]])
            num_ports = re.sub(ident[0], val, num_ports)

    # Now, with identifiers resolved to parameter values,
    # attempt to evaluate the expression. If eval() fails,
    # we'll catch the exception, num_ports will remain non-
    # integral, and the if statement after the exception
    # is caught will inform the user.
    try:
        return eval(num_ports, {}, {})
    except:  # noqa: E722
        return None
