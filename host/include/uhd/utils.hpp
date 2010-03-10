//
// Copyright 2010 Ettus Research LLC
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

#ifndef INCLUDED_UHD_UTILS_HPP
#define INCLUDED_UHD_UTILS_HPP

#include <stdexcept>
#include <algorithm>
#include <boost/format.hpp>
#include <boost/current_function.hpp>

/*!
 * Useful templated functions and classes that I like to pretend are part of stl
 */
namespace std{

    class assert_error : public std::logic_error{
    public:
        explicit assert_error(const string& what_arg) : logic_error(what_arg){
            /* NOP */
        }
    };

    #define ASSERT_THROW(_x) if (not (_x)) { \
        throw std::assert_error(str(boost::format( \
            "Assertion Failed:\n  %s:%d\n  %s\n  __/ %s __/" \
        ) % __FILE__ % __LINE__ % BOOST_CURRENT_FUNCTION % std::string(#_x))); \
    }

    template<class T, class InputIterator, class Function>
    T reduce(InputIterator first, InputIterator last, Function fcn, T init = 0){
        T tmp = init;
        for ( ; first != last; ++first ){
            tmp = fcn(tmp, *first);
        }
        return tmp;
    }

    template<class T, class InputIterator>
    bool has(InputIterator first, InputIterator last, const T &elem){
        return last != std::find(first, last, elem);
    }

    template<class T, class Iterable>
    bool has(const Iterable &iterable, const T &elem){
        return has(iterable.begin(), iterable.end(), elem);
    }

    template<class T>
    T sum(const T &a, const T &b){
        return a + b;
    }

    template<typename T> T signum(T n){
        if (n < 0) return -1;
        if (n > 0) return 1;
        return 0;
    }

}//namespace std

#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

namespace uhd{

    /*!
     * Check that an element is found in a container.
     * If not, throw a meaningful assertion error.
     * The "what" in the error will show what is
     * being set and a list of known good values.
     *
     * \param iterable a list of possible settings
     * \param elem an element that may be in the list
     * \param what a description of what is being set
     * \throw assertion_error when elem not in list
     */
    template<class T, class Iterable> void assert_has(
        const Iterable &iterable,
        const T &elem,
        const std::string &what = "unknown"
    ){
        if (std::has(iterable, elem)) return;
        std::string possible_values = "";
        BOOST_FOREACH(T e, iterable){
            if (e != iterable.begin()[0]) possible_values += ", ";
            possible_values += boost::lexical_cast<std::string>(e);
        }
        throw std::assert_error(str(boost::format(
                "Error: %s is not a valid %s. "
                "Possible values are: [%s]."
            )
            % boost::lexical_cast<std::string>(elem)
            % what % possible_values
        ));
    }

}//namespace uhd

#endif /* INCLUDED_UHD_UTILS_HPP */
