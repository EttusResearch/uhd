//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/extension/extension.hpp>
#include <uhd/utils/static.hpp>
#include <uhdlib/extension/extension_factory.hpp>
#include <unordered_map>


using namespace uhd::extension;

using extension_registry_t =
    std::unordered_map<std::string /* extension_key */, extension::factory_type>;
UHD_SINGLETON_FCN(extension_registry_t, get_extension_registry);

void extension::register_extension(
    const std::string& extension_name, extension::factory_type factory_fn)
{
    if (get_extension_registry().count(extension_name)) {
        std::cerr << "[REGISTRY] WARNING: Attempting to overwrite previously "
                     "registered extension with extension key"
                  << extension_name << std::endl;
        return;
    }
    get_extension_registry().insert({extension_name, std::move(factory_fn)});
}

/******************************************************************************
 * Factory functions
 *****************************************************************************/

extension::factory_type extension_factory::get_extension_factory(
    const std::string& ext_name)
{
    if (!get_extension_registry().count(ext_name)) {
        UHD_LOG_WARNING(
            "EXTENSION_REGISTRY", "Could not find extension of name " << ext_name);
        std::string ext_available = "Installed extensions:";
        for (auto& it : get_extension_registry()) {
            ext_available += " " + it.first;
        }
        UHD_LOG_WARNING("EXTENSION_REGISTRY", ext_available);

        return nullptr;
    }
    return get_extension_registry().at(ext_name);
}
