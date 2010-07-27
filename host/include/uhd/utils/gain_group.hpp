//
// Copyright 2010 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef INCLUDED_UHD_UTILS_GAIN_GROUP_HPP
#define INCLUDED_UHD_UTILS_GAIN_GROUP_HPP

#include <uhd/config.hpp>
#include <uhd/types/ranges.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/utility.hpp>

namespace uhd{

/*!
 * A set of function to control a gain element.
 */
struct UHD_API gain_fcns_t{
    boost::function<gain_range_t(void)> get_range;
    boost::function<float(void)>        get_value;
    boost::function<void(float)>        set_value;
};

class UHD_API gain_group : boost::noncopyable{
public:
    typedef boost::shared_ptr<gain_group> sptr;

    /*!
     * Get the overall gain range for this group.
     * Overall step is defined as the minimum step size.
     * \return a gain range with overall min, max, step
     */
    virtual gain_range_t get_range(void) = 0;

    /*!
     * Get the overall gain value for this group.
     * \return a summation of all the gain values
     */
    virtual float get_value(void) = 0;

    /*!
     * Set the overall gain value for this group.
     * The power will be distributed across individual gain elements.
     * The semantics of how to do this are determined by the priority.
     * \param gain the gain to set across the group
     */
    virtual void set_value(float gain) = 0;

    /*!
     * Register a set of gain functions into this group.
     * Priority determines how power will be distributed
     * with higher priorities getting the power first,
     * and lower priorities getting the remainder power.
     * \param gain_fcns the set of gain functions
     * \param priority the priority of the gain element
     */
    virtual void register_fcns(
        const gain_fcns_t &gain_fcns, size_t priority = 0
    ) = 0;

    /*!
     * Make a new empty gain group.
     * \return a gain group object.
     */
    static sptr make(void);
};

} //namespace uhd

#endif /* INCLUDED_UHD_UTILS_GAIN_GROUP_HPP */

