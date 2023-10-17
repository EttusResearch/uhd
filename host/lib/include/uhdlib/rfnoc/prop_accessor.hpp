//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/rfnoc/property.hpp>
#include <uhd/utils/scope_exit.hpp>
#include <functional>

namespace uhd { namespace rfnoc {

//! Special class which may access properties
//
// For the sake of property resolution, we require access to certain private
// members of properties. Instead of giving the entire graph access to
// everything, we create this accessor class which is not available
// in the public API.
class prop_accessor_t
{
public:
    //! Clear the dirty bit on a property
    void mark_clean(property_base_t& prop)
    {
        prop.mark_clean();
    }

    //! Set the access mode on a property
    void set_access(property_base_t& prop, const property_base_t::access_t access)
    {
        prop._access_mode = access;
    }

    //! Set the access mode on a property
    void set_access(property_base_t* prop, const property_base_t::access_t access)
    {
        prop->_access_mode = access;
    }

    //! RAII-Style access mode setter
    //
    // This will return an object which will set the access mode on a property
    // only for the duration of its own lifetime. Use this in situations where
    // you want to guarantee a certain read-write mode of a property, even when
    // an exception is thrown.
    //
    // \param prop A reference to the property
    // \param access The temporary access mode which will be set as long as the
    //               scope_exit object is alive
    // \param default_access The access mode which will be set once the
    //                       scope_exit object will be destroyed
    uhd::utils::scope_exit::uptr get_scoped_prop_access(property_base_t& prop,
        property_base_t::access_t access,
        property_base_t::access_t default_access = property_base_t::RO)
    {
        prop._access_mode = access;
        return uhd::utils::scope_exit::make(
            [&prop, default_access]() { prop._access_mode = default_access; });
    }

    /*! Forward the value from \p source to \p dst
     *
     * Note: This method will grant temporary write access to the destination
     * property!
     * If \p safe is set to true, it'll only allow RWLOCKED forwarding, i.e.,
     * the new value cannot be different.
     *
     * \throws uhd::type_error if types mismatch
     */
    template <bool safe>
    void forward(property_base_t* source, property_base_t* dst)
    {
        const auto w_access_type = (safe && dst->is_dirty()) ? property_base_t::RWLOCKED
                                                             : property_base_t::RW;
        auto read_access         = get_scoped_prop_access(
            *source, property_base_t::RO, source->get_access_mode());
        auto write_access =
            get_scoped_prop_access(*dst, w_access_type, dst->get_access_mode());
        source->forward(dst);
    }

    /*! Returns true if \p source and \p dst are of the same type
     */
    bool are_compatible(property_base_t* source, property_base_t* dst)
    {
        return source->is_type_equal(dst);
    }
};


}} /* namespace uhd::rfnoc */
