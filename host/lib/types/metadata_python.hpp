//
// Copyright 2017 Ettus Research
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef INCLUDED_UHD_METADATA_PYTHON_HPP
#define INCLUDED_UHD_METADATA_PYTHON_HPP

#include <uhd/types/ranges.hpp>

void export_metadata()
{
    using range_t       = uhd::range_t;
    using meta_range_t  = uhd::meta_range_t;
    using rx_metadata_t = uhd::rx_metadata_t;
    using error_code_t  = rx_metadata_t::error_code_t;
    using tx_metadata_t = uhd::tx_metadata_t;

    bp::enum_<error_code_t>("rx_metadata_error_code")
        .value("none"        , error_code_t::ERROR_CODE_NONE        )
        .value("timeout"     , error_code_t::ERROR_CODE_TIMEOUT     )
        .value("late"        , error_code_t::ERROR_CODE_LATE_COMMAND)
        .value("broken_chain", error_code_t::ERROR_CODE_BROKEN_CHAIN)
        .value("overflow"    , error_code_t::ERROR_CODE_OVERFLOW    )
        .value("alignment"   , error_code_t::ERROR_CODE_ALIGNMENT   )
        .value("bad_packet"  , error_code_t::ERROR_CODE_BAD_PACKET  )
        ;

    bp::class_<range_t>
        ("range", bp::init<double>())

        // Constructors
        .def(bp::init<double, double, double>())

        // Methods
        .def("start"  , &range_t::start       )
        .def("stop"   , &range_t::stop        )
        .def("step"   , &range_t::step        )
        .def("__str__", &range_t::to_pp_string)
        ;

    bp::class_<std::vector<range_t> >("range_vector")
        .def(bp::vector_indexing_suite<std::vector<range_t> >());

    bp::class_<meta_range_t, bp::bases<std::vector<range_t> > >
        ("meta_range", bp::init<>())

        // Constructors
        .def(bp::init<double, double, double>())

        // Methods
        .def("start"  , &meta_range_t::start       )
        .def("stop"   , &meta_range_t::stop        )
        .def("step"   , &meta_range_t::step        )
        .def("clip"   , &meta_range_t::clip        )
        .def("__str__", &meta_range_t::to_pp_string)
        ;

    bp::class_<rx_metadata_t>("rx_metadata", bp::init<>())

        // Methods
        .def("reset"       , &rx_metadata_t::reset       )
        .def("to_pp_string", &rx_metadata_t::to_pp_string)
        .def("strerror"    , &rx_metadata_t::strerror    )
        .def("__str__"     , &rx_metadata_t::to_pp_string, bp::args("compact") = false)

        // Properties
        .def_readonly("has_time_spec"  , &rx_metadata_t::has_time_spec  )
        .def_readonly("time_spec"      , &rx_metadata_t::time_spec      )
        .def_readonly("more_fragments" , &rx_metadata_t::more_fragments )
        .def_readonly("start_of_burst" , &rx_metadata_t::start_of_burst )
        .def_readonly("end_of_burst"   , &rx_metadata_t::end_of_burst   )
        .def_readonly("error_code"     , &rx_metadata_t::error_code     )
        .def_readonly("out_of_sequence", &rx_metadata_t::out_of_sequence)
        ;

    bp::class_<tx_metadata_t>("tx_metadata", bp::init<>())

        // Properties
        .def_readwrite("has_time_spec" , &tx_metadata_t::has_time_spec )
        .def_readwrite("time_spec"     , &tx_metadata_t::time_spec     )
        .def_readwrite("start_of_burst", &tx_metadata_t::start_of_burst)
        .def_readwrite("end_of_burst"  , &tx_metadata_t::end_of_burst  )
        ;
}

#endif /* INCLUDED_UHD_METADATA_PYTHON_HPP */
