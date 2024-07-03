"""
Copyright 2019 Ettus Research, A National Instrument Brand

SPDX-License-Identifier: GPL-3.0-or-later

This module contains files for handling GRC based image cores.
"""

import re
import os
import logging
from collections import deque

from . import yaml_utils

def split(iterable, function):
    """
    Split an iterable by condition. Matching items are returned in the first
    deque of the returned tuple unmatched in the second
    :param iterable: an iterable to split
    :param function: an expression that returns True/False for iterable values
    :return: 2-tuple with deque for matching/non-matching items
    """
    dq_true = deque()
    dq_false = deque()

    deque(((dq_true if function(item) else dq_false).append(item)
           for item in iterable), maxlen=0)

    return dq_true, dq_false

def read_grc_block_configs(path):
    """
    Reads RFNoC config block used by Gnuradio Companion
    :param path: location of grc block configuration files
    :return: dictionary of block (id mapped to description)
    """
    result = {}

    for root, _dirs, names in os.walk(path):
        for name in names:
            if re.match(r".*\.block\.yml", name):
                with open(os.path.join(root, name), encoding='utf-8') as stream:
                    config = yaml_utils.ordered_load(stream)
                    result[config["id"]] = config
    return result


def convert_to_image_config(grc, grc_config_path):
    """
    Converts Gnuradio Companion grc into image configuration.
    :param grc:
    :return: image configuration as it would be returned by image_config(args)
    """
    grc_blocks = read_grc_block_configs(grc_config_path)
    #filter all blocks that have no block representation
    seps = {item["name"]: item for item in grc["blocks"] if item["parameters"]["type"] == 'sep'}
    blocks = {item["name"]: item for item in grc["blocks"] if item["parameters"]["type"] == 'block'}
    device = [item for item in grc["blocks"] if item["parameters"]["type"] == 'device']
    if len(device) == 1:
        device = device[0]
    else:
        logging.error("More than one or no device found in grc file")
        return None

    result = {
        "schema": "rfnoc_imagebuilder",
        "copyright": "Ettus Research, A National Instruments Brand",
        "license": "SPDX-License-Identifier: LGPL-3.0-or-later",
        "version": "1.0",
        "rfnoc_version": "1.0"}
    # for param in [item for item in grc["blocks"] if item["id"] == "parameter"]:
    #     result[param["name"]] = {
    #         "str": lambda value: str,
    #         "": lambda value: str,
    #         "complex": str,
    #         "intx": int,
    #         "long": int,
    #     }[param["parameters"]["type"]](param["parameters"]["value"])

    result["stream_endpoints"] = {}
    for sep in seps.values():
        result["stream_endpoints"][sep["name"]] = {"ctrl": bool(sep["parameters"]["ctrl"]),
                                                   "data": bool(sep["parameters"]["data"])}
        if "buff_size" in sep["parameters"]:
            result["stream_endpoints"][sep["name"]]["buff_size"] = \
                int(sep["parameters"]["buff_size"])
        if "buff_size_bytes" in sep["parameters"]:
            result["stream_endpoints"][sep["name"]]["buff_size_bytes"] = \
                int(sep["parameters"]["buff_size_bytes"])

    result["noc_blocks"] = {}
    for block in blocks.values():
        result["noc_blocks"][block["name"]] = {
            "block_desc": block["parameters"]["desc"]
        }
        if "nports" in block["parameters"]:
            result["noc_blocks"][block["name"]]["parameters"] = {
                "NUM_PORTS": block["parameters"]["nports"]
            }

    device_clocks = {
        port["id"]: port for port in grc_blocks[device['id']]["outputs"]
        if port["dtype"] == "message"
    }

    for connection in grc["connections"]:
        if connection[0] == device["name"]:
            connection[0] = "_device_"
        if connection[2] == device["name"]:
            connection[2] = "_device_"
    device["name"] = "_device_"

    (clk_connections, connections) = split(
        grc["connections"], lambda con:
        con[0] == device["name"] and con[1] in device_clocks)

    result["connections"] = []
    for connection in connections:
        result["connections"].append(
            {"srcblk":  connection[0],
             "srcport": connection[1],
             "dstblk":  connection[2],
             "dstport": connection[3]}
        )

    result["clk_domains"] = []
    for connection in clk_connections:
        result["clk_domains"].append(
            {"srcblk":  connection[0],
             "srcport": connection[1],
             "dstblk":  connection[2],
             "dstport": connection[3]}
        )

    return result

