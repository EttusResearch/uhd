//
// Copyright 2010 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_UTILS_PIMPL_HPP
#define INCLUDED_UHD_UTILS_PIMPL_HPP

#include <uhd/config.hpp>
#include <boost/shared_ptr.hpp>

/*! \file pimpl.hpp
 * "Pimpl idiom" (pointer to implementation idiom).
 * The UHD_PIMPL_* macros simplify code overhead for declaring and making pimpls.
 *
 * Each pimpl is implemented as a shared pointer to the implementation:
 * - The container class will not have to deallocate the pimpl.
 * - The container class will use the pimpl as a regular pointer.
 * - Usage: _impl->method(arg0, arg1)
 * - Usage: _impl->member = value;
 *
 * \see http://en.wikipedia.org/wiki/Opaque_pointer
 */

/*!
 * Make a declaration for a pimpl in a header file.
 * - Usage: UHD_PIMPL_DECL(impl) _impl;
 * \param _name the name of the pimpl class
 */
#define UHD_PIMPL_DECL(_name) \
    struct _name; boost::shared_ptr<_name>

/*!
 * Make an instance of a pimpl in a source file.
 * - Usage: _impl = UHD_PIMPL_MAKE(impl, ());
 * - Usage: _impl = UHD_PIMPL_MAKE(impl, (a0, a1));
 * \param _name the name of the pimpl class
 * \param _args the constructor args for the pimpl
 */
#define UHD_PIMPL_MAKE(_name, _args) \
    boost::shared_ptr<_name>(new _name _args)

#endif /* INCLUDED_UHD_UTILS_PIMPL_HPP */
