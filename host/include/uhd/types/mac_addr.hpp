//
// Copyright 2010 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_TYPES_MAC_ADDR_HPP
#define INCLUDED_UHD_TYPES_MAC_ADDR_HPP

#include <uhd/config.hpp>
#include <uhd/types/serial.hpp>
#include <string>

namespace uhd{

    /*!
    * Wrapper for an ethernet mac address.
    * Provides conversion between string and binary formats.
    */
    class UHD_API mac_addr_t{
    public:
        /*!
         * Create a mac address a byte array.
         * \param bytes a vector of bytes
         * \return a new mac address
         */
        static mac_addr_t from_bytes(const byte_vector_t &bytes);

        /*!
         * Create a mac address from a string.
         * \param mac_addr_str the string with delimiters
         * \return a new mac address
         */
        static mac_addr_t from_string(const std::string &mac_addr_str);

        /*!
         * Get the byte representation of the mac address.
         * \return a vector of bytes
         */
        byte_vector_t to_bytes(void) const;

        /*!
         * Get the string representation of this mac address.
         * \return a string with delimiters
         */
        std::string to_string(void) const;

    private:
        mac_addr_t(const byte_vector_t &bytes); //private constructor
        const byte_vector_t _bytes; //internal representation
    };

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_MAC_ADDR_HPP */
