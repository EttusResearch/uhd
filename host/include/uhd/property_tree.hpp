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

#ifndef INCLUDED_UHD_PROPERTY_TREE_HPP
#define INCLUDED_UHD_PROPERTY_TREE_HPP

#include <uhd/config.hpp>
#include <uhd/property.hpp>
#include <boost/any.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/filesystem/path.hpp>
#include <list>

namespace uhd{

/*!
 * The property tree provides a file system structure for accessing properties.
 */
class UHD_API property_tree : boost::noncopyable{
public:
    typedef boost::shared_ptr<property_tree> sptr;
    typedef boost::filesystem::path path_type;
    typedef std::list<std::string> iterator_type;

    //! Create a new + empty property tree
    static sptr make(void);

    //! Remove a property or directory (recursive)
    virtual void remove(const path_type &path) = 0;

    //! True if the path exists in the tree
    virtual bool exists(const path_type &path) = 0;

    //! Get an iterator to all things in the given path
    virtual iterator_type iterate(const path_type &path) = 0;

    //! Create a new property entry in the tree
    template <typename T> void create(const path_type &path, const property<T> &prop = property<T>()){
        return this->_create(path, prop);
    }

    //! Get access to a property in the tree
    template <typename T> property<T> access(const path_type &path){
        return boost::any_cast<property<T> >(this->_access(path));
    }

protected:
    //! Internal create property with wild-card type
    virtual void _create(const path_type &path, const boost::any &prop) = 0;

    //! Internal access property with wild-card type
    virtual boost::any &_access(const path_type &path) = 0;

};

} //namespace uhd

#endif /* INCLUDED_UHD_PROPERTY_TREE_HPP */
