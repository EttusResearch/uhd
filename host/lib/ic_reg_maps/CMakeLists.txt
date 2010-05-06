#
# Copyright 2010 Ettus Research LLC
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

#This file will be included by cmake, use absolute paths!

INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/lib/ic_reg_maps)

LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_SOURCE_DIR}/lib/ic_reg_maps/gen_adf4350_regs.py
    ${CMAKE_BINARY_DIR}/lib/ic_reg_maps/adf4350_regs.hpp
)

LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_SOURCE_DIR}/lib/ic_reg_maps/gen_adf4360_regs.py
    ${CMAKE_BINARY_DIR}/lib/ic_reg_maps/adf4360_regs.hpp
)

LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_SOURCE_DIR}/lib/ic_reg_maps/gen_ad9510_regs.py
    ${CMAKE_BINARY_DIR}/lib/ic_reg_maps/ad9510_regs.hpp
)

LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_SOURCE_DIR}/lib/ic_reg_maps/gen_ad9777_regs.py
    ${CMAKE_BINARY_DIR}/lib/ic_reg_maps/ad9777_regs.hpp
)

LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_SOURCE_DIR}/lib/ic_reg_maps/gen_ad5624_regs.py
    ${CMAKE_BINARY_DIR}/lib/ic_reg_maps/ad5624_regs.hpp
)

LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_SOURCE_DIR}/lib/ic_reg_maps/gen_ad7922_regs.py
    ${CMAKE_BINARY_DIR}/lib/ic_reg_maps/ad7922_regs.hpp
)

LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_SOURCE_DIR}/lib/ic_reg_maps/gen_max2829_regs.py
    ${CMAKE_BINARY_DIR}/lib/ic_reg_maps/max2829_regs.hpp
)

LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_SOURCE_DIR}/lib/ic_reg_maps/gen_ad9862_regs.py
    ${CMAKE_BINARY_DIR}/lib/ic_reg_maps/ad9862_regs.hpp
)

LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_SOURCE_DIR}/lib/ic_reg_maps/gen_ad9522_regs.py
    ${CMAKE_BINARY_DIR}/lib/ic_reg_maps/ad9522_regs.hpp
)
