//
// Copyright 2010-2011,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_UTILS_GAIN_GROUP_HPP
#define INCLUDED_UHD_UTILS_GAIN_GROUP_HPP

#include <uhd/config.hpp>
#include <uhd/types/ranges.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/utility.hpp>
#include <vector>
#include <string>

namespace uhd{

/*!
 * A set of function to control a gain element.
 */
struct UHD_API gain_fcns_t{
    boost::function<gain_range_t(void)> get_range;
    boost::function<double(void)>        get_value;
    boost::function<void(double)>        set_value;
};

class UHD_API gain_group : boost::noncopyable{
public:
    typedef boost::shared_ptr<gain_group> sptr;

    virtual ~gain_group(void) = 0;

    /*!
     * Get the gain range for the gain element specified by name.
     * For an empty name, get the overall gain range for this group.
     * Overall step is defined as the minimum step size.
     * \param name name of the gain element (optional)
     * \return a gain range with overall min, max, step
     */
    virtual gain_range_t get_range(const std::string &name = "") = 0;

    /*!
     * Get the gain value for the gain element specified by name.
     * For an empty name, get the overall gain value for this group.
     * \param name name of the gain element (optional)
     * \return a gain value of the element or all elements
     */
    virtual double get_value(const std::string &name = "") = 0;

    /*!
     * Set the gain value for the gain element specified by name.
     * For an empty name, set the overall gain value for this group.
     * The power will be distributed across individual gain elements.
     * The semantics of how to do this are determined by the priority.
     * \param gain the gain to set for the element or across the group
     * \param name name of the gain element (optional)
     */
    virtual void set_value(double gain, const std::string &name = "") = 0;

    /*!
     * Get a list of names of registered gain elements.
     * The names are in the order that they were registered.
     * \return a vector of gain name strings
     */
    virtual const std::vector<std::string> get_names(void) = 0;

    /*!
     * Register a set of gain functions into this group:
     *
     * The name should be a unique and non-empty name.
     * Otherwise, the implementation will rename it.
     *
     * Priority determines how power will be distributed
     * with higher priorities getting the power first,
     * and lower priorities getting the remainder power.
     *
     * \param name the name of the gain element
     * \param gain_fcns the set of gain functions
     * \param priority the priority of the gain element
     */
    virtual void register_fcns(
        const std::string &name,
        const gain_fcns_t &gain_fcns,
        size_t priority = 0
    ) = 0;

    /*!
     * Make a new empty gain group.
     * \return a gain group object.
     */
    static sptr make(void);
};

} //namespace uhd

#endif /* INCLUDED_UHD_UTILS_GAIN_GROUP_HPP */

