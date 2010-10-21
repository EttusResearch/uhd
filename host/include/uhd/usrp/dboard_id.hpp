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

#ifndef INCLUDED_UHD_USRP_DBOARD_ID_HPP
#define INCLUDED_UHD_USRP_DBOARD_ID_HPP

#include <uhd/config.hpp>
#include <boost/cstdint.hpp>
#include <boost/operators.hpp>
#include <string>

namespace uhd{ namespace usrp{

    class UHD_API dboard_id_t : boost::equality_comparable<dboard_id_t>{
    public:
        /*!
         * Create a dboard id from an integer.
         * \param id the integer representation
         */
        dboard_id_t(boost::uint16_t id = 0xffff);

        /*!
         * Obtain a dboard id that represents no dboard.
         * \return the dboard id with the 0xffff id.
         */
        static dboard_id_t none(void);

        /*!
         * Create a new dboard id from an integer representation.
         * \param uint16 an unsigned 16 bit integer
         * \return a new dboard id containing the integer
         */
        static dboard_id_t from_uint16(boost::uint16_t uint16);

        /*!
         * Get the dboard id represented as an integer.
         * \return an unsigned 16 bit integer representation
         */
        boost::uint16_t to_uint16(void) const;

        /*!
         * Create a new dboard id from a string representation.
         * If the string has a 0x prefix, it will be parsed as hex.
         * \param string a numeric string, possibly hex
         * \return a new dboard id containing the integer
         */
        static dboard_id_t from_string(const std::string &string);

        /*!
         * Get the dboard id represented as an integer.
         * \return a hex string representation with 0x prefix
         */
        std::string to_string(void) const;

        /*!
         * Get the dboard id represented as a canonical name.
         * \return the canonical string representation
         */
        std::string to_cname(void) const;

        /*!
         * Get the pretty print representation of this dboard id.
         * \return a string with the dboard name and id number
         */
        std::string to_pp_string(void) const;

    private:
        boost::uint16_t _id; //internal representation
    };

    /*!
     * Comparator operator overloaded for dboard ids.
     * The boost::equality_comparable provides the !=.
     * \param lhs the dboard id to the left of the operator
     * \param rhs the dboard id to the right of the operator
     * \return true when the dboard ids are equal
     */
    UHD_API bool operator==(const dboard_id_t &lhs, const dboard_id_t &rhs);

}} //namespace

#endif /* INCLUDED_UHD_USRP_DBOARD_ID_HPP */
