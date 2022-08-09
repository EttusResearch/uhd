//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/exception.hpp>
#include <uhd/rfnoc/res_source_info.hpp>
#include <uhd/utils/cast.hpp>
#include <uhd/utils/dirty_tracked.hpp>
#include <memory>
#include <string>


namespace uhd { namespace rfnoc {

// Forward declaration, separates includes
class prop_accessor_t;

/*! Base class for properties
 *
 */
class UHD_API property_base_t
{
public:
    enum access_t {
        NONE, //!< Neither reading nor writing to this property is permitted
        RO       = 0x1, //!< Read-Only
        RW       = 0x3, //!< Read-Write
        RWLOCKED = 0x5 //!< Write is locked. This lets you call set(), but only if the
                       //!< value is unchanged.
    };

    property_base_t(const std::string& id, const res_source_info& source_info)
        : _id(id), _source_info(source_info)
    {
        if (_id.find(':') != std::string::npos) {
            throw uhd::value_error(
                "Property ID `" + _id + "' contains invalid character!");
        }
    }

    virtual ~property_base_t()
    {
        // nop
    }

    //! Gets the ID (name) of this property
    const std::string& get_id() const
    {
        return _id;
    }

    //! Return the source info for this property
    const res_source_info& get_src_info() const
    {
        return _source_info;
    }

    //! Query this property's dirty flag.
    //
    // If it's true, that means this property was recently changed, but changes
    // have not propagated yet and still need resolving.
    virtual bool is_dirty() const = 0;

    //! Query this property's valid flag.
    //
    // If it's false, that means this property has a default value that should
    // NOT be forwarded.
    virtual bool is_valid() const = 0;

    //! Returns true if this property can be read.
    bool read_access_granted() const
    {
        return static_cast<uint8_t>(_access_mode) & 0x1;
    }

    //! Returns true if this property can be written to.
    bool write_access_granted() const
    {
        return static_cast<uint8_t>(_access_mode) & 0x2;
    }

    //! Return the current access mode
    access_t get_access_mode() const
    {
        return _access_mode;
    }

    //! Return true if rhs has the same type and value
    virtual bool equal(property_base_t* rhs) const = 0;

    //! Create a copy of this property
    //
    // The copy must have the same type, value, and ID. However, it is often
    // desirable to have a new source information, so that can be overridden.
    //
    // The cleanliness state of \p original is not preserved. The new property
    // will have the same cleanliness state as any other new property.
    //
    virtual std::unique_ptr<property_base_t> clone(res_source_info)
    {
        throw uhd::not_implemented_error("Cloning is not available for this property.");
    }

    virtual void force_dirty() = 0;

    /*! Set this property's value using a string
     *
     * This requires the underlying property type to be convertible from a
     * string.
     *
     * \throws uhd::runtime_error if the underlying type has no conversion from
     *         a string
     */
    virtual void set_from_str(const std::string& new_val_str) = 0;

private:
    friend class prop_accessor_t;

    //! Reset the dirty bit. See also dirty_tracked::mark_clean()
    virtual void mark_clean() = 0;

    //! Forward the value of this property to another one
    virtual void forward(property_base_t* next_prop) = 0;

    //! Compare property types
    //
    // Note: This uses RTTI to evaluate type equality
    virtual bool is_type_equal(property_base_t* other_prop) const = 0;

    /*** Attributes **********************************************************/
    //! Stores an ID string for this property. They don't need to be unique.
    const std::string _id;

    //! Stores the source info for this property.
    const res_source_info _source_info;

    //! Access mode. Note that the prop_accessor_t is the only one who can
    // write this.
    access_t _access_mode = RO;
};

/*!
 * An encapsulation class for a block property.
 */
template <typename data_t>
class UHD_API_HEADER property_t : public property_base_t
{
public:
    //! We want to be good C++ citizens
    using value_type = data_t;

    property_t(const std::string& id, data_t&& value, const res_source_info& source_info);

    property_t(
        const std::string& id, const data_t& value, const res_source_info& source_info);

    property_t(const std::string& id, const res_source_info& source_info);

    property_t(const property_t<data_t>& prop) = default;

    //! Returns the dirty state of this property
    //
    // If true, this means the value was recently changed, but it wasn't marked
    // clean yet.
    bool is_dirty() const override
    {
        return _data.is_dirty();
    }

    //! Query this property's valid flag.
    //
    // If it's false, that means this property has a default value that should
    // NOT be used.
    bool is_valid() const override
    {
        return _valid;
    }

    bool equal(property_base_t* rhs) const override
    {
        if (!is_type_equal(rhs)) {
            return false;
        }
        return get() == dynamic_cast<property_t<data_t>*>(rhs)->get();
    }

    std::unique_ptr<property_base_t> clone(res_source_info new_src_info) override
    {
        return std::unique_ptr<property_base_t>(
            new property_t<data_t>(get_id(), get(), new_src_info));
    }

    void set_from_str(const std::string& new_val_str) override
    {
        try {
            set(uhd::cast::from_str<data_t>(new_val_str));
        } catch (uhd::runtime_error& ex) {
            throw uhd::runtime_error(
                std::string("Property ") + get_id() + ":" + ex.what());
        }
    }

    //! Returns the source info for the property
    // const res_source_info& get_src_info() const = 0;

    //! Set the value of this property
    //
    // \throws uhd::access_error if the current access mode is not RW or RWLOCKED
    // \throws uhd::resolve_error if the property is RWLOCKED but the new value
    //         doesn't match
    void set(const data_t& value)
    {
        if (write_access_granted()) {
            _data  = value;
            _valid = true;
        } else if (get_access_mode() == RWLOCKED) {
            if (_data.get() != value) {
                throw uhd::resolve_error(std::string("Attempting to overwrite property `")
                                         + get_id() + "@" + get_src_info().to_string()
                                         + "' with a new value after it was locked!");
            }
        } else {
            throw uhd::access_error(std::string("Attempting to write to property `")
                                    + get_id() + "' without access privileges!");
        }
    }

    void force_dirty() override
    {
        if (write_access_granted()) {
            _data.force_dirty();
        } else if (get_access_mode() == RWLOCKED) {
            if (!_data.is_dirty()) {
                throw uhd::resolve_error(std::string("Attempting to overwrite property `")
                                         + get_id()
                                         + "' with dirty flag after it was locked!");
            }
        } else {
            throw uhd::access_error(std::string("Attempting to flag dirty property `")
                                    + get_id() + "' without access privileges!");
        }
    }

    //! Get the value of this property
    //
    // \throws uhd::access_error if either the property is flagged as invalid,
    //         or if no read access was granted.
    const data_t& get() const
    {
        if (!is_valid()) {
            throw uhd::access_error(std::string("Attempting to read property `")
                                    + get_id() + "@" + get_src_info().to_string()
                                    + "' before it was initialized!");
        }
        if (read_access_granted()) {
            return _data;
        }
        throw uhd::access_error(std::string("Attempting to read property `") + get_id()
                                + "' without access privileges!");
    }

    operator const data_t&() const
    {
        return get();
    }

    bool operator==(const data_t& rhs)
    {
        return get() == rhs;
    }

    property_t<data_t>& operator=(const data_t& value)
    {
        set(value);
        return *this;
    }

private:
    void mark_clean() override
    {
        _data.mark_clean();
    }

    void forward(property_base_t* next_prop) override
    {
        if (not _valid) {
            throw uhd::resolve_error(
                std::string("Unable to forward invalid property ") + get_id());
        }
        property_t<data_t>* prop_ptr = dynamic_cast<property_t<data_t>*>(next_prop);
        if (prop_ptr == nullptr) {
            throw uhd::type_error(std::string("Unable to cast property ")
                                  + next_prop->get_id() + " to the same type as property "
                                  + get_id());
        }

        prop_ptr->set(get());
    }

    bool is_type_equal(property_base_t* other_prop) const override
    {
        return dynamic_cast<property_t<data_t>*>(other_prop) != nullptr;
    }

    dirty_tracked<data_t> _data;
    bool _valid;
}; // class property_t

}} /* namespace uhd::rfnoc */

#include <uhd/rfnoc/property.ipp>
