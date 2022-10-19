#
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

# List all header files here (UHD and GNU Radio)
install(
    FILES
    usrp.h
    usrp_factory.h
    file_writer.h
    file_writer_factory.h
    data_metrics.h
    bitdown_block_ctrl.hpp
    DESTINATION include/rfnoc/convert
    COMPONENT headers
)
