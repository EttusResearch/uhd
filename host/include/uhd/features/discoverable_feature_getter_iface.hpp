//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/exception.hpp>
#include <uhd/features/discoverable_feature.hpp>
#include <vector>

namespace uhd { namespace features {

/*! Interface for discovering and accessing discoverable features.
 */
class UHD_API discoverable_feature_getter_iface
{
public:
    virtual ~discoverable_feature_getter_iface() = default;

    //! Retrieves a feature of the specified type.
    //
    // Note that if the given feature type does not exist, this function will
    // assert. The user should first check that the feature exists via the
    // has_feature method.
    // Usage:
    // auto feature_ref = radio.get_feature<desired_feature_class>();
    template <typename T>
    T& get_feature()
    {
        auto p = get_feature_ptr(T::get_feature_id());
        UHD_ASSERT_THROW(p);
        auto typed_p = dynamic_cast<T*>(p.get());
        UHD_ASSERT_THROW(typed_p);
        return *typed_p;
    }

    //! Determines whether a given feature exists
    //
    // This function should be used to gate functionality before calling
    // get_feature().
    template <typename T>
    bool has_feature()
    {
        return bool(get_feature_ptr(T::get_feature_id()));
    }

    //! Enumerate all discoverable features present on the device.
    //
    // Returns a vector (in no particular order) of the features that this
    // device supports.
    virtual std::vector<std::string> enumerate_features() = 0;

private:
    //! Get a shared pointer to a feature, if it exists.
    //
    // If the feature does not exist on the device, returns a null pointer.
    virtual discoverable_feature::sptr get_feature_ptr(
        discoverable_feature::feature_id_t feature_id) = 0;
};

}} // namespace uhd::features
