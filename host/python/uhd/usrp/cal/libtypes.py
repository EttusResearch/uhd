#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Import cal types into Python
"""

from ... import libpyuhd as lib

# Disable PyLint because we want to make it look like the following classes are
# defined in Python, but they're just renames of lib types. They therefore
# follow name conventions for Python classes, not for global constants.
# pylint: disable=invalid-name
# database is a class, but we treat it like a namespace, i.e., a submodule
database = lib.cal.database
Source = lib.cal.source
# pylint: enable=invalid-name
