"""
Copyright 2019 Ettus Research, A National Instrument Brand

SPDX-License-Identifier: GPL-3.0-or-later

This module contains utility functions and helpers used by the image builder.
"""

import os
import re
import ast
import logging
import mako.template
from mako.exceptions import MakoException

def merge_dicts(origd, newd):
    """
    Merge two dictionaries recursively.

    Originally from (with some modifications):
    https://stackoverflow.com/questions/71396284/python-how-to-recursively-merge-2-dictionaries
    """
    for key, val in newd.items():
        if key not in origd:
            origd[key] = val
            continue
        if isinstance(val, dict):
            if not isinstance(origd[key], dict):
                origd[key] = {}
            merge_dicts(origd[key], val)
        elif isinstance(val, (tuple, list)) and isinstance(origd[key], (tuple, list)):
            origd[key] = list(origd[key]) + list(val)
        else:
            origd[key] = val
    return origd

def generate_edge_table(config):
    """
    Generate the edge table based on the connections in 'config'.

    The edge table is a list of 4-tuples of the following format:

    (src_blk_index, src_port, dst_blk_index, dst_port)

    The block indices match the control crossbar indices. This table can be
    used to directly initialize the edge list in the FPGA.
    """
    block_con = [con for con in config.connections if con['srctype'] == 'output']
    logging.debug("Generating edge table...")
    edge_tbl = []
    for connection in block_con:
        if connection["srcblk"] in config.stream_endpoints:
            sep = config.stream_endpoints[connection["srcblk"]]
            index_match = re.match(r"out(\d)", connection["srcport"])
            if not index_match:
                logging.error("Port %s is invalid on endpoint %s",
                              connection["srcport"], connection["srcblk"])
            port_index = int(index_match.group(1))
            # Verify index < num_data_o
            if port_index >= sep["num_data_o"]:
                logging.error("Port %s exceeds num_data_o for endpoint %s",
                              connection["srcport"], connection["srcblk"])
            src = (sep["index"], port_index)
        else:
            block = config.noc_blocks[connection['srcblk']]
            src = (block['index'],
                   block['data']['outputs'][connection['srcport']]['index'])
        if connection["dstblk"] in config.stream_endpoints:
            sep = config.stream_endpoints[connection["dstblk"]]
            index_match = re.match(r"in(\d)", connection["dstport"])
            if not index_match:
                logging.error("Port %s is invalid on endpoint %s",
                              connection["dstport"], connection["dstblk"])
            # Verify index < num_data_i
            port_index = int(index_match.group(1))
            if port_index >= sep["num_data_i"]:
                logging.error("Port %s exceeds num_data_i for endpoint %s",
                              connection["dstport"], connection["dstblk"])
            dst = (sep["index"], port_index)
        else:
            block = config.noc_blocks[connection['dstblk']]
            dst = (block['index'],
                   block['data']['inputs'][connection['dstport']]['index'])
        edge_tbl.append((*src, *dst))
        logging.debug("  %s-%s (%d,%d) => %s-%s (%d,%d)",
                      connection["srcblk"], connection["srcport"],
                      src[0], src[1],
                      connection["dstblk"], connection["dstport"],
                      dst[0], dst[1])
    return edge_tbl

def resolve(var_val, **kwargs):
    """
    Resolve a string that contains variable references.
    """
    if not isinstance(var_val, str):
        return var_val
    if 'env' not in kwargs:
        kwargs['env'] = dict(os.environ)
    try:
        tpl = mako.template.Template(var_val, strict_undefined=True)
        res = tpl.render(**kwargs)
    except MakoException as ex:
        raise SyntaxError(f"Unable to parse:\n{var_val}") from ex
    try:
        return ast.literal_eval(res)
    except (SyntaxError, ValueError):
        return res


def find_include_file(filename, include_paths):
    """
    Return an absolute path to a file based on a relative path, and a list
    of include paths.

    When calling this, 'filename' contains a relative path to a file (e.g.,
    "rfnoc_block_foo/Makefile.srcs"). 'include_paths' is a list of paths in
    which this file is searched. Example:

    >>> find_include_file("rfnoc_block_foo/Makefile.srcs",
                          ["/usr/share/foo", "/usr/share/bar"])
    '/usr/share/foo/rfnoc_block_foo/Makefile.srcs'

    To determine the absolute path, it will search for 'filename' in all include
    paths in the given order.

    There are two exceptions, where the filename is returned verbatim:
    1. If the filename contains a make variable, then we just keep that and let
       make find the file, e.g. '$(LIB_DIR)/rfnoc/blocks/rfnoc_block_ddc/Makefile.srcs'
    2. If the filename is an absolute path, it is returned verbatim, but only
       if the file exists.

    If the file cannot be found, a FileNotFoundError is thrown.
    """
    # check for Makefile vars
    if re.match(r'\$\(.*\)', filename):
        return filename

    # if the file is already a fully valid path return the real path
    if os.path.isabs(filename):
        if os.path.isfile(filename):
            return os.path.realpath(filename)
        raise FileNotFoundError(f"File {filename} not found!")

    # otherwise search any combination of include_paths and filename
    for path in include_paths:
        candidate = os.path.join(path, filename)
        if os.path.isfile(candidate):
            return os.path.realpath(candidate)

    raise FileNotFoundError(
        f"File {filename} not found in {include_paths}")
