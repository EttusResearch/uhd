//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/extension/extension.hpp>

namespace uhd { namespace extension {

class extension_factory
{
public:
    /*! Return a factory function for an RF Extension
     *
     * \returns a reference to the make function of the extension
     * \throws UHD_LOG_WARNING if requested extension is not found
     */
    static extension::factory_type get_extension_factory(const std::string& name);
};
}} // namespace uhd::extension