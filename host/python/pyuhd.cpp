//
// Copyright 2017-2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>

namespace py = pybind11;

#include "cal/cal_python.hpp"
#include "device_python.hpp"
#include "property_tree_python.hpp"
#include "rfnoc/ddc_block_control_python.hpp"
#include "rfnoc/duc_block_control_python.hpp"
#include "rfnoc/fft_block_control_python.hpp"
#include "rfnoc/fir_filter_block_control_python.hpp"
#include "rfnoc/fosphor_block_control_python.hpp"
#include "rfnoc/keep_one_in_n_block_control_python.hpp"
#include "rfnoc/moving_average_block_control_python.hpp"
#include "rfnoc/null_block_control_python.hpp"
#include "rfnoc/radio_control_python.hpp"
#include "rfnoc/replay_block_control_python.hpp"
#include "rfnoc/rfnoc_python.hpp"
#include "rfnoc/siggen_block_control_python.hpp"
#include "rfnoc/switchboard_block_control_python.hpp"
#include "rfnoc/vector_iir_block_control_python.hpp"
#include "rfnoc/window_block_control_python.hpp"
#include "stream_python.hpp"
#include "types/filters_python.hpp"
#include "types/metadata_python.hpp"
#include "types/sensors_python.hpp"
#include "types/serial_python.hpp"
#include "types/time_spec_python.hpp"
#include "types/tune_python.hpp"
#include "types/types_python.hpp"
#include "usrp/dboard_iface_python.hpp"
#include "usrp/fe_connection_python.hpp"
#include "usrp/multi_usrp_python.hpp"
#include "usrp/subdev_spec_python.hpp"
#include "utils/paths_python.hpp"
#include "utils/utils_python.hpp"

// We need this hack because import_array() returns NULL
// for newer Python versions.
// This function is also necessary because it ensures access to the C API
// and removes a warning.
void* init_numpy()
{
    import_array();
    return NULL;
}

PYBIND11_MODULE(libpyuhd, m)
{
    // Initialize the numpy C API
    // (otherwise we will see segmentation faults)
    init_numpy();

    // Register uhd::device::find
    export_device(m);

    // Register paths submodule
    auto paths_module = m.def_submodule("paths", "Path Utilities");
    export_paths(paths_module);

    // Register types submodule
    auto types_module = m.def_submodule("types", "UHD Types");

    export_types(types_module);
    export_time_spec(types_module);
    export_spi_config(types_module);
    export_metadata(types_module);
    export_sensors(types_module);
    export_tune(types_module);

    // Register usrp submodule
    auto usrp_module = m.def_submodule("usrp", "USRP Objects");
    export_multi_usrp(usrp_module);
    export_subdev_spec(usrp_module);
    export_dboard_iface(usrp_module);
    export_fe_connection(usrp_module);
    export_stream(usrp_module);

    // Register filters submodule
    auto filters_module = m.def_submodule("filters", "Filter Submodule");
    export_filters(filters_module);

    // Register RFNoC submodule
    auto rfnoc_module = m.def_submodule("rfnoc", "RFNoC Objects");
    export_rfnoc(rfnoc_module);
    export_ddc_block_control(rfnoc_module);
    export_duc_block_control(rfnoc_module);
    export_fft_block_control(rfnoc_module);
    export_fosphor_block_control(rfnoc_module);
    export_fir_filter_block_control(rfnoc_module);
    export_keep_one_in_n_block_control(rfnoc_module);
    export_moving_average_block_control(rfnoc_module);
    export_null_block_control(rfnoc_module);
    export_radio_control(rfnoc_module);
    export_replay_block_control(rfnoc_module);
    export_siggen_block_control(rfnoc_module);
    export_switchboard_block_control(rfnoc_module);
    export_vector_iir_block_control(rfnoc_module);
    export_window_block_control(rfnoc_module);

    // Register calibration submodule
    auto cal_module = m.def_submodule("cal", "Calibration Objects");
    export_cal(cal_module);

    auto chdr_module = m.def_submodule("chdr", "CHDR Parsing");
    export_utils(chdr_module);

    // Register property tree
    export_property_tree(m);
}
