//
// Copyright 2010-2012 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_TYPES_TIME_SPEC_HPP
#define INCLUDED_UHD_TYPES_TIME_SPEC_HPP

#include <uhd/config.hpp>
#include <boost/operators.hpp>
#include <ctime>

namespace uhd{

    /*!
     * A time_spec_t holds a seconds and a fractional seconds time value.
     * Depending upon usage, the time_spec_t can represent absolute times,
     * relative times, or time differences (between absolute times).
     *
     * The time_spec_t provides clock-domain independent time storage,
     * but can convert fractional seconds to/from clock-domain specific units.
     *
     * The fractional seconds are stored as double precision floating point.
     * This gives the fractional seconds enough precision to unambiguously
     * specify a clock-tick/sample-count up to rates of several petahertz.
     */
    class UHD_API time_spec_t : boost::additive<time_spec_t>, boost::totally_ordered<time_spec_t>{
    public:

        /*!
         * Create a time_spec_t from a real-valued seconds count.
         * \param secs the real-valued seconds count (default = 0)
         */
        time_spec_t(double secs = 0);

        /*!
         * Create a time_spec_t from whole and fractional seconds.
         * \param full_secs the whole/integer seconds count
         * \param frac_secs the fractional seconds count (default = 0)
         */
        time_spec_t(time_t full_secs, double frac_secs = 0);

        /*!
         * Create a time_spec_t from whole seconds and fractional ticks.
         * Translation from clock-domain specific units.
         * \param full_secs the whole/integer seconds count
         * \param tick_count the fractional seconds tick count
         * \param tick_rate the number of ticks per second
         */
        time_spec_t(time_t full_secs, long tick_count, double tick_rate);

        /*!
         * Create a time_spec_t from a 64-bit tick count.
         * Translation from clock-domain specific units.
         * \param ticks an integer count of ticks
         * \param tick_rate the number of ticks per second
         */
        static time_spec_t from_ticks(long long ticks, double tick_rate);

        /*!
         * Convert the fractional seconds to clock ticks.
         * Translation into clock-domain specific units.
         * \param tick_rate the number of ticks per second
         * \return the fractional seconds tick count
         */
        long get_tick_count(double tick_rate) const;

        /*!
         * Convert the time spec into a 64-bit tick count.
         * Translation into clock-domain specific units.
         * \param tick_rate the number of ticks per second
         * \return an integer number of ticks
         */
        long long to_ticks(const double tick_rate) const;

        /*!
         * Get the time as a real-valued seconds count.
         * Note: If this time_spec_t represents an absolute time,
         * the precision of the fractional seconds may be lost.
         * \return the real-valued seconds
         */
        double get_real_secs(void) const;

        /*!
         * Get the whole/integer part of the time in seconds.
         * \return the whole/integer seconds
         */
        time_t get_full_secs(void) const;

        /*!
         * Get the fractional part of the time in seconds.
         * \return the fractional seconds
         */
        double get_frac_secs(void) const;

        //! Implement addable interface
        time_spec_t &operator+=(const time_spec_t &);
        time_spec_t &operator+=(double &);
        time_spec_t operator+(double &);
        time_spec_t operator+(const time_spec_t &);
        //! Implement subtractable interface
        time_spec_t &operator-=(const time_spec_t &);

    //private time storage details
    private: time_t _full_secs; double _frac_secs;
    };

    //! Implement equality_comparable interface
    UHD_API bool operator==(const time_spec_t &, const time_spec_t &);

    //! Implement less_than_comparable interface
    UHD_API bool operator<(const time_spec_t &, const time_spec_t &);

    UHD_INLINE time_t time_spec_t::get_full_secs(void) const{
        return this->_full_secs;
    }

    UHD_INLINE double time_spec_t::get_frac_secs(void) const{
        return this->_frac_secs;
    }

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_TIME_SPEC_HPP */
