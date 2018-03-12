//
// Copyright 2011-2013,2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_TYPES_WB_IFACE_HPP
#define INCLUDED_UHD_TYPES_WB_IFACE_HPP

#include <uhd/config.hpp>
#include <uhd/types/time_spec.hpp>
#include <stdint.h>
#include <boost/shared_ptr.hpp>

namespace uhd
{

class UHD_API wb_iface
{
public:
    typedef boost::shared_ptr<wb_iface> sptr;
    typedef uint32_t wb_addr_type;

    virtual ~wb_iface(void);

    /*!
     * Write a register (64 bits)
     * \param addr the address
     * \param data the 64bit data
     */
    virtual void poke64(const wb_addr_type addr, const uint64_t data);

    /*!
     * Read a register (64 bits)
     * \param addr the address
     * \return the 64bit data
     */
    virtual uint64_t peek64(const wb_addr_type addr);

    /*!
     * Write a register (32 bits)
     * \param addr the address
     * \param data the 32bit data
     */
    virtual void poke32(const wb_addr_type addr, const uint32_t data);

    /*!
     * Read a register (32 bits)
     * \param addr the address
     * \return the 32bit data
     */
    virtual uint32_t peek32(const wb_addr_type addr);

    /*!
     * Write a register (16 bits)
     * \param addr the address
     * \param data the 16bit data
     */
    virtual void poke16(const wb_addr_type addr, const uint16_t data);

    /*!
     * Read a register (16 bits)
     * \param addr the address
     * \return the 16bit data
     */
    virtual uint16_t peek16(const wb_addr_type addr);
};

class UHD_API timed_wb_iface : public wb_iface
{
public:
    typedef boost::shared_ptr<timed_wb_iface> sptr;

    /*!
     * Get the command time.
     * \return the command time
     */
    virtual time_spec_t get_time(void) = 0;

    /*!
     * Set the command time.
     * \param t the command time
     */
    virtual void set_time(const time_spec_t& t) = 0;
};

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_WB_IFACE_HPP */
