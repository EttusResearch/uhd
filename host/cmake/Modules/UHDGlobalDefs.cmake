#
# Copyright 2015 Ettus Research LLC
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

# This file sets up all the stuff for the config.h file

INCLUDE(CheckCXXSymbolExistsCopy)

## Check for std::log2
CHECK_CXX_SYMBOL_EXISTS(log2 cmath HAVE_LOG2)

## Macros for the version number
IF(UHD_VERSION_DEVEL)
    MATH(EXPR UHD_VERSION_ADDED "10000 * ${TRIMMED_VERSION_MAJOR} + 100 * ${TRIMMED_VERSION_MINOR} + 99")
ELSE()
    MATH(EXPR UHD_VERSION_ADDED "10000 * ${TRIMMED_VERSION_MAJOR} + 100 * ${TRIMMED_VERSION_MINOR} + ${TRIMMED_VERSION_PATCH}")
ENDIF(UHD_VERSION_DEVEL)
ADD_DEFINITIONS(-DUHD_VERSION=${UHD_VERSION_ADDED})

## make sure the code knows about config.h
ADD_DEFINITIONS(-DHAVE_CONFIG_H)
