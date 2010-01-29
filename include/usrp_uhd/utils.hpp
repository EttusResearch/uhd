//
// Copyright 2010 Ettus Research LLC
//

#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/function.hpp>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <map>

#ifndef INCLUDED_USRP_UHD_UTILS_HPP
#define INCLUDED_USRP_UHD_UTILS_HPP

namespace usrp_uhd{

template <class Key, class T> //TODO template this better
std::vector<Key> get_map_keys(const std::map<Key, T> &m){
    std::vector<Key> v;
    std::pair<Key, T> p;
    BOOST_FOREACH(p, m){
        v.push_back(p.first);
    }
    return v;
}

} //namespace usrp_uhd

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
        throw std::assert_error("Assertion Failed: " + std::string(#_x)); \
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

    template <class T>
    T sum(const T &a, const T &b){
        return a + b;
    }

}//namespace std

#endif /* INCLUDED_USRP_UHD_UTILS_HPP */
