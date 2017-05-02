#
# Copyright 2017 Ettus Research (National Instruments)
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

#MACRO(ETTUS_PYTHON_GEN_SOURCE pyfile outfile)
    ##ensure that the directory exists for outfile
    #GET_FILENAME_COMPONENT(outfile_dir ${outfile} PATH)
    #FILE(MAKE_DIRECTORY ${outfile_dir})
    #IF(NOT PYTHON_EXECUTABLE)
        #MESSAGE( FATAL_ERROR "No python executable found to generate ic_regmaps!" )
    #ENDIF(NOT PYTHON_EXECUTABLE)
    ##make the outfile depend on the python script
    #ADD_CUSTOM_COMMAND(
        #OUTPUT ${outfile} DEPENDS ${pyfile} ${ETTUS_PYTHON_GEN_SOURCE_DEPS}
        #COMMAND ${PYTHON_EXECUTABLE} -B ${pyfile} ${outfile}
        #COMMENT "Generating ${outfile}"
    #)

    ##make lmk04828 depend on the outfile
    #LIST(APPEND lmk04828_srcs ${ARGV})
#ENDMACRO(ETTUS_PYTHON_GEN_SOURCE)

####################################################
# LMK04828
####################################################

# Register definitions need to be generated
#SET(UHD_HOST_ROOT ${CMAKE_SOURCE_DIR}/../host)
#MESSAGE("uhd host root: ${UHD_HOST_ROOT}")
#SET(UHD_IC_REG_MAP_PATH ${UHD_HOST_ROOT}/lib/ic_reg_maps)

#SET(ETTUS_PYTHON_GEN_SOURCE_DEPS ${UHD_IC_REG_MAP_PATH}/common.py)
#ETTUS_PYTHON_GEN_SOURCE(
    #${UHD_IC_REG_MAP_PATH}/gen_lmk04828_regs.py
    #${CMAKE_CURRENT_BINARY_DIR}/lmk04828_regs.hpp
#)
#SET(LIBUHD_PYTHON_GEN_SOURCE_DEPS)

# Define the object
#USRP_PERIPHS_ADD_OBJECT(chips
    #lmk04828_spi_iface.cpp
#)
