"""Image builder connections module.

Copyright 2019 Ettus Research, A National Instrument Brand

SPDX-License-Identifier: GPL-3.0-or-later

Functions, helpers, and other code to handle connections in the image builder.
"""

import copy
import re
import sys

from .common import DEVICE_NAME, NONE_PORT


def con2str(con):
    """Pretty-print a single connection."""
    return f"{con['srcblk']}:{con['srcport']} → {con['dstblk']}:{con['dstport']}"


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

    ## Phase 1: We go through the list of connections and provide annotations
    for conn_idx, con in enumerate(config.connections):
        for s_d in ("src", "dst"):
            con = sanitize_port(con, s_d, failure)
        src_blk = config.get_module(con["srcblk"])
        dst_blk = config.get_module(con["dstblk"])
        if (con["srcblk"], con["srcport"], "output") in config.block_ports and (
            con["dstblk"],
            con["dstport"],
            "input",
        ) in config.block_ports:
            con["srctype"] = "output"
            con["dsttype"] = "input"
        elif (
            src_blk.io_ports.get(con["srcport"], {}).get("drive") == "master"
            and dst_blk.io_ports.get(con["dstport"], {}).get("drive") == "slave"
        ) or (
            src_blk.io_ports.get(con["srcport"], {}).get("drive") == "broadcaster"
            and dst_blk.io_ports.get(con["dstport"], {}).get("drive") == "listener"
        ):
            # TODO: Check IO port compatibility (e.g. wire widths)
            con["srctype"] = src_blk.io_ports[con["srcport"]]["drive"]
            con["dsttype"] = dst_blk.io_ports[con["dstport"]]["drive"]
            con["src_iosig"] = copy.deepcopy(src_blk.io_ports[con["srcport"]])
            con["dst_iosig"] = copy.deepcopy(dst_blk.io_ports[con["dstport"]])
            if con["src_iosig"].get("type") != con["dst_iosig"].get("type"):
                failure += (
                    f"IO port type mismatch: {con['srcblk']}.{con['srcport']} "
                    f"(type: {con['src_iosig'].get('type')}) → "
                    f"{con['dstblk']}.{con['dstport']} "
                    f"(type: {con['dst_iosig'].get('type')})\n"
                )
        elif con["srcport"] == NONE_PORT or con["dstport"] == NONE_PORT:
            pass
        else:
            failure += "Unresolved connection: " + con2str(con) + "\n"
        config.connections[conn_idx] = con

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

    if failure:
        config.log.error(
            "There are errors in the image core file's connection settings:\n" + failure
        )
        config.log.info("    Make sure block ports are connected output (src) to input (dst)")
        config.log.info("    Available block ports for connections:")
        for block in config.block_ports:
            config.log.info("        %s", (block,))
        config.log.info("    Make sure io ports are connected master      (src) to slave    (dst)")
        config.log.info("                                  or broadcaster (src) to listener (dst)")
        config.log.info("    Available IO ports for connections:")
        for mod_list in (config.noc_blocks, config.modules):
            for module_name, module in mod_list.items():
                for io_name, io_port in module.io_ports.items():
                    config.log.info("        %s.%s (%s)", module_name, io_name, io_port["type"])
        for io_name, io_port in config.device.io_ports.items():
            config.log.info("        %s.%s (%s)", DEVICE_NAME, io_name, io_port["type"])
        sys.exit(1)
