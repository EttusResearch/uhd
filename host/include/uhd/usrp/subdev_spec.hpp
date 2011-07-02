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

#ifndef INCLUDED_UHD_USRP_SUBDEV_SPEC_HPP
#define INCLUDED_UHD_USRP_SUBDEV_SPEC_HPP

#include <uhd/config.hpp>
#include <boost/operators.hpp>
#include <vector>
#include <string>

namespace uhd{ namespace usrp{

    /*!
     * A subdevice specification (daughterboard slot, subdevice) name pairing.
     */
    struct UHD_API subdev_spec_pair_t : boost::equality_comparable<subdev_spec_pair_t>{
        //! The daughterboard slot name
        std::string db_name;

        //! The subdevice name
        std::string sd_name;

        /*!
         * Create a new subdevice specification pair from dboard and subdev names.
         * \param db_name the name of a daughterboard slot
         * \param sd_name the name of a subdevice on that daughterboard
         */
        subdev_spec_pair_t(
            const std::string &db_name = "",
            const std::string &sd_name = ""
        );
    };

    //! overloaded comparison operator for subdev_spec_pair_t
    UHD_API bool operator==(const subdev_spec_pair_t &, const subdev_spec_pair_t &);

    /*!
     * A list of (daughterboard slot name, subdevice name) pairs:
     *
     * A subdevice specification represents a list of subdevices on a motherboard.
     * The subdevices specified may span across multiple daughterboards;
     * Hence the need for a subdevice specification over a simple list of strings.
     * Typically, the user will pass a RX or TX subdevice specification into the API,
     * and the implementation will infer the channel configuration from the specification.
     *
     * The subdevice specification can be represented as a markup-string.
     * The markup-string is a whitespace separated list of dboard:subdev pairs.
     * The first pair represents the subdevice for channel zero,
     * the second pair represents the subdevice for channel one, and so on.
     */
    class UHD_API subdev_spec_t : public std::vector<subdev_spec_pair_t>{
    public:

        /*!
         * Create a subdev specification from a markup string.
         * \param markup the markup string
         */
        subdev_spec_t(const std::string &markup = "");

        /*!
         * Convert a subdev specification into a pretty print string.
         * \return a printable string representing the subdev specification
         */
        std::string to_pp_string(void) const;

        /*!
         * Convert the subdevice specification into a markup string.
         * The markup string contains the delimiter symbols.
         * \return a string with delimiter markup
         */
        std::string to_string(void) const;
    };

}}

#endif /* INCLUDED_UHD_USRP_SUBDEV_SPEC_HPP */
