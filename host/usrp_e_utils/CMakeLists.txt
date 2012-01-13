#
# Copyright 2011 Ettus Research LLC
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
# USRP embedded utilities that get installed into the share path
########################################################################
LIBUHD_REGISTER_COMPONENT("USRP-E Utils" ENABLE_USRP_E_UTILS OFF "LINUX" OFF)

IF(ENABLE_USRP_E_UTILS)
    ENABLE_LANGUAGE(C)
    INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
    INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/lib/usrp/e100)
    INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/lib/usrp/e100/include)

    SET(usrp_e_utils_sources
        usrp-e-loopback.cpp
        usrp-e-wb-test.cpp
    )

    #for each source: build an executable and install
    FOREACH(util_source ${usrp_e_utils_sources})
        GET_FILENAME_COMPONENT(util_name ${util_source} NAME_WE)
        ADD_EXECUTABLE(${util_name} ${util_source})
        TARGET_LINK_LIBRARIES(${util_name} ${Boost_LIBRARIES})
        INSTALL(TARGETS ${util_name} RUNTIME DESTINATION ${PKG_LIB_DIR}/usrp_e_utils)
    ENDFOREACH(util_source)

ENDIF(ENABLE_USRP_E_UTILS)
