//
// Copyright 2010 Ettus Research LLC
//

#include <boost/foreach.hpp>
#include <map>
#include <vector>

#ifndef INCLUDED_USRP_UHD_UTILS_HPP
#define INCLUDED_USRP_UHD_UTILS_HPP

namespace usrp_uhd{

template <class Key, class T>
std::vector<Key> get_map_keys(const std::map<Key, T> &m){
    std::vector<Key> v;
    std::pair<Key, T> p;
    BOOST_FOREACH(p, m){
        v.push_back(p.first);
    }
    return v;
}

//TODO implement a set and get gains that takes a wx obj ptr, and gain properties

//TODO check name in vector of names

//TODO optionally extract a name from the named_prop_t

} //namespace usrp_uhd

#endif /* INCLUDED_USRP_UHD_UTILS_HPP */
