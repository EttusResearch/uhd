"""Image builder: GRC integration.

Copyright 2019 Ettus Research, A National Instrument Brand

SPDX-License-Identifier: GPL-3.0-or-later

This module contains files for handling GRC based image cores.
"""

import logging
import sys

from . import yaml_utils
from .connections import get_num_ports_from_data_port
from .utils import resolve


def sanitize_connection(connection, known_modules, device, grc_block_dict):
    """Sanitize a connection tuple.

    Analyze the connection info that GRC provides and apply some rules to make
    it compatible with UHD's image builder.

    The following rules are checked:
    1. If a block is a terminator block, replace the block name with "_device_"
       and the port name with "_none_".
    2. If one of the ports goes to the BSP (device block), replace the block name with "_device_".
    3. In GRC, all port names must be unique. However, clock ports, IO ports and
       data ports are all the same thing. We therefore strip trailing "_clk" from
       port names.
    4. Fix multiplicity name modifier. In GNU Radio, if a port has a 'multiplicity' parameter,
       GRC will append numbers to the port name (e.g,. in0, in1, in2, ...), but
       only if 'multiplicity' is greater than 1. In UHD, if there is a num_ports
       parameter for a data port, then the number is always appended, even if
       there is only one port.
    5. If the block used an alias, then we need to also update the connection
       tuple.
    """
    con_src = connection[0:2]
    con_dst = connection[2:4]
    hints = {}

    def sanitize_clkname(clkname, hint):
        if clkname.endswith("_clk"):
            hint.update({"stripped_clk": True})
            return clkname[:-4]
        return clkname

    def sanitize_dataport_multiplicity(block_name, port_name, in_out):
        """Sanitize multiplicity name modifier (see docstring up top).

        This is not an optimal implementation. TODO: Create better tooling around
        programmatically "understanding" the block's ports and their multiplicity.
        Such code could be reused in builder_config.py, where we parse the
        num_ports parameter of a block's data port.
        """
        # Check if this is a regular block
        if (
            block_name not in grc_block_dict
            or grc_block_dict[block_name].get("parameters", {}).get("type") != "block"
        ):
            return [block_name, port_name]
        block_info = known_modules["noc_blocks"].get(
            grc_block_dict[block_name]["parameters"]["desc"]
        )
        # Check if this block has a data port with a variable num_ports parameter
        for k, v in block_info.get("data", {}).get(in_out, {}).items():
            if (
                port_name.startswith(k)
                and (v.get("num_ports") in block_info["parameters"])
                and port_name.endswith("_")
            ):
                return [block_name, port_name + "0"]
        return [block_name, port_name]

    # Apply rules
    for con in (con_src, con_dst):
        # Rule 1
        if grc_block_dict[con[0]]["parameters"].get("type") == "term":
            con[0] = "_device_"
            con[1] = "_none_"
        # Rule 2
        if con[0] == device["name"]:
            con[0] = "_device_"
        # Rule 3
        con[1] = sanitize_clkname(con[1], hints)
    # Rule 4
    con_src = sanitize_dataport_multiplicity(*con_src, "outputs")
    con_dst = sanitize_dataport_multiplicity(*con_dst, "inputs")
    # Rule 5
    for con in (con_src, con_dst):
        if con[0] in grc_block_dict and grc_block_dict[con[0]].get("parameters", {}).get("alias"):
            con[0] = grc_block_dict[con[0]]["parameters"]["alias"]
    return [con_src[0], con_src[1], con_dst[0], con_dst[1], hints]


def handle_block_quirks(block_dict, block_type, block):
    """Handle quirks of certain blocks.

    Sadly, the mapping from GRC to image configuration is not always 1:1. This
    means we need to hardcode special rules for certain blocks. This function
    applies these rules.
    """
    if block_type == "noc_blocks" and block_dict["block_desc"] == "radio.yml":
        if block["parameters"].get("ctrl_clock"):
            block_dict["ctrl_clock"] = block["parameters"]["ctrl_clock"]
        if block["parameters"].get("timebase_clock"):
            block_dict["timebase_clock"] = block["parameters"]["timebase_clock"]
    return block_dict


def load_variables(grc):
    """Load variables from GRC file.

    This will load all variables from the GRC file and return them as a
    dictionary. Only works for variables that can directly be evaluated.
    """
    var_blocks = [item for item in grc["blocks"] if item["id"] == "variable"]
    variables = {}
    for vb in var_blocks:
        try:
            variables[vb["name"]] = eval(vb["parameters"]["value"], variables)
        except (NameError, SyntaxError) as ex:
            logging.warning(
                "Could not evaluate variable %s == %s: %s",
                vb["name"],
                vb["parameters"]["value"],
                ex,
            )
    return variables


def convert_to_image_config(grc, config_path, known_modules):
    """Convert GRC file into image configuration.

    :param grc:
    :return: image configuration as it would be returned by image_config(args)
    """
    # This is a list of parameters that are generally part of GNU Radio blocks,
    # but can be ignored when creating the image configuration.
    param_blacklist = ["desc", "type", "affinity", "alias", "comment", "maxoutbuf", "minoutbuf"]
    grc_block_dict = {item["name"]: item for item in grc["blocks"]}
    variables = load_variables(grc)

    def param_resolve(value):
        """Resolve a parameter value.

        This will resolve variables defined in the GRC file, which means you
        can use variables in the GRC file to define parameters of blocks.
        """
        if value in variables:
            logging.debug("Resolving variable %s to %s", value, variables[value])
            return variables[value]
        return resolve(value)

    ###########################################################################
    # Find device configuration
    device = [item for item in grc["blocks"] if item["parameters"].get("type") == "device"]
    if len(device) == 1:
        device = device[0]
    elif len(device) == 0:
        logging.error("No device found in grc file")
        return None
    else:
        logging.error("More than one device found in grc file")
        return None
    device_config = yaml_utils.device_config(config_path, device["parameters"]["device"])
    # Generate base image configuration. This dictionary shall look like the
    # dictionary that would be generated the equivalent image core YAML file
    # to this GRC file.
    result = {
        "schema": "rfnoc_imagebuilder_args",
        "copyright": grc["options"]["parameters"]["copyright"],
        "license": "SPDX-License-Identifier: LGPL-3.0-or-later",
        "version": "1.0",
        "chdr_width": int(device["parameters"]["chdr_width"]),
        "device": device["parameters"]["device"],
        "image_core_name": grc["options"]["parameters"]["id"],
        "default_target": device["parameters"].get("default_target", None),
        "parameters": {
            k: param_resolve(v)
            for k, v in device["parameters"].items()
            if k not in param_blacklist and k in device_config.get("parameters", {})
        },
        "stream_endpoints": {},
        "transport_adapters": {},
        "modules": {},
        "noc_blocks": {},
        "connections": [],
        "clk_domains": [],
    }
    # This dictionary lists all clock ports as a list of clock port names for
    # each block, e.g., {"radio0": ["radio",], "ddc0": ["ce",]}.
    clocks = {"_device_": [clk["name"] for clk in device_config["clocks"]]}
    # This dictionary lists all IO and data ports as a list of port names for
    # each block, e.g., {"radio0": ["ctrlport", "in_0", ...], "ddc0": ["in_0", ...]}.
    ports = {"_device_": list(device_config["io_ports"].keys())}
    ###########################################################################
    # Stream Endpoints
    seps = {item["name"]: item for item in grc["blocks"] if item["parameters"].get("type") == "sep"}
    for sep_name, sep in seps.items():
        if sep["parameters"].get("alias"):
            sep_name = sep["parameters"]["alias"]
        clocks[sep_name] = []
        try:
            result["stream_endpoints"][sep_name] = {
                "ctrl": (
                    "auto"
                    if sep["parameters"]["ctrl"].lower() == "auto"
                    else bool(sep["parameters"]["ctrl"].lower() in ("true", "1", "yes"))
                ),
                "data": bool(sep["parameters"]["data"].lower() in ("true", "1", "yes")),
            }
            # We support both buff_size and buff_size_bytes
            for bufsiz_type in ("buff_size", "buff_size_bytes"):
                if bufsiz_type in sep["parameters"]:
                    result["stream_endpoints"][sep_name][bufsiz_type] = int(
                        sep["parameters"][bufsiz_type]
                    )
        except KeyError:
            logging.error("Error parsing stream endpoint %s", sep)
            sys.exit(1)
    # Resolve 'auto' ctrl for stream endpoints. The rules are simple: If there
    # is any SEP with ctrl set to True, then all SEPs with ctrl set to 'auto'
    # will have no control. If there is no SEP with ctrl set to True, then the
    # first SEP with ctrl set to 'auto' will have control.
    if all(sep["ctrl"] is not True for sep in result["stream_endpoints"].values()):
        for sep in result["stream_endpoints"].values():
            if sep["ctrl"] == "auto":
                sep["ctrl"] = True
                break
    for sep in result["stream_endpoints"].values():
        if sep["ctrl"] == "auto":
            sep["ctrl"] = False
    assert all(type(sep["ctrl"]) is bool for sep in result["stream_endpoints"].values())
    assert all(type(sep["data"]) is bool for sep in result["stream_endpoints"].values())
    ###########################################################################
    # Blocks (NoC Blocks, Transport Adapters, Modules)
    # design_blocks is a dictionary of all blocks, transport adapters, and
    # modules in this current GRC design.
    design_blocks = {}
    for grc_blocktype, blocktype in (
        ("block", "noc_blocks"),
        ("transport_adapter", "transport_adapters"),
        ("module", "modules"),
    ):
        design_blocks[blocktype] = {
            item["name"]: item
            for item in grc["blocks"]
            if item["parameters"].get("type") == grc_blocktype
        }
        for block_name, block in design_blocks[blocktype].items():
            if "desc" not in block["parameters"]:
                raise ValueError("Block %s has no 'desc' parameter." % block_name)
            if block["parameters"]["desc"] not in known_modules[blocktype]:
                logging.error("Block %s not found.", block["parameters"]["desc"])
                sys.exit(1)
            block_info = known_modules[blocktype][block["parameters"]["desc"]]
            clocks[block_name] = [clk["name"] for clk in block_info.get("clocks", [])]
            ports[block_name] = list(block_info.get("io_ports", {}).keys())
    for block_type in ("noc_blocks", "transport_adapters", "modules"):
        for block in design_blocks.get(block_type, {}).values():
            block_dict = {
                "block_desc": block["parameters"]["desc"],
                "parameters": {},
            }
            if block_dict["block_desc"] not in known_modules[block_type]:
                logging.error("Block %s not found.", block_dict["block_desc"])
                sys.exit(1)
            block_param_info = known_modules[block_type][block_dict["block_desc"]].get(
                "parameters", {}
            )
            block_dict["parameters"] = {
                param_name: param_resolve(param_value)
                for param_name, param_value in block.get("parameters", {}).items()
                if param_name not in param_blacklist
                and not param_name.startswith("_")
                and param_name in block_param_info
            }
            # Update list of ports with data ports. We can do this now that the
            # parameters have been resolved. For example, if we have a radio
            # block called 'radio0', then ports["radio0"] will add all the
            # data ports of that block ("in_0", "out_0", ...).
            if block_type == "noc_blocks":  # Only NoC blocks have data ports
                for direction in ("inputs", "outputs"):
                    data_ports = (
                        known_modules[block_type][block_dict["block_desc"]]
                        .get("data", {})
                        .get(direction, {})
                        .items()
                    )
                    for port_name, port_info in data_ports:
                        num_ports = get_num_ports_from_data_port(port_info, block_dict)
                        if num_ports is None:
                            logging.error(
                                "Cannot identify number of ports for data port %s on block %s.",
                                port_name,
                                block["name"],
                            )
                            sys.exit(1)
                        # If we have a variable number of ports, then we unroll
                        # the port names (e.g., in_0, in_1, in_2, ...).
                        if "num_ports" in port_info:
                            ports[block["name"]].extend(
                                [f"{port_name}_{i}" for i in range(num_ports)]
                            )
                        # If not, then we just add the port name.
                        else:
                            ports[block["name"]].append(port_name)
            # 'priority' is a special parameter that is not part of the block's
            # parameter list, but rather directly a key of the block.
            if "priority" in block["parameters"]:
                block_dict["priority"] = param_resolve(block["parameters"]["priority"])
            block_dict = handle_block_quirks(block_dict, block_type, block)
            block_name = block["name"]
            if grc_block_dict[block_name]["parameters"].get("alias"):
                alias = grc_block_dict[block_name]["parameters"]["alias"]
                clocks[alias] = clocks[block_name]  # Update from old block name to alias
                ports[alias] = ports[block_name]  # Update from old block name to alias
                block_name = alias
            if block_name in result[block_type]:
                logging.error("Block %s already exists.", block_name)
                sys.exit(1)
            result[block_type][block_name] = block_dict
    ###########################################################################
    # Connections
    # First, sanitize connections. The format from GRC is not directly compatible
    # with the image builder.
    grc["connections"] = [
        sanitize_connection(connection, known_modules, device, grc_block_dict)
        for connection in grc["connections"]
    ]
    device["name"] = "_device_"

    # Now, add connections to the image configuration. We need to determine if
    # a connection is a clock domain connection or a regular connection.
    def is_clk_con(con):
        """Check if a connection is a clock domain connection.

        The following rules are checked:
        - If the names of the ports are in the list of known clocks, but they
          are not in the list of IO ports of the block, then it is a clock
          domain connection.
        - If the names of the ports are both known clock names, but also
          known IO ports of the block, then we check the connection hints.
          If we previously stripped _clk from the port names, then it is a
          clock domain connection.
        - Otherwise, it's not.

        Reminders:
        - The connection tuple is (srcblk, srcport, dstblk, dstport, hints).
        - The clocks dictionary is a dictionary of block names to a list of
          clock port names.
        - Same for the ports dictionary, but for IO and data ports.
        """
        srcblk, srcport, dstblk, dstport, hints = con
        return (
            # Case 1: All ports are clock ports, and are not also regular ports
            (srcport in clocks[srcblk] and dstport in clocks[dstblk])
            and not (srcport in ports[srcblk] and dstport in ports[dstblk])
        ) or (
            # Case 2: All ports are both clock ports, and are also regular ports,
            # but we stripped _clk from the port names
            (srcport in clocks[srcblk] and dstport in clocks[dstblk])
            and (srcport in ports[srcblk] and dstport in ports[dstblk])
            and hints.get("stripped_clk")
        )

    # The goal of this loop is to sort all the connections that are in
    # grc["connections"] into either result["connections"] or
    # result["clk_domains"].
    for con in grc["connections"]:
        con_type = "clk_domains" if is_clk_con(con) else "connections"
        result[con_type].append(
            {
                "srcblk": con[0],
                "srcport": con[1],
                "dstblk": con[2],
                "dstport": con[3],
            }
        )

    ###########################################################################
    # Housekeeping / sanity checks / cleanup
    empty_keys = [k for k, v in result.items() if not v]
    for k in empty_keys:
        del result[k]
    return result
