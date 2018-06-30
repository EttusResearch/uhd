#
# Copyright 2017-2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

SET(python_examples
    rx_to_file.py
    tx_waveforms.py
    curses_fft.py
    benchmark_rate.py
)

UHD_INSTALL(PROGRAMS ${python_examples} DESTINATION ${PKG_LIB_DIR}/examples/python COMPONENT examples)
