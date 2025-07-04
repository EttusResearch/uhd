#
# Copyright 2017-2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

set(python_examples
    rfnoc_rx_replay_samples_to_file.py
    rx_to_file.py
    tx_waveforms.py
    rx_spectrum_to_asciiplot.py
    benchmark_rate.py
    usrp_power_meter.py
    rx_to_remote_udp.py
    rfnoc_rx_to_file.py
    x440_L_band_capture.py
    rfnoc_txrx_fft_block_loopback.py
)

UHD_INSTALL(PROGRAMS ${python_examples}
    DESTINATION ${PKG_LIB_DIR}/examples/python
    COMPONENT examples)
