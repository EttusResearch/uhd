//
// Copyright 2017-2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <pybind11/pybind11.h>
#include <pybind11/complex.h>

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>

namespace py = pybind11;

// Allow boost::shared_ptr<T> to be a holder class of an object (PyBind11
// supports std::shared_ptr and std::unique_ptr out of the box)
#include <boost/shared_ptr.hpp>
PYBIND11_DECLARE_HOLDER_TYPE(T, boost::shared_ptr<T>);

#include "stream_python.hpp"

#include "types/types_python.hpp"
#include "types/serial_python.hpp"
#include "types/time_spec_python.hpp"
#include "types/metadata_python.hpp"
#include "types/sensors_python.hpp"
#include "types/filters_python.hpp"
#include "types/tune_python.hpp"

#include "usrp/fe_connection_python.hpp"
#include "usrp/dboard_iface_python.hpp"
#include "usrp/subdev_spec_python.hpp"
#include "usrp/multi_usrp_python.hpp"

// We need this hack because import_array() returns NULL
// for newer Python versions.
// This function is also necessary because it ensures access to the C API
// and removes a warning.
#if PY_MAJOR_VERSION >= 3
void* init_numpy()
{
    import_array();
    return NULL;
}
#else
void init_numpy()
{
    import_array();
}
#endif

PYBIND11_MODULE(libpyuhd, m)
{
    // Initialize the numpy C API
    // (otherwise we will see segmentation faults)
    init_numpy();

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
}

