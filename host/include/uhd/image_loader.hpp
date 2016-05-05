//
// Copyright 2014-2015 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef INCLUDED_UHD_IMAGE_LOADER_HPP
#define INCLUDED_UHD_IMAGE_LOADER_HPP

#include <string>

#include <boost/function.hpp>

#include <uhd/config.hpp>
#include <uhd/types/device_addr.hpp>

namespace uhd{

class UHD_API image_loader : boost::noncopyable{

public:

    typedef struct{
        uhd::device_addr_t args;
        bool load_firmware;
        bool load_fpga;
        std::string firmware_path;
        std::string fpga_path;
    } image_loader_args_t;

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
    typedef boost::function<bool(const image_loader_args_t &)> loader_fcn_t;

    //! Register an image loader
    /*!
     * \param device_type the "type=foo" value given in an --args option
     * \param loader_fcn  the loader function for the given device
     * \param recovery_instructions instructions on how to restore a device
     */
    static void register_image_loader(
        const std::string &device_type,
        const loader_fcn_t &loader_fcn,
        const std::string &recovery_instructions
    );

    //! Load firmware and/or FPGA onto a device
    /*!
     * \param image_loader_args arguments to pass into image loading function
     */
    static bool load(const image_loader_args_t &image_loader_args);

    //! Get the instructions on how to recovery a particular device
    /*!
     * These instructions should be queried if the user interrupts an image loading
     * session, as this will likely leave the device in an unstable state.
     * \param device_type the "type=foo" value given in an --args option
     * \return recovery instructions
     */
    static std::string get_recovery_instructions(const std::string &device_type);
};

}

#endif /* INCLUDED_UHD_IMAGE_LOADER_HPP */
