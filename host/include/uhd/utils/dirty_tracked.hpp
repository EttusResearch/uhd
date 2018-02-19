//
// Copyright 2010-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_UTILS_DIRTY_TRACKED_HPP
#define INCLUDED_UHD_UTILS_DIRTY_TRACKED_HPP

namespace uhd{
    /*!
     * A class that wraps a data value with a dirty flag
     * When the client uses the assignment operator on this object, the object
     * automatically dirties itself if the assigned type is not equal the underlying
     * data. Data can be cleaned using the mark_clean entry-point.
     *
     * Requirements for data_t
     * - Must have a default constructor
     * - Must have a copy constructor
     * - Must have an assignment operator (=)
     * - Must have an equality operator (==)
     */
    template<typename data_t>
    class dirty_tracked {
    public:
        /*!
         * Default ctor: Initialize to default value and dirty
         */
        dirty_tracked() :
            _data(),                //data_t must have a default ctor
            _dirty(true)
        {}

        /*!
         * Initialize to specified value and dirty
         */
        dirty_tracked(const data_t& value) :
            _data(value),        //data_t must have a copy ctor
            _dirty(true)
        {}

        /*!
         * Copy ctor: Assign source to this type
         */
        dirty_tracked(const dirty_tracked& source) {
            *this = source;
        }

        /*!
         * Get underlying data
         */
        UHD_INLINE const data_t& get() const {
            return _data;
        }

        /*!
         * Has the underlying data changed since the last
         * time it was cleaned?
         */
        UHD_INLINE bool is_dirty() const {
            return _dirty;
        }

        /*!
         * Mark the underlying data as clean
         */
        UHD_INLINE void mark_clean() {
            _dirty = false;
        }

        /*!
         * Mark the underlying data as dirty
         */
        UHD_INLINE void force_dirty() {
            _dirty = true;
        }

        /*!
         * Assignment with data.
         * Store the specified value and mark it as dirty
         * if it is not equal to the underlying data.
         */
        UHD_INLINE dirty_tracked& operator=(const data_t& value)
        {
            if(!(_data == value)) {    //data_t must have an equality operator
                _dirty = true;
                _data = value;        //data_t must have an assignment operator
            }
            return *this;
        }

        /*!
         * Assignment with dirty tracked type.
         * Store the specified value from dirty type and mark it as dirty
         * if it is not equal to the underlying data.
         * This exists to optimize out an implicit cast from dirty_tracked
         * type to data type.
         */
        UHD_INLINE dirty_tracked& operator=(const dirty_tracked& source) {
            if (!(_data == source._data)) {
                _dirty = true;
                _data = source._data;
            }
            return *this;
        }

        /*!
         * Explicit conversion from this type to data_t
         */
        UHD_INLINE operator const data_t&() const {
            return get();
        }

    private:
        data_t _data;
        bool   _dirty;
    };

} //namespace uhd

#endif /* INCLUDED_UHD_UTILS_DIRTY_TRACKED_HPP */
