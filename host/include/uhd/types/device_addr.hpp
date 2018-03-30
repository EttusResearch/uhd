//
// Copyright 2010-2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_TYPES_DEVICE_ADDR_HPP
#define INCLUDED_UHD_TYPES_DEVICE_ADDR_HPP

#include <uhd/config.hpp>
#include <uhd/types/dict.hpp>
#include <boost/lexical_cast.hpp>
#include <stdexcept>
#include <vector>
#include <string>
#include <map>

namespace uhd{

    /*!
     * Mapping of key/value pairs for locating devices on the system.
     * When left empty, the device discovery routines will search
     * all available transports on the system (ethernet, usb...).
     *
     * To narrow down the discovery process to a particular device,
     * specify a transport key/value pair specific to your device.
     * - Ex, to find a usrp2: my_dev_addr["addr"] = [resolvable_hostname_or_ip]
     *
     * The device address can also be used to pass arguments into
     * the transport layer control to set (for example) buffer sizes.
     *
     * An arguments string, is a way to represent a device address
     * using a single string with delimiter characters.
     * - Ex: addr=192.168.10.2
     * - Ex: addr=192.168.10.2, recv_buff_size=1e6
     */
    class UHD_API device_addr_t : public dict<std::string, std::string>{
    public:
        /*!
         * Create a device address from an args string.
         * \param args the arguments string
         */
        device_addr_t(const std::string &args = "");

        /*!
         * Create a device address from a std::map
         * \param info the device info map
         */
        device_addr_t(const std::map<std::string, std::string> &info);

        /*!
         * Convert a device address into a pretty print string.
         * \return a printable string representing the device address
         */
        std::string to_pp_string(void) const;

        /*!
         * Convert the device address into an args string.
         * The args string contains delimiter symbols.
         * \return a string with delimiter markup
         */
        std::string to_string(void) const;

        /*!
         * Lexically cast a parameter to the specified type,
         * or use the default value if the key is not found.
         * \param key the key as one of the address parameters
         * \param def the value to use when key is not present
         * \return the casted value as type T or the default
         * \throw error when the parameter cannot be casted
         */
        template <typename T> T cast(const std::string &key, const T &def) const{
            if (not this->has_key(key)) return def;
            try{
                return boost::lexical_cast<T>((*this)[key]);
            }
            catch(const boost::bad_lexical_cast &){
                throw std::runtime_error("cannot cast " + key + " = " + (*this)[key]);
            }
        }
    };

    //! A typedef for a vector of device addresses
    typedef std::vector<device_addr_t> device_addrs_t;

    //! Separate an indexed device address into a vector of device addresses
    UHD_API device_addrs_t separate_device_addr(const device_addr_t &dev_addr);

    //! Combine a vector of device addresses into an indexed device address
    UHD_API device_addr_t combine_device_addrs(const device_addrs_t &dev_addrs);

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_DEVICE_ADDR_HPP */
