//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/rfnoc/mb_controller.hpp>
#include <uhd/rfnoc/radio_control.hpp>
#include <uhd/rfnoc/rf_control/core_iface.hpp>
#include <uhd/utils/noncopyable.hpp>
#include <uhd/utils/static.hpp>
#include <functional>
#include <memory>
#include <string>


namespace uhd { namespace extension {

/*!
 * Common base class for RF extensions
 */
class UHD_API extension : public uhd::noncopyable,
                          virtual public uhd::rfnoc::rf_control::core_iface,
                          public uhd::rfnoc::rf_control::power_reference_iface
{
public:
    /*!
     * Arguments to be passed to the extension
     */
    struct factory_args
    {
        uhd::rfnoc::radio_control::sptr radio_ctrl;
        uhd::rfnoc::mb_controller::sptr mb_ctrl;
    };

    using sptr         = std::shared_ptr<extension>;
    using factory_type = std::function<sptr(factory_args)>;

    virtual ~extension() = default;

    virtual std::string get_name() = 0;

    static void register_extension(
        const std::string& extension_name, extension::factory_type factory_fn);
};


}} // namespace uhd::extension

/*!
 * Function to be called by the extension to register in the extension framework
 *
 * \param NAME Key under which the extension gets registered
 * \param CLASS_NAME Name of the class that holds the make function of the extension
 */
#define UHD_REGISTER_EXTENSION(NAME, CLASS_NAME)                                \
    UHD_STATIC_BLOCK(register_extension_##NAME)                                 \
    {                                                                           \
        uhd::extension::extension::register_extension(#NAME, CLASS_NAME::make); \
    }
