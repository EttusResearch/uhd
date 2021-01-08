//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/exception.hpp>
#include <uhd/features/discoverable_feature.hpp>
#include <uhd/features/discoverable_feature_getter_iface.hpp>
#include <map>
#include <memory>
#include <vector>

namespace uhd { namespace features {

/*! Map-based registry to implement discoverable_feature_getter_iface
 */
class discoverable_feature_registry : public virtual discoverable_feature_getter_iface
{
public:
    ~discoverable_feature_registry() override = default;

    std::vector<std::string> enumerate_features() override;

    template <typename T>
    void register_feature(std::shared_ptr<T> feature)
    {
        if (!_features.emplace(T::get_feature_id(), feature).second) {
            UHD_ASSERT_THROW(false);
        }
    }

private:
    discoverable_feature::sptr get_feature_ptr(
        discoverable_feature::feature_id_t feature_id) override;

    std::map<discoverable_feature::feature_id_t, discoverable_feature::sptr> _features;
};

}} // namespace uhd::features
