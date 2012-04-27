//
// Copyright 2011-2012 Ettus Research LLC
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
#include <uhd/types/ref_vector.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/operators.hpp>
#include <string>

namespace uhd{ namespace convert{

    //! A conversion class that implements a conversion from inputs -> outputs.
    class converter{
    public:
        typedef boost::shared_ptr<converter> sptr;
        typedef uhd::ref_vector<void *> output_type;
        typedef uhd::ref_vector<const void *> input_type;

        //! Set the scale factor (used in floating point conversions)
        virtual void set_scalar(const double) = 0;

        //! The public conversion method to convert inputs -> outputs
        UHD_INLINE void conv(const input_type &in, const output_type &out, const size_t num){
            if (num != 0) (*this)(in, out, num);
        }

    private:
        //! Callable method: input vectors, output vectors, num samples
        virtual void operator()(const input_type&, const output_type&, const size_t) = 0;
    };

    //! Conversion factory function typedef
    typedef boost::function<converter::sptr(void)> function_type;

    //! Priority of conversion routines
    typedef int priority_type;

    //! Identify a conversion routine in the registry
    struct id_type : boost::equality_comparable<id_type>{
        std::string input_format;
        size_t num_inputs;
        std::string output_format;
        size_t num_outputs;
        std::string to_pp_string(void) const;
    };

    //! Implement equality_comparable interface
    UHD_API bool operator==(const id_type &, const id_type &);

    /*!
     * Register a converter function.
     * \param id identify the conversion
     * \param fcn makes a new converter
     * \param prio the function priority
     */
    UHD_API void register_converter(
        const id_type &id,
        const function_type &fcn,
        const priority_type prio
    );

    /*!
     * Get a converter factory function.
     * \param id identify the conversion
     * \param prio the desired prio or -1 for best
     * \return the converter factory function
     */
    UHD_API function_type get_converter(
        const id_type &id,
        const priority_type prio = -1
    );

    /*!
     * Register the size of a particular item.
     * \param format the item format
     * \param size the size in bytes
     */
    UHD_API void register_bytes_per_item(
        const std::string &format, const size_t size
    );

    //! Convert an item format to a size in bytes
    UHD_API size_t get_bytes_per_item(const std::string &format);

}} //namespace

#endif /* INCLUDED_UHD_CONVERT_HPP */
