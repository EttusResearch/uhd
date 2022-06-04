//
// Copyright 2014-2017 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/utils/noncopyable.hpp>
#include <functional>
#include <string>

namespace uhd {

class UHD_API image_loader : uhd::noncopyable
{
public:
    struct image_loader_args_t
    {
        uhd::device_addr_t args;
        bool load_firmware;
        bool load_fpga;
        bool download;
        std::string firmware_path;
        std::string fpga_path;
        std::string out_path;
        std::string id;
        std::vector<uint8_t> component;
        uhd::dict<std::string, std::string> metadata;
        bool delay_reload = false;
        bool just_reload  = false;
    };

    //! Signature of an image loading function
    /*!
     * This is the function signature for an image loading function.
     * See the declaration of load() for the meaning of these arguments.
     *
     * This function must return true upon the end of a successful image load
     * or false if no applicable device was found. It may only throw a runtime
     * error under one of three conditions:
     *
     *  * The function finds multiple devices that fit the user's arguments.
     *  * The function has already engaged with a specific device and
     *    something goes wrong.
     *  * The user gives arguments that unambiguously lead to a specific
     *    device and expect the default image(s) to be loaded, but the specific
     *    model of the device cannot be determined beyond a category.
     */
    typedef std::function<bool(const image_loader_args_t&)> loader_fcn_t;

    //! Register an image loader
    /*!
     * \param device_type the "type=foo" value given in an --args option
     * \param loader_fcn  the loader function for the given device
     * \param recovery_instructions instructions on how to restore a device
     */
    static void register_image_loader(const std::string& device_type,
        const loader_fcn_t& loader_fcn,
        const std::string& recovery_instructions);

    //! Load firmware and/or FPGA onto a device
    /*!
     * \param image_loader_args arguments to pass into image loading function
     */
    static bool load(const image_loader_args_t& image_loader_args);

    //! Get the instructions on how to recovery a particular device
    /*!
     * These instructions should be queried if the user interrupts an image loading
     * session, as this will likely leave the device in an unstable state.
     * \param device_type the "type=foo" value given in an --args option
     * \return recovery instructions
     */
    static std::string get_recovery_instructions(const std::string& device_type);
};

} // namespace uhd
