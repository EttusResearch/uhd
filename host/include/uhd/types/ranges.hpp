//
// Copyright 2010-2012 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_TYPES_RANGES_HPP
#define INCLUDED_UHD_TYPES_RANGES_HPP

#include <uhd/config.hpp>
#include <string>
#include <vector>

namespace uhd{

    /*!
     * A range object describes a set of discrete values of the form:
     * y = start + step*n, where n is an integer between 0 and (stop - start)/step
     */
    class UHD_API range_t{
    public:

        /*!
         * Create a range from a single value.
         * The step size will be taken as zero.
         * \param value the only possible value in this range
         */
        range_t(double value = 0);

        /*!
         * Create a range from a full set of values.
         * A step size of zero implies infinite precision.
         * \param start the minimum value for this range
         * \param stop the maximum value for this range
         * \param step the step size for this range
         */
        range_t(double start, double stop, double step = 0);

        //! Get the start value for this range.
        double start(void) const;

        //! Get the stop value for this range.
        double stop(void) const;

        //! Get the step value for this range.
        double step(void) const;

        //! Convert this range to a printable string
        const std::string to_pp_string(void) const;

        //! Equality operator
        bool operator==(const range_t &other) const;

        //! Inequality operator
        bool operator!=(const range_t &other) const;

    private: double _start, _stop, _step;
    };

    /*!
     * A meta-range object holds a list of individual ranges.
     */
    struct UHD_API meta_range_t : std::vector<range_t>{

        //! A default constructor for an empty meta-range
        meta_range_t(void);

        /*!
         * Input iterator constructor:
         * Makes boost::assign::list_of work.
         * \param first the begin iterator
         * \param last the end iterator
         */
        template <typename InputIterator>
        meta_range_t(InputIterator first, InputIterator last):
            std::vector<range_t>(first, last){ /* NOP */ }

        /*!
         * A convenience constructor for a single range.
         * A step size of zero implies infinite precision.
         * \param start the minimum value for this range
         * \param stop the maximum value for this range
         * \param step the step size for this range
         */
        meta_range_t(double start, double stop, double step = 0);

        //! Get the overall start value for this meta-range.
        double start(void) const;

        //! Get the overall stop value for this meta-range.
        double stop(void) const;

        //! Get the overall step value for this meta-range.
        double step(void) const;

        /*!
         * Clip the target value to a possible range value.
         * \param value the value to clip to this range
         * \param clip_step if true, clip to steps as well
         * \return a value that is in one of the ranges
         */
        double clip(double value, bool clip_step = false) const;

        //! Convert this meta-range to a printable string
        const std::string to_pp_string(void) const;

    };

    typedef meta_range_t gain_range_t;
    typedef meta_range_t freq_range_t;

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_RANGES_HPP */
