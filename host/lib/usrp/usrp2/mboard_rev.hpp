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

#ifndef INCLUDED_MBOARD_REV_HPP
#define INCLUDED_MBOARD_REV_HPP

#include <uhd/config.hpp>
#include <boost/cstdint.hpp>
#include <boost/operators.hpp>
#include <string>

class UHD_API mboard_rev_t : boost::equality_comparable<mboard_rev_t>, boost::less_than_comparable<mboard_rev_t>{
    public:
        /*!
         * Create a mboard rev from an integer.
         * \param rev the integer representation
         */
        mboard_rev_t(boost::uint16_t rev = 0xffff);

        /*!
         * Obtain a mboard rev that represents an invalid/uninit mboard ID
         * \return the mboard rev with the 0xffff rev.
         */
        static mboard_rev_t none(void);

        /*!
         * Create a new mboard rev from an integer representation.
         * \param uint16 an unsigned 16 bit integer
         * \return a new mboard rev containing the integer
         */
        static mboard_rev_t from_uint16(boost::uint16_t uint16);

        /*!
         * Get the mboard rev represented as an integer.
         * \return an unsigned 16 bit integer representation
         */
        boost::uint16_t to_uint16(void) const;

        /*!
         * Create a new mboard rev from a string representation.
         * If the string has a 0x prefix, it will be parsed as hex.
         * \param string a numeric string, possibly hex
         * \return a new dboard id containing the integer
         */
        static mboard_rev_t from_string(const std::string &string);

        /*!
         * Get the mboard rev represented as an integer.
         * \return a hex string representation with 0x prefix
         */
        std::string to_string(void) const;

        /*!
         * Get the pretty print representation of this mboard rev.
         * \return a string with the mboard name and rev number
         */
        std::string to_pp_string(void) const;
        
        /*!
         * Tell you if you're USRP2 or USRP2+
         * \return true if USRP2+, false if USRP2
         */
        bool is_usrp2p(void) const;
        
        /*!
         * Get the major revision number
         * \return major revision number
         */
        boost::uint8_t major(void) const;
        
        /*!
         * Get the minor revision number
         * \return minor revision number
         */
        boost::uint8_t minor(void) const;

    private:
        boost::uint16_t _rev; //internal representation
    };

    /*!
     * Comparator operator overloaded for mboard rev.
     * The boost::equality_comparable provides the !=.
     * \param lhs the mboard rev to the left of the operator
     * \param rhs the mboard rev to the right of the operator
     * \return true when the mboard revs are equal
     */
    UHD_API bool operator==(const mboard_rev_t &lhs, const mboard_rev_t &rhs);
    
    /*!
     * Comparator operator overloaded for mboard rev.
     * The boost::less_than_comparable provides the >, <=, >=.
     * \param lhs the mboard rev to the left of the operator
     * \param rhs the mboard rev to the right of the operator
     * \return true when lhs < rhs
     */
    
    UHD_API bool operator<(const mboard_rev_t &lhs, const mboard_rev_t &rhs);

#endif /* INCLUDED_MBOARD_REV_HPP */
