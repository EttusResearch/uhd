//
// Copyright 2010-2011 Ettus Research LLC
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
#include <vector>
#include <list>

namespace uhd{

    /*!
     * A templated dictionary class with a python-like interface.
     */
    template <typename Key, typename Val> class dict{
    public:
        /*!
         * Create a new empty dictionary.
         */
        dict(void);

        /*!
         * Input iterator constructor:
         * Makes boost::assign::map_list_of work.
         * \param first the begin iterator
         * \param last the end iterator
         */
        template <typename InputIterator>
        dict(InputIterator first, InputIterator last);

        /*!
         * Get the number of elements in this dict.
         * \return the number of elements
         */
        std::size_t size(void) const;

        /*!
         * Get a list of the keys in this dict.
         * Key order depends on insertion precedence.
         * \return vector of keys
         */
        std::vector<Key> keys(void) const;

        /*!
         * Get a list of the values in this dict.
         * Value order depends on insertion precedence.
         * \return vector of values
         */
        std::vector<Val> vals(void) const;

        /*!
         * Does the dictionary contain this key?
         * \param key the key to look for
         * \return true if found
         */
        bool has_key(const Key &key) const;

        /*!
         * Get a value in the dict or default.
         * \param key the key to look for
         * \param other use if key not found
         * \return the value or default
         */
        const Val &get(const Key &key, const Val &other) const;

        /*!
         * Get a value in the dict or throw.
         * \param key the key to look for
         * \return the value or default
         */
        const Val &get(const Key &key) const;

        /*!
         * Set a value in the dict at the key.
         * \param key the key to set at
         * \param val the value to set
         */
        void set(const Key &key, const Val &val);

        /*!
         * Get a value for the given key if it exists.
         * If the key is not found throw an error.
         * \param key the key to look for
         * \return the value at the key
         * \throw an exception when not found
         */
        const Val &operator[](const Key &key) const;

        /*!
         * Set a value for the given key, however, in reality
         * it really returns a reference which can be assigned to.
         * \param key the key to set to
         * \return a reference to the value
         */
        Val &operator[](const Key &key);

        /*!
         * Pop an item out of the dictionary.
         * \param key the item key
         * \return the value of the item
         * \throw an exception when not found
         */
        Val pop(const Key &key);

    private:
        typedef std::pair<Key, Val> pair_t;
        std::list<pair_t> _map; //private container
    };

} //namespace uhd

#include <uhd/types/dict.ipp>

#endif /* INCLUDED_UHD_TYPES_DICT_HPP */
