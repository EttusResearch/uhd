//
// Copyright 2013-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_N230_EEPROM_MANAGER_HPP
#define INCLUDED_N230_EEPROM_MANAGER_HPP

#include <boost/thread/mutex.hpp>
#include <uhd/transport/udp_simple.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>
#include "n230_fw_host_iface.h"

namespace uhd { namespace usrp { namespace n230 {

class n230_eeprom_manager : boost::noncopyable
{
public:
    n230_eeprom_manager(const std::string& addr);

    const mboard_eeprom_t& read_mb_eeprom();
    void write_mb_eeprom(const mboard_eeprom_t& eeprom);

    inline const mboard_eeprom_t& get_mb_eeprom() {
        return _mb_eeprom;
    }

private:    //Functions
    void _transact(const uint32_t command);
    void _flush_xport();

private:    //Members
    mboard_eeprom_t             _mb_eeprom;
    transport::udp_simple::sptr _udp_xport;
    n230_flash_prog_t           _request;
    n230_flash_prog_t           _response;
    uint32_t             _seq_num;
    boost::mutex                _mutex;

    static const double UDP_TIMEOUT_IN_SEC;
};

}}} //namespace

#endif /* INCLUDED_N230_EEPROM_MANAGER_HPP */
