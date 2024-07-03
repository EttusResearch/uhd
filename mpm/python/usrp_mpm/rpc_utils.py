#!/usr/bin/env python3
#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Implements decorators and utility functions to be used with the RPC server
"""

def no_claim(func):
    " Decorator for functions that require no token check "
    func._notok = True
    return func

def no_rpc(func):
    " Decorator for functions that should not be exposed via RPC "
    func._norpc = True
    return func

def get_map_for_rpc(map, log):
    """
    ensure the map contains only string values otherwise it cannot be
    casted to std::map<string, string> in C++
    # TODO reconsider the workaround once we transition away from mprpc
    """
    for key, value in map.items():
        if value is None:
            log.warning('casting parameter "{}" from None to "n/a"'.format(key))
            map[key] = "n/a"
    return map
