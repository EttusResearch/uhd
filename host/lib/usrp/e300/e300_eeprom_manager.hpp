//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_E300_EEPROM_MANAGER_HPP
#define INCLUDED_E300_EEPROM_MANAGER_HPP

#include <boost/thread/mutex.hpp>
#include <boost/shared_ptr.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>
#include <uhd/usrp/dboard_eeprom.hpp>

#include "e300_i2c.hpp"

namespace uhd { namespace usrp { namespace e300 {

static const uint16_t E300_MB_PID = 0x77d1;
static const uint16_t E310_SG1_MB_PID = 0x77d2;
static const uint16_t E310_SG3_MB_PID = 0x77d3;

static const uint16_t E300_DB_PID = 0x0100;
static const uint16_t E310_DB_PID = 0x0110;

static const uint16_t E310_MB_MAP_MAJOR = 0x0001;
static const uint16_t E310_MB_MAP_MINOR = 0x0000;

static const uint16_t E310_DB_MAP_MAJOR = 0x0001;
static const uint16_t E310_DB_MAP_MINOR = 0x0000;

class e300_eeprom_manager : boost::noncopyable
{
public:
    typedef boost::shared_ptr<e300_eeprom_manager> sptr;
    e300_eeprom_manager(i2c::sptr i2c);
    ~e300_eeprom_manager();

    // mboard
    const mboard_eeprom_t& read_mb_eeprom();
    void write_mb_eeprom(const mboard_eeprom_t& eeprom);

    UHD_INLINE const mboard_eeprom_t& get_mb_eeprom()
    {
        return _mb_eeprom;
    }

    // dboard
    const dboard_eeprom_t& read_db_eeprom();
    void write_db_eeprom(const dboard_eeprom_t& eeprom);

    UHD_INLINE const dboard_eeprom_t& get_db_eeprom()
    {
        return _db_eeprom;
    }


    i2c::sptr get_i2c_sptr(void);

    enum mboard_t {USRP_E300_MB, USRP_E310_SG1_MB, USRP_E310_SG3_MB, UNKNOWN};

    mboard_t get_mb_type(void) const;
    static mboard_t get_mb_type(uint16_t pid);
    std::string get_mb_type_string(void) const;

private: // types
    const static size_t MB_SERIAL_LEN = 8;
    const static size_t MB_NAME_LEN   = 32;
    const static size_t MB_ADDR       = 0x51;

    const static size_t DB_SERIAL_LEN = 8;
    const static size_t DB_ADDR       = 0x50;

    struct mb_eeprom_map_t
    {
        // Data format version
        uint16_t data_version_major;
        uint16_t data_version_minor;

        // NIC mac address
        uint8_t mac_addr[6];

        // HW identification info
        uint16_t hw_product;
        uint16_t hw_revision;

        // serial
        uint8_t serial[MB_SERIAL_LEN];
        uint8_t pad[20 - MB_SERIAL_LEN];

        //User specific
        uint8_t user_name[MB_NAME_LEN];
    };

    struct db_eeprom_map_t
    {
        // Data format version
        uint16_t data_version_major;
        uint16_t data_version_minor;

        // HW identification info
        uint16_t hw_product;
        uint16_t hw_revision;

        // serial
        uint8_t serial[DB_SERIAL_LEN];
        uint8_t pad[20 - DB_SERIAL_LEN];
    };

private: // members
    mboard_eeprom_t                         _mb_eeprom;
    dboard_eeprom_t                         _db_eeprom;
    i2c::sptr                               _i2c;

    boost::mutex    _mutex;
};

}}} //namespace

#endif // INCLUDED_E300_EEPROM_MANAGER_HPP
