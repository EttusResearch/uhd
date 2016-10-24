#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright 2016 Ettus Research LLC.
#
# This is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this software; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
#

from gnuradio import gr
from gnuradio import blocks
import numpy as np


class phase_calc_ccf(gr.hier_block2):
    """
    docstring for block phase_calc_ccf
    """

    def __init__(self):
        gr.hier_block2.__init__(
            self,
            "phase_calc_ccf",
            gr.io_signature(2, 2, gr.sizeof_gr_complex),  # Input signature
            gr.io_signature(1, 1, gr.sizeof_float))  # Output signature
        self.block = dict()
        self.block['mult_conj'] = blocks.multiply_conjugate_cc()
        self.block['arg'] = blocks.complex_to_arg()
        self.block['mult_const'] = blocks.multiply_const_ff(180.0 / np.pi)

        self.connect((self, 0), (self.block['mult_conj'], 0))
        self.connect((self, 1), (self.block['mult_conj'], 1))
        self.connect((self.block['mult_conj'], 0), (self.block['arg'], 0))
        self.connect((self.block['arg'], 0), (self.block['mult_const'], 0))
        self.connect((self.block['mult_const'], 0), (self, 0))
