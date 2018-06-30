//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_USRP_CLOCK_OCTOCLOCK_EEPROM_HPP
#define INCLUDED_UHD_USRP_CLOCK_OCTOCLOCK_EEPROM_HPP

#include <uhd/config.hpp>
#include <uhd/transport/udp_simple.hpp>
#include <uhd/types/dict.hpp>
#include <string>

namespace uhd{ namespace usrp_clock{

/*!
 * The OctoClock EEPROM object:
 * Knows how to read and write the OctoClock EEPROM.
 * The class inherits from a string, string dictionary.
 * Use the dictionary interface to get and set values.
 * Commit to the EEPROM to save changed settings.
 */
class UHD_API octoclock_eeprom_t : public uhd::dict<std::string, std::string>{
public:
    //! Make a new empty OctoClock EEPROM handler
    octoclock_eeprom_t(void);

    /*!
     * Make a new OctoClock EEPROM handler.
     * \param transport the UDP transport to the OctoClock
     * \param proto_ver firmware protocol version
     */
    octoclock_eeprom_t(transport::udp_simple::sptr transport, uint32_t proto_ver);

    /*!
     * Write the contents of this object to the EEPROM.
     */
    void commit() const;

private:
    transport::udp_simple::sptr xport;
    uint32_t _proto_ver;
    void _load();
    void _store() const;

};

} //namespace
} //namespace

#endif /* INCLUDED_UHD_USRP_CLOCK_OCTOCLOCK_EEPROM_HPP */
