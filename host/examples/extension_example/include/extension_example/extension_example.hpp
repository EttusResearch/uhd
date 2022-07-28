//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/extension/extension.hpp>
#include <uhd/rfnoc/radio_control.hpp>
#include <uhd/rfnoc/rf_control/core_iface.hpp>
#include <uhd/utils/noncopyable.hpp>
#include <memory>

namespace ext_example {

/*! Interface for Extension Example
 *
 * This interface contains all methods related directly to the Extension.
 */
class extension_example : public uhd::extension::extension
{
public:
    using sptr = std::shared_ptr<extension_example>;

    virtual ~extension_example() = default;

    /*!
     * This is a method that is directly related to this extension and not inherited from
     * anywhere. Therefore it needs to be defined here.
     */
    virtual void write_log(std::string& text) = 0;

    /*!
     * Make an example extension which is plugged into the given radio block.
     *
     * \param fargs Factory args that hold pointers to radio control and motherboard
     * controller
     * \returns Smart pointer to extension_example class object
     */
    static sptr make(uhd::extension::extension::factory_args fargs);
};

} // namespace ext_example
