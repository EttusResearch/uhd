//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/features/discoverable_feature.hpp>
#include <uhdlib/features/discoverable_feature_registry.hpp>
#include <vector>

namespace uhd { namespace features {

std::vector<std::string> discoverable_feature_registry::enumerate_features()
{
    std::vector<std::string> features;
    for (auto& entry : _features) {
        features.push_back(entry.second->get_feature_name());
    }
    return features;
}

discoverable_feature::sptr discoverable_feature_registry::get_feature_ptr(
    discoverable_feature::feature_id_t feature_id)
{
    auto it = _features.find(feature_id);
    if (it == _features.end()) {
        return discoverable_feature::sptr();
    }
    return it->second;
}

}} // namespace uhd::features
