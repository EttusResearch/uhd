"""Copyright 2026 Ettus Research, a National Instruments Brand.

SPDX-License-Identifier: GPL-3.0-or-later

Access to GPIOs via libgpiod.
"""

"""Wrapper for loading GPIO sys utilities depending on libgpiod version."""
import gpiod

# Versions 1.5.4 and earlier have no __version__ attribute
if hasattr(gpiod, "__version__"):
    version = gpiod.__version__
else:
    version = "1.x"

if version.startswith("1."):
    # Version 1.x
    from usrp_mpm.sys_utils.gpio_v1_x import *
else:
    # Version 2.x
    from usrp_mpm.sys_utils.gpio_v2_x import *
