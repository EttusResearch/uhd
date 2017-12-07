#
# Copyright 2010-2013 Ettus Research LLC
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

########################################################################
# This file included, use CMake directory variables
########################################################################

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
SET(LIBUHD_PYTHON_GEN_SOURCE_DEPS ${CMAKE_CURRENT_SOURCE_DIR}/common.py)

LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_adf4350_regs.py
    ${CMAKE_CURRENT_BINARY_DIR}/adf4350_regs.hpp
)

LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_adf4351_regs.py
    ${CMAKE_CURRENT_BINARY_DIR}/adf4351_regs.hpp
)

LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_max2870_regs.py
    ${CMAKE_CURRENT_BINARY_DIR}/max2870_regs.hpp
)

LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_max2871_regs.py
    ${CMAKE_CURRENT_BINARY_DIR}/max2871_regs.hpp
)

LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_adf4360_regs.py
    ${CMAKE_CURRENT_BINARY_DIR}/adf4360_regs.hpp
)

LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_ad9510_regs.py
    ${CMAKE_CURRENT_BINARY_DIR}/ad9510_regs.hpp
)

LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_ad9777_regs.py
    ${CMAKE_CURRENT_BINARY_DIR}/ad9777_regs.hpp
)

LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_ad5623_regs.py
    ${CMAKE_CURRENT_BINARY_DIR}/ad5623_regs.hpp
)

LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_ad7922_regs.py
    ${CMAKE_CURRENT_BINARY_DIR}/ad7922_regs.hpp
)

LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_max2829_regs.py
    ${CMAKE_CURRENT_BINARY_DIR}/max2829_regs.hpp
)

LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_max2118_regs.py
    ${CMAKE_CURRENT_BINARY_DIR}/max2118_regs.hpp
)

LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_max2112_regs.py
    ${CMAKE_CURRENT_BINARY_DIR}/max2112_regs.hpp
)

LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_max2112_regs.py
    ${CMAKE_CURRENT_BINARY_DIR}/max2112_regs.hpp
)

LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_ad9862_regs.py
    ${CMAKE_CURRENT_BINARY_DIR}/ad9862_regs.hpp
)

LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_ad9522_regs.py
    ${CMAKE_CURRENT_BINARY_DIR}/ad9522_regs.hpp
)

LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_ads62p44_regs.py
    ${CMAKE_CURRENT_BINARY_DIR}/ads62p44_regs.hpp
)

LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_ads62p48_regs.py
    ${CMAKE_CURRENT_BINARY_DIR}/ads62p48_regs.hpp
)

LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_tuner_4937di5_regs.py
    ${CMAKE_CURRENT_BINARY_DIR}/tuner_4937di5_regs.hpp
)

LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_tda18272hnm_regs.py
    ${CMAKE_CURRENT_BINARY_DIR}/tda18272hnm_regs.hpp
)

LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_lmk04816_regs.py
    ${CMAKE_CURRENT_BINARY_DIR}/lmk04816_regs.hpp
)

LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_adf5355_regs.py
    ${CMAKE_CURRENT_BINARY_DIR}/adf5355_regs.hpp
)

LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_adf5356_regs.py
    ${CMAKE_CURRENT_BINARY_DIR}/adf5356_regs.hpp
)

SET(LIBUHD_PYTHON_GEN_SOURCE_DEPS)
