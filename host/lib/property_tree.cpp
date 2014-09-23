//
// Copyright 2011,2014 Ettus Research LLC
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

using namespace uhd;

/***********************************************************************
 * Helper function to iterate through paths
 **********************************************************************/
#include <boost/tokenizer.hpp>
#define path_tokenizer(path) \
    boost::tokenizer<boost::char_separator<char> > \
    (path, boost::char_separator<char>("/"))

/***********************************************************************
 * Property path implementation wrapper
 **********************************************************************/
fs_path::fs_path(void): std::string(){}
fs_path::fs_path(const char *p): std::string(p){}
fs_path::fs_path(const std::string &p): std::string(p){}

std::string fs_path::leaf(void) const{
    const size_t pos = this->rfind("/");
    if (pos == std::string::npos) return *this;
    return this->substr(pos+1);
}

fs_path fs_path::branch_path(void) const{
    const size_t pos = this->rfind("/");
    if (pos == std::string::npos) return *this;
    return fs_path(this->substr(0, pos));
}

fs_path uhd::operator/(const fs_path &lhs, const fs_path &rhs){
    //strip trailing slash on left-hand-side
    if (not lhs.empty() and *lhs.rbegin() == '/'){
        return fs_path(lhs.substr(0, lhs.size()-1)) / rhs;
    }

    //strip leading slash on right-hand-side
    if (not rhs.empty() and *rhs.begin() == '/'){
        return lhs / fs_path(rhs.substr(1));
    }

    return fs_path(lhs + "/" + rhs);
}

fs_path uhd::operator/(const fs_path &lhs, size_t rhs)
{
    fs_path rhs_str = boost::lexical_cast<std::string>(rhs);
    return lhs / rhs_str;
}

/***********************************************************************
 * Property tree implementation
 **********************************************************************/
class property_tree_impl : public uhd::property_tree{
public:

    property_tree_impl(const fs_path &root = fs_path()):
        _root(root)
    {
        _guts = boost::make_shared<tree_guts_type>();
    }

    sptr subtree(const fs_path &path_) const{
        const fs_path path = _root / path_;
        boost::mutex::scoped_lock lock(_guts->mutex);

        property_tree_impl *subtree = new property_tree_impl(path);
        subtree->_guts = this->_guts; //copy the guts sptr
        return sptr(subtree);
    }

    void remove(const fs_path &path_){
        const fs_path path = _root / path_;
        boost::mutex::scoped_lock lock(_guts->mutex);

        node_type *parent = NULL;
        node_type *node = &_guts->root;
        BOOST_FOREACH(const std::string &name, path_tokenizer(path)){
            if (not node->has_key(name)) throw_path_not_found(path);
            parent = node;
            node = &(*node)[name];
        }
        if (parent == NULL) throw uhd::runtime_error("Cannot uproot");
        parent->pop(fs_path(path.leaf()));
    }

    bool exists(const fs_path &path_) const{
        const fs_path path = _root / path_;
        boost::mutex::scoped_lock lock(_guts->mutex);

        node_type *node = &_guts->root;
        BOOST_FOREACH(const std::string &name, path_tokenizer(path)){
            if (not node->has_key(name)) return false;
            node = &(*node)[name];
        }
        return true;
    }

    std::vector<std::string> list(const fs_path &path_) const{
        const fs_path path = _root / path_;
        boost::mutex::scoped_lock lock(_guts->mutex);

        node_type *node = &_guts->root;
        BOOST_FOREACH(const std::string &name, path_tokenizer(path)){
            if (not node->has_key(name)) throw_path_not_found(path);
            node = &(*node)[name];
        }

        return node->keys();
    }

    void _create(const fs_path &path_, const boost::shared_ptr<void> &prop){
        const fs_path path = _root / path_;
        boost::mutex::scoped_lock lock(_guts->mutex);

        node_type *node = &_guts->root;
        BOOST_FOREACH(const std::string &name, path_tokenizer(path)){
            if (not node->has_key(name)) (*node)[name] = node_type();
            node = &(*node)[name];
        }
        if (node->prop.get() != NULL) throw uhd::runtime_error("Cannot create! Property already exists at: " + path);
        node->prop = prop;
    }

    boost::shared_ptr<void> &_access(const fs_path &path_) const{
        const fs_path path = _root / path_;
        boost::mutex::scoped_lock lock(_guts->mutex);

        node_type *node = &_guts->root;
        BOOST_FOREACH(const std::string &name, path_tokenizer(path)){
            if (not node->has_key(name)) throw_path_not_found(path);
            node = &(*node)[name];
        }
        if (node->prop.get() == NULL) throw uhd::runtime_error("Cannot access! Property uninitialized at: " + path);
        return node->prop;
    }

private:
    void throw_path_not_found(const fs_path &path) const{
        throw uhd::lookup_error("Path not found in tree: " + path);
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
    const fs_path _root;
};

property_tree::~property_tree(void){
    /* NOP */
}

/***********************************************************************
 * Property tree factory
 **********************************************************************/
uhd::property_tree::sptr uhd::property_tree::make(void){
    return sptr(new property_tree_impl());
}
