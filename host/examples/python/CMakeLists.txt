#
# Copyright 2017-2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

set(python_examples
    replay_capture.py
    rx_to_file.py
    tx_waveforms.py
    curses_fft.py
    benchmark_rate.py
    usrp_power_meter.py
    remote_rx.py
)

UHD_INSTALL(PROGRAMS ${python_examples}
    DESTINATION ${PKG_LIB_DIR}/examples/python
    COMPONENT examples)
