//
// Copyright 2010-2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_TYPES_DICT_IPP
#define INCLUDED_UHD_TYPES_DICT_IPP

#include <uhd/exception.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <typeinfo>

namespace uhd{

    namespace /*anon*/{
        template<typename Key, typename Val>
        struct key_not_found: uhd::key_error{
            key_not_found(const Key &key): uhd::key_error(
                str(boost::format(
                    "key \"%s\" not found in dict(%s, %s)"
                    ) % boost::lexical_cast<std::string>(key)
                    % typeid(Key).name() % typeid(Val).name()
                )
            ){
                /* NOP */
            }
        };
    } // namespace /*anon*/

    template <typename Key, typename Val>
    dict<Key, Val>::dict(void){
        /* NOP */
    }

    template <typename Key, typename Val> template <typename InputIterator>
    dict<Key, Val>::dict(InputIterator first, InputIterator last):
        _map(first, last)
    {
        /* NOP */
    }

    template <typename Key, typename Val>
    std::size_t dict<Key, Val>::size(void) const{
        return _map.size();
    }

    template <typename Key, typename Val>
    std::vector<Key> dict<Key, Val>::keys(void) const{
        std::vector<Key> keys;
        BOOST_FOREACH(const pair_t &p, _map){
            keys.push_back(p.first);
        }
        return keys;
    }

    template <typename Key, typename Val>
    std::vector<Val> dict<Key, Val>::vals(void) const{
        std::vector<Val> vals;
        BOOST_FOREACH(const pair_t &p, _map){
            vals.push_back(p.second);
        }
        return vals;
    }

    template <typename Key, typename Val>
    bool dict<Key, Val>::has_key(const Key &key) const{
        BOOST_FOREACH(const pair_t &p, _map){
            if (p.first == key) return true;
        }
        return false;
    }

    template <typename Key, typename Val>
    const Val &dict<Key, Val>::get(const Key &key, const Val &other) const{
        BOOST_FOREACH(const pair_t &p, _map){
            if (p.first == key) return p.second;
        }
        return other;
    }

    template <typename Key, typename Val>
    const Val &dict<Key, Val>::get(const Key &key) const{
        BOOST_FOREACH(const pair_t &p, _map){
            if (p.first == key) return p.second;
        }
        throw key_not_found<Key, Val>(key);
    }

    template <typename Key, typename Val>
    void dict<Key, Val>::set(const Key &key, const Val &val){
        (*this)[key] = val;
    }

    template <typename Key, typename Val>
    const Val &dict<Key, Val>::operator[](const Key &key) const{
        BOOST_FOREACH(const pair_t &p, _map){
            if (p.first == key) return p.second;
        }
        throw key_not_found<Key, Val>(key);
    }

    template <typename Key, typename Val>
    Val &dict<Key, Val>::operator[](const Key &key){
        BOOST_FOREACH(pair_t &p, _map){
            if (p.first == key) return p.second;
        }
        _map.push_back(std::make_pair(key, Val()));
        return _map.back().second;
    }

    template <typename Key, typename Val>
    bool dict<Key, Val>::operator==(const dict<Key, Val> &other) const{
        if (this->size() != other.size()){
            return false;
        }
        BOOST_FOREACH(const pair_t& p, _map) {
            if (not (other.has_key(p.first) and other.get(p.first) == p.second)){
                return false;
            }
        }
        return true;
    }

    template <typename Key, typename Val>
    bool dict<Key, Val>::operator!=(const dict<Key, Val> &other) const{
        return not (*this == other);
    }

    template <typename Key, typename Val>
    Val dict<Key, Val>::pop(const Key &key){
        typename std::list<pair_t>::iterator it;
        for (it = _map.begin(); it != _map.end(); it++){
            if (it->first == key){
                Val val = it->second;
                _map.erase(it);
                return val;
            }
        }
        throw key_not_found<Key, Val>(key);
    }

    template <typename Key, typename Val>
    void dict<Key, Val>::update(const dict<Key, Val> &new_dict, bool fail_on_conflict)
    {
        BOOST_FOREACH(const Key &key, new_dict.keys()) {
            if (fail_on_conflict and has_key(key) and get(key) != new_dict[key]) {
                throw uhd::value_error(str(
                    boost::format("Option merge conflict: %s:%s != %s:%s")
                    % key % get(key) % key % new_dict[key]
                ));
            }
            set(key, new_dict[key]);
        }
    }

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_DICT_IPP */
