#
# Copyright 2022 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Types and values relevant for clocking on the X4xx USRPs.
"""

from enum import Enum

class Spll1Vco(Enum):
    """
    VCO options for the sample PLL's (LMK04832) PLL1 VCOs.
    """
    VCO100MHz = 0
    VCO122_88MHz = 1

class RpllRefSel(Enum):
    """
    Reference input options for RPLL

    Primary reference (PRIREF) is connected to the eCPRI PLL on the clocking
    aux card.

    Secondary reference (SECREF) is connected to a reliable, fixed 100 MHz
    crystal.
    """
    PRIMARY = 1
    SECONDARY = 2


class RpllBrcSrcSel(Enum):
    """
    Identify source of the BRC (bypass PLL, or use PLL)

    BYPASS is only a valid option if the primary reference is already the
    desired BRC rate.
    """
    BYPASS = 'bypass'
    PLL = 'PLL'


class BrcSource(Enum):
    """
    Provides valid options for the BRC source. It can either come from the
    clocking AUX board, or from the reference PLL. The hardware has no other
    options available.
    """
    CLK_AUX = 'clk_aux'
    RPLL = 'rpll'
