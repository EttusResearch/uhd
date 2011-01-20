//
// Copyright 2011 Ettus Research LLC
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

#ifndef INCLUDED_UHD_CONVERT_HPP
#define INCLUDED_UHD_CONVERT_HPP

#include <uhd/config.hpp>
#include <uhd/types/io_type.hpp>
#include <uhd/types/otw_type.hpp>
#include <boost/function.hpp>
#include <string>
#include <vector>

namespace uhd{ namespace convert{

    typedef std::vector<void *> output_type;
    typedef std::vector<const void *> input_type;
    typedef boost::function<void(input_type&, output_type&, size_t)> function_type;

    /*!
     * Describe the priority of a converter function.
     * A higher priority function takes precedence.
     * The general case function are the lowest.
     * Next comes the liborc implementations.
     * Custom intrinsics implementations are highest.
     */
    enum priority_type{
        PRIORITY_GENERAL = 0,
        PRIORITY_LIBORC = 1,
        PRIORITY_CUSTOM = 2,
        PRIORITY_EMPTY = -1,
    };

    /*!
     * Register a converter function that converts cpu type to/from otw type.
     * \param markup representing the signature
     * \param fcn a pointer to the converter
     * \param prio the function priority
     */
    UHD_API void register_converter(
        const std::string &markup,
        function_type fcn,
        priority_type prio
    );

    /*!
     * Get a converter function that converts cpu to otw.
     * \param io_type the type of the input samples
     * \param otw_type the type of the output samples
     * \param num_input_buffs the number of inputs
     * \param num_output_buffs the number of outputs
     */
    UHD_API const function_type &get_converter_cpu_to_otw(
        const io_type_t &io_type,
        const otw_type_t &otw_type,
        size_t num_input_buffs,
        size_t num_output_buffs
    );

    /*!
     * Get a converter function that converts otw to cpu.
     * \param io_type the type of the input samples
     * \param otw_type the type of the output samples
     * \param num_input_buffs the number of inputs
     * \param num_output_buffs the number of outputs
     */
    UHD_API const function_type &get_converter_otw_to_cpu(
        const io_type_t &io_type,
        const otw_type_t &otw_type,
        size_t num_input_buffs,
        size_t num_output_buffs
    );

}} //namespace

#endif /* INCLUDED_UHD_CONVERT_HPP */
