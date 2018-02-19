//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/utils/compat_check.hpp>
#include <boost/format.hpp>

void uhd::assert_fpga_compat(
    const size_t uhd_major,
    const size_t uhd_minor,
    const uint64_t fpga_compat,
    const std::string& fpga_component,
    const std::string& log_component,
    const bool fail_on_minor_behind
) {
    const size_t fpga_major = fpga_compat >> 32;
    const size_t fpga_minor = fpga_compat & 0xFFFFFFFF;

    if (!log_component.empty()) {
        UHD_LOGGER_DEBUG(log_component)
            << "Checking compat number for FPGA component `" << fpga_component
            << "': Expecting " << uhd_major << "." << uhd_minor << ", actual: "
            << fpga_major << "." << fpga_minor << "."
        ;
    }

    if (uhd_major > fpga_major) {
        if (!log_component.empty()) {
            UHD_LOGGER_ERROR(log_component)
                << "Major compat number mismatch for " << fpga_component << ":"
                   " Expecting " << uhd_major << ", got " << fpga_major << "."
            ;
        }
        throw uhd::runtime_error(str(
            boost::format("FPGA component `%s' is revision %d and UHD supports"
                          " revision %d. Please either upgrade the FPGA"
                          " image (recommended) or downgrade UHD.")
            % fpga_component
            % fpga_major
            % uhd_major
        ));
    }
    if (uhd_major < fpga_major) {
        if (!log_component.empty()) {
            UHD_LOGGER_ERROR(log_component)
                << "Major compat number mismatch for " << fpga_component << ":"
                " Expecting " << uhd_major << ", got " << fpga_major << "."
            ;
        }
        throw uhd::runtime_error(str(
            boost::format("FPGA component `%s' is revision %d and UHD supports"
                          " revision %d. Please either upgrade UHD "
                          " (recommended) or downgrade the FPGA image.")
            % fpga_component
            % fpga_major
            % uhd_major
        ));

    }
    if (uhd_minor > fpga_minor) {
        if (fail_on_minor_behind) {
            if (!log_component.empty()) {
                UHD_LOGGER_ERROR(log_component) << str(
                    boost::format("Minor compat number mismatch for `%s':"
                                  " Expecting %d.%d, got %d.%d.")
                    % fpga_component
                    % uhd_major
                    % uhd_minor
                    % fpga_major
                    % fpga_minor
                );
            }
            throw uhd::runtime_error(str(
                boost::format("FPGA component `%s' is revision %d.%d and UHD supports"
                              " revision %d.%d. Please either upgrade UHD "
                              " (recommended) or downgrade the FPGA image.")
                % fpga_component
                % fpga_major
                % fpga_minor
                % uhd_major
                % uhd_minor
            ));
        } else {
            if (!log_component.empty()) {
                UHD_LOGGER_WARNING(log_component) << str(
                    boost::format("Non-critical minor compat number mismatch "
                                  "for `%s': Expecting %d.%d, got %d.%d.")
                    % fpga_component
                    % uhd_major
                    % uhd_minor
                    % fpga_major
                    % fpga_minor
                );
            }
        }
    } else if (uhd_minor < fpga_minor) {
        if (!log_component.empty()) {
            UHD_LOGGER_WARNING(log_component) << str(
                boost::format("Non-critical minor compat number mismatch "
                              "for `%s': Expecting %d.%d, got %d.%d.")
                % fpga_component
                % uhd_major
                % uhd_minor
                % fpga_major
                % fpga_minor
            );
        }
    }
    // We got here? Then all is good.
}
