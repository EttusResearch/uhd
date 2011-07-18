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
#include <boost/make_shared.hpp>
#include <iostream>

class property_tree_impl : public uhd::property_tree{
public:

    property_tree_impl(const path_type &root = path_type()):
        _root(root)
    {
        _guts = boost::make_shared<tree_guts_type>();
    }

    sptr subtree(const path_type &path_) const{
        const path_type path = _root / path_;
        boost::mutex::scoped_lock lock(_guts->mutex);

        property_tree_impl *subtree = new property_tree_impl(path);
        subtree->_guts = this->_guts; //copy the guts sptr
        return sptr(subtree);
    }

    void remove(const path_type &path_){
        const path_type path = _root / path_;
        boost::mutex::scoped_lock lock(_guts->mutex);

        node_type *parent = NULL;
        node_type *node = &_guts->root;
        BOOST_FOREACH(const path_type &branch, path){
            const std::string name = branch.string();
            if (not node->has_key(name)) throw_path_not_found(path);
            parent = node;
            node = &(*node)[name];
        }
        if (parent == NULL) throw uhd::runtime_error("Cannot uproot");
        parent->pop(path_type(path.leaf()).string());
    }

    bool exists(const path_type &path_) const{
        const path_type path = _root / path_;
        boost::mutex::scoped_lock lock(_guts->mutex);

        node_type *node = &_guts->root;
        BOOST_FOREACH(const path_type &branch, path){
            const std::string name = branch.string();
            if (not node->has_key(name)) return false;
            node = &(*node)[name];
        }
        return true;
    }

    std::vector<std::string> list(const path_type &path_) const{
        const path_type path = _root / path_;
        boost::mutex::scoped_lock lock(_guts->mutex);

        node_type *node = &_guts->root;
        BOOST_FOREACH(const path_type &branch, path){
            const std::string name = branch.string();
            if (not node->has_key(name)) throw_path_not_found(path);
            node = &(*node)[name];
        }

        return node->keys();
    }

    void _create(const path_type &path_, const boost::shared_ptr<void> &prop){
        const path_type path = _root / path_;
        boost::mutex::scoped_lock lock(_guts->mutex);

        node_type *node = &_guts->root;
        BOOST_FOREACH(const path_type &branch, path){
            const std::string name = branch.string();
            if (not node->has_key(name)) (*node)[name] = node_type();
            node = &(*node)[name];
        }
        if (node->prop.get() != NULL) throw uhd::runtime_error("Cannot create! Property already exists at: " + path.string());
        node->prop = prop;
    }

    boost::shared_ptr<void> &_access(const path_type &path_) const{
        const path_type path = _root / path_;
        boost::mutex::scoped_lock lock(_guts->mutex);

        node_type *node = &_guts->root;
        BOOST_FOREACH(const path_type &branch, path){
            const std::string name = branch.string();
            if (not node->has_key(name)) throw_path_not_found(path);
            node = &(*node)[name];
        }
        if (node->prop.get() == NULL) throw uhd::runtime_error("Cannot access! Property uninitialized at: " + path.string());
        return node->prop;
    }

private:
    void throw_path_not_found(const path_type &path) const{
        throw uhd::lookup_error("Path not found in tree: " + path.string());
    }

    //basic structural node element
    struct node_type : uhd::dict<std::string, node_type>{
        boost::shared_ptr<void> prop;
    };

    //tree guts which may be referenced in a subtree
    struct tree_guts_type{
        node_type root;
        boost::mutex mutex;
    };

    //members, the tree and root prefix
    boost::shared_ptr<tree_guts_type> _guts;
    const path_type _root;
};

uhd::property_tree::sptr uhd::property_tree::make(void){
    return sptr(new property_tree_impl());
}
