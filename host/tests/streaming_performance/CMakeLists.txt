#
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

set(streaming_performance_files
    parse_benchmark_rate.py
    run_benchmark_rate.py
    batch_run_benchmark_rate.py
    run_X300_performance_tests.py
)

UHD_INSTALL(PROGRAMS ${streaming_performance_files} DESTINATION ${PKG_LIB_DIR}/tests/streaming_performance COMPONENT tests)
