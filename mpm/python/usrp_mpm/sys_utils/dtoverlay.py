#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Manipulation of device tree overlays (Linux kernel)
"""

import os
from usrp_mpm.mpmlog import get_logger

SYSFS_OVERLAY_BASE_DIR = '/sys/kernel/config/device-tree/overlays'
OVERLAY_DEFAULT_PATH = '/lib/firmware'

def get_overlay_attrs(overlay_name):
    """
    List those attributes that are connected to an overlay entry in a sysfs
    node.
    """
    overlay_path = os.path.join(SYSFS_OVERLAY_BASE_DIR, overlay_name)
    attrs = {}
    for attr_name in os.listdir(overlay_path):
        try:
            attr_val = open(
                os.path.join(overlay_path, attr_name), 'r'
            ).read().strip()
        except OSError:
            pass
        if len(attr_val):
            attrs[attr_name] = attr_val
    return attrs

def is_applied(overlay_name):
    """
    Returns True if the overlay is already applied, False if not.
    """
    try:
        return open(
            os.path.join(SYSFS_OVERLAY_BASE_DIR, overlay_name, 'status')
        ).read().strip() == 'applied'
    except IOError:
        return False

def list_overlays(applied_only=False):
    """
    List all registered kernel modules. Returns a dict of dicts:
    {
        '<overlay_name>': {
            # attributes
        },
    }

    All the attributes come from sysfs. See get_overlay_attrs().

    applied_only -- Only return those overlays that are already applied.
    """
    return {
        overlay_name: get_overlay_attrs(overlay_name)
        for overlay_name in os.listdir(SYSFS_OVERLAY_BASE_DIR)
        if not applied_only \
            or get_overlay_attrs(overlay_name).get('status') == 'applied'
    }

def list_available_overlays(path=None):
    """
    List available overlay files (dtbo)
    """
    path = path or OVERLAY_DEFAULT_PATH
    return [x.strip()[:-5] for x in os.listdir(path) if x.endswith('.dtbo')]

def apply_overlay(overlay_name):
    """
    Applies the given overlay. Does not check if the overlay is loaded.
    """
    get_logger("DTO").trace("Applying overlay `{}'...".format(overlay_name))
    overlay_path = os.path.join(SYSFS_OVERLAY_BASE_DIR, overlay_name)
    if not os.path.exists(overlay_path):
        os.mkdir(overlay_path)
    open(
        os.path.join(SYSFS_OVERLAY_BASE_DIR, overlay_name, 'path'), 'w'
    ).write("{}.dtbo".format(overlay_name))

def apply_overlay_safe(overlay_name):
    """
    Only apply an overlay if it's not yet applied.

    Finally, checks that the overlay was applied and throws an exception if not.
    """
    if is_applied(overlay_name):
        get_logger("DTO").debug(
            "Overlay `{}' was already applied, not applying again.".format(
                overlay_name
            )
        )
    else:
        apply_overlay(overlay_name)
    if not is_applied(overlay_name):
        raise RuntimeError("Failed to apply overlay `{}'".format(overlay_name))

def rm_overlay(overlay_name):
    """
    Removes the given overlay. Does not check if the overlay is loaded.
    """
    get_logger("DTO").trace("Removing overlay `{}'...".format(overlay_name))
    os.rmdir(os.path.join(SYSFS_OVERLAY_BASE_DIR, overlay_name))

def rm_overlay_safe(overlay_name):
    """
    Only remove an overlay if it's already applied.
    """
    if overlay_name in list(list_overlays(applied_only=True).keys()):
        rm_overlay(overlay_name)
    else:
        get_logger("DTO").debug(
            "Overlay `{}' was not loaded, not removing.".format(overlay_name)
        )

