//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHDLIB_UTILS_COMPATCHECK_HPP
#define INCLUDED_UHDLIB_UTILS_COMPATCHECK_HPP

#include <cstddef>
#include <string>

namespace uhd {

    /*! Checks for FPGA compatibility, and throws an exception on mismatch.
     *
     * \throws uhd::runtime_error on mismatch.
     */
    void assert_fpga_compat(
        const size_t uhd_major,
        const size_t uhd_minor,
        const uint64_t fpga_compat,
        const std::string& fpga_component,
        const std::string& log_component,
        const bool fail_on_minor_behind=false
    );

} /* namespace uhd */

#endif /* INCLUDED_UHDLIB_UTILS_COMPATCHECK_HPP */
