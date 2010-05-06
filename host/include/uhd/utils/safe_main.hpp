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

#ifndef INCLUDED_UHD_UTILS_SAFE_MAIN_HPP
#define INCLUDED_UHD_UTILS_SAFE_MAIN_HPP

#include <uhd/config.hpp>
#include <iostream>
#include <stdexcept>

/*!
 * Defines a safe wrapper that places a catch-all around main.
 * If an exception is thrown, it prints to stderr and returns.
 * Usage: int UHD_SAFE_MAIN(int argc, char *argv[]){ main code here }
 * \param _argc the declaration for argc
 * \param _argv the declaration for argv
 */
#define UHD_SAFE_MAIN(_argc, _argv) _main(int, char*[]); \
int main(int argc, char *argv[]){ \
    try { \
        return _main(argc, argv); \
    } catch(const std::exception &e) { \
        std::cerr << "Error: " << e.what() << std::endl; \
    } catch(...) { \
        std::cerr << "Error: unknown exception" << std::endl; \
    } \
    return ~0; \
} int _main(_argc, _argv)

#endif /* INCLUDED_UHD_UTILS_SAFE_MAIN_HPP */
