#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
MPM Module
"""

from . import libpyusrp_periphs as lib
from .discovery import spawn_discovery_process
from .rpc_server import spawn_rpc_process
from . import mpmtypes
from . import periph_manager
from . import dboard_manager
from . import xports
from . import cores
from . import chips
from . import gpsd_iface
from .mpmlog import get_main_logger

__version__ = periph_manager.__version__
__githash__ = periph_manager.__githash__
