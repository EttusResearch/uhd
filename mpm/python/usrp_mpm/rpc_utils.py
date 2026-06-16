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
    "Decorator for functions that require no token check"
    func._notok = True
    return func


def no_rpc(func):
    "Decorator for functions that should not be exposed via RPC"
    func._norpc = True
    return func
