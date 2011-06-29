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

#include <uhd/property_tree.hpp>
#include <uhd/types/dict.hpp>
#include <boost/foreach.hpp>
#include <boost/thread/mutex.hpp>
#include <iostream>

class property_tree_impl : public uhd::property_tree{
public:

    void remove(const path_type &path){
        boost::mutex::scoped_lock lock(_mutex);

        node_type *parent = NULL;
        node_type *node = &_root;
        BOOST_FOREACH(const std::string &leaf, path){
            if (not node->has_key(leaf)) throw_path_not_found(path);
            parent = node;
            node = &(*node)[leaf];
        }
        if (parent == NULL) throw uhd::runtime_error("Cannot uproot");
        parent->pop(path.leaf());
    }

    bool exists(const path_type &path){
        boost::mutex::scoped_lock lock(_mutex);

        node_type *node = &_root;
        BOOST_FOREACH(const std::string &leaf, path){
            if (not node->has_key(leaf)) return false;
            node = &(*node)[leaf];
        }
        return true;
    }

    std::vector<std::string> list(const path_type &path){
        boost::mutex::scoped_lock lock(_mutex);

        node_type *node = &_root;
        BOOST_FOREACH(const std::string &leaf, path){
            if (not node->has_key(leaf)) throw_path_not_found(path);
            node = &(*node)[leaf];
        }

        return node->keys();
    }

    void _create(const path_type &path, const boost::shared_ptr<void> &prop){
        boost::mutex::scoped_lock lock(_mutex);

        node_type *node = &_root;
        BOOST_FOREACH(const std::string &leaf, path){
            if (not node->has_key(leaf)) (*node)[leaf] = node_type();
            node = &(*node)[leaf];
        }
        node->prop = prop;
    }

    boost::shared_ptr<void> &_access(const path_type &path){
        boost::mutex::scoped_lock lock(_mutex);

        node_type *node = &_root;
        BOOST_FOREACH(const std::string &leaf, path){
            if (not node->has_key(leaf)) throw_path_not_found(path);
            node = &(*node)[leaf];
        }
        if (node->prop.get() == NULL) throw uhd::type_error("Uninitialized property at: " + path.string());
        return node->prop;
    }

private:
    void throw_path_not_found(const path_type &path){
        throw uhd::lookup_error("Path not found in tree: " + path.string());
    }

    struct node_type : uhd::dict<std::string, node_type>{
        boost::shared_ptr<void> prop;
    } _root;

    boost::mutex _mutex;
};

uhd::property_tree::sptr uhd::property_tree::make(void){
    return sptr(new property_tree_impl());
}
