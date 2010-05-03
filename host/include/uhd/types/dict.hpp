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

#ifndef INCLUDED_UHD_TYPES_DICT_HPP
#define INCLUDED_UHD_TYPES_DICT_HPP

#include <uhd/config.hpp>
#include <boost/foreach.hpp>
#include <stdexcept>
#include <vector>
#include <list>

namespace uhd{

    /*!
     * A templated dictionary class with a python-like interface.
     */
    template <typename Key, typename Val> class dict{
    public:
        typedef std::pair<Key, Val> pair_t;

        /*!
         * Create a new empty dictionary.
         */
        dict(void){
            /* NOP */
        }

        /*!
         * Input iterator constructor:
         * Makes boost::assign::map_list_of work.
         * \param first the begin iterator
         * \param last the end iterator
         */
        template <class InputIterator>
        dict(InputIterator first, InputIterator last){
            for(InputIterator it = first; it != last; it++){
                _map.push_back(*it);
            }
        }

        /*!
         * Destroy this dict.
         */
        ~dict(void){
            /* NOP */
        }

        /*!
         * Get the number of elements in this dict.
         * \return the number of elements
         */
        std::size_t size(void) const{
            return _map.size();
        }

        /*!
         * Get a list of the keys in this dict.
         * Key order depends on insertion precedence.
         * \return vector of keys
         */
        const std::vector<Key> keys(void) const{
            std::vector<Key> keys;
            BOOST_FOREACH(const pair_t &p, _map){
                keys.push_back(p.first);
            }
            return keys;
        }

        /*!
         * Get a list of the values in this dict.
         * Value order depends on insertion precedence.
         * \return vector of values
         */
        const std::vector<Val> vals(void) const{
            std::vector<Val> vals;
            BOOST_FOREACH(const pair_t &p, _map){
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
            BOOST_FOREACH(const pair_t &p, _map){
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
            BOOST_FOREACH(const pair_t &p, _map){
                if (p.first == key) return p.second;
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
            BOOST_FOREACH(pair_t &p, _map){
                if (p.first == key) return p.second;
            }
            _map.push_back(std::make_pair(key, Val()));
            return _map.back().second;
        }

        /*!
         * Pop an item out of the dictionary.
         * \param key the item key
         * \return the value of the item
         * \throw an exception when not found
         */
        Val pop(const Key &key){
            Val val = (*this)[key];
            _map.remove(pair_t(key, val));
            return val;
        }

    private:
        std::list<pair_t> _map; //private container
    };

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_DICT_HPP */
