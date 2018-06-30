//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_USRP_COMPONENT_FILE_HPP
#define INCLUDED_UHD_USRP_COMPONENT_FILE_HPP

#include <uhd/types/dict.hpp>
#include <string>
#include <vector>

namespace uhd{ namespace usrp{

    /*! Defines a file that can be sent using update_component
     *
     * \param metadata dictionary of strings that define the metadata
     * associated with the file.
     *
     * Keys must include "id" and "filename", and may include an "md5" hash,
     * as well as other, device- or component-specific keys.
     *
     * \param data the binary data file
     *
     * Can be the contents of the FPGA image file, for example.
     */
    struct component_file_t {
        uhd::dict<std::string, std::string> metadata;
        std::vector<uint8_t> data;
    };

    typedef std::vector<component_file_t> component_files_t;

}} // namespace uhd::usrp

#endif /* INCLUDED_UHD_USRP_COMPONENT_FILE_HPP */
