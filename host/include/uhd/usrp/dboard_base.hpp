//
// Copyright 2010,2017 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/usrp/dboard_eeprom.hpp>
#include <uhd/usrp/dboard_id.hpp>
#include <uhd/usrp/dboard_iface.hpp>
#include <uhd/utils/noncopyable.hpp>
#include <memory>

namespace uhd { namespace usrp {

/*!
 * A daughter board dboard_base class for all dboards.
 * Only other dboard dboard_base classes should inherit this.
 */
class UHD_API dboard_base : uhd::noncopyable
{
public:
    typedef std::shared_ptr<dboard_base> sptr;
    /*!
     * An opaque type for the dboard constructor args.
     * Derived classes should pass the args into the base class,
     * but should not deal with the internals of the args.
     */
    typedef void* ctor_args_t;

    // structors
    dboard_base(ctor_args_t);
    virtual ~dboard_base();

    // post-construction initializer
    virtual void initialize() {}

protected:
    std::string get_subdev_name(void);
    dboard_iface::sptr get_iface(void);
    dboard_id_t get_rx_id(void);
    dboard_id_t get_tx_id(void);
    dboard_eeprom_t get_rx_eeprom(void);
    dboard_eeprom_t get_tx_eeprom(void);
    property_tree::sptr get_rx_subtree(void);
    property_tree::sptr get_tx_subtree(void);

private:
    struct impl;
    std::unique_ptr<impl> _impl;
};

/*!
 * A xcvr daughter board implements rx and tx methods
 * Sub classes for xcvr boards should inherit this.
 */
class UHD_API xcvr_dboard_base : public dboard_base
{
public:
    /*!
     * Create a new xcvr dboard object, override in subclasses.
     */
    xcvr_dboard_base(ctor_args_t);
    ~xcvr_dboard_base() override {}
};

/*!
 * A rx daughter board only implements rx methods.
 * Sub classes for rx-only boards should inherit this.
 */
class UHD_API rx_dboard_base : public dboard_base
{
public:
    /*!
     * Create a new rx dboard object, override in subclasses.
     */
    rx_dboard_base(ctor_args_t);
    ~rx_dboard_base() override {}
};

/*!
 * A tx daughter board only implements tx methods.
 * Sub classes for rx-only boards should inherit this.
 */
class UHD_API tx_dboard_base : public dboard_base
{
public:
    /*!
     * Create a new rx dboard object, override in subclasses.
     */
    tx_dboard_base(ctor_args_t);
    ~tx_dboard_base() override {}
};

}} // namespace uhd::usrp
