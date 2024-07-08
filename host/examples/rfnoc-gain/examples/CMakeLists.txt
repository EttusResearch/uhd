#
# Copyright 2024 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

set(examples_sources_py
    rx_gain_estimate_power.py
)

# TODO: Properly shebang the Python files depending on platform

install(
    FILES ${examples_sources_py}
    DESTINATION ${UHD_PKG_DATA_DIR}/examples/rfnoc-gain
    COMPONENT examples
)

