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

#ifndef INCLUDED_UHD_DICT_HPP
#define INCLUDED_UHD_DICT_HPP

#include <map>
#include <vector>
#include <boost/foreach.hpp>
#include <stdexcept>

namespace uhd{

    /*!
     * A templated dictionary class with a python-like interface.
     * Its wraps around a std::map internally.
     */
    template <class Key, class Val> class dict{
    public:
        /*!
         * Create a new empty dictionary.
         */
        dict(void){
            /* NOP */
        }

        /*!
         * Create a dictionary from a map.
         * \param map a map with key value pairs
         */
        dict(const std::map<Key, Val> &map){
            _map = map;
        }

        /*!
         * Destroy this dict.
         */
        ~dict(void){
            /* NOP */
        }

        /*!
         * Get a list of the keys in this dict.
         * \return vector of keys
         */
        std::vector<Key> get_keys(void) const{
            std::vector<Key> keys;
            std::pair<Key, Val> p;
            BOOST_FOREACH(p, _map){
                keys.push_back(p.first);
            }
            return keys;
        }

        /*!
         * Get a list of the values in this dict.
         * \return vector of values
         */
        std::vector<Val> get_vals(void) const{
            std::vector<Val> vals;
            std::pair<Key, Val> p;
            BOOST_FOREACH(p, _map){
                vals.push_back(p.second);
            }
            return vals;
        }

        /*!
         * Does the dictionary contain this key?
         * \param key the key to look for
         * \return true if found
         */
        bool has_key(const Key &key) const{
            std::pair<Key, Val> p;
            BOOST_FOREACH(p, _map){
                if (p.first == key) return true;
            }
            return false;
        }

        /*!
         * Get a value for the given key if it exists.
         * If the key is not found throw an error.
         * \param key the key to look for
         * \return the value at the key
         * \throw an exception when not found
         */
        const Val &operator[](const Key &key) const{
            if (has_key(key)){
                return _map.find(key)->second;
            }
            throw std::invalid_argument("key not found in dict");
        }

        /*!
         * Set a value for the given key, however, in reality
         * it really returns a reference which can be assigned to.
         * \param key the key to set to
         * \return a reference to the value
         */
        Val &operator[](const Key &key){
            return _map[key];
        }

        /*!
         * Pop an item out of the dictionary.
         * \param key the item key
         * \return the value of the item
         * \throw an exception when not found
         */
        Val pop_key(const Key &key){
            if (has_key(key)){
                Val val = _map.find(key)->second;
                _map.erase(key);
                return val;
            }
            throw std::invalid_argument("key not found in dict");
        }

    private:
        std::map<Key, Val> _map; //private container
    };

} //namespace uhd

#endif /* INCLUDED_UHD_DICT_HPP */
