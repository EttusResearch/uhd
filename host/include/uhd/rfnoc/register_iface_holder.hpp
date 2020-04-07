//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/rfnoc/register_iface.hpp>

namespace uhd { namespace rfnoc {

/*! Register interface holder class
 *
 * Classes derived from this class have access to a uhd::rfnoc::register_iface
 * object.
 */
class register_iface_holder
{
public:
    register_iface_holder(register_iface::sptr reg) : _reg(reg){};
    virtual ~register_iface_holder() = default;

    /*! Return the register interface to access low-level registers
     *
     * \return iface A reference to an interface for low-level register access
     */
    register_iface& regs()
    {
        return *(_reg.get());
    };

protected:
    void update_reg_iface(register_iface::sptr new_iface = nullptr);

private:
    register_iface::sptr _reg;
};

}} /* namespace uhd::rfnoc */
