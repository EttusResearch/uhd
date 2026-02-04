#
# Copyright 2017-2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
dboards module __init__.py
"""
from usrp_mpm import __simulated__

from .base import DboardManagerBase

if not __simulated__:
    from .ad936x_db import AD936xDboard
    from .dboard_iface import DboardIface
    from .e31x_db import E31x_db
    from .empty_slot import EmptySlot
    from .fbx import FBX
    from .magnesium import Magnesium
    from .neon import Neon
    from .rhodium import Rhodium
    from .test import test
    from .unknown import unknown
    from .x4xx_db_iface import X4xxDboardIface
    from .x4xx_debug_db import X4xxDebugDboard
    from .x4xx_if_test_cca import X4xxIfTestCCA
    from .zbx import ZBX
