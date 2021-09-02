#
# Copyright 2017 Ettus Research, National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

#macro(ETTUS_PYTHON_GEN_SOURCE pyfile outfile)
    ##ensure that the directory exists for outfile
    #get_filename_component(outfile_dir ${outfile} PATH)
    #file(MAKE_DIRECTORY ${outfile_dir})
    #if(NOT PYTHON_EXECUTABLE)
        #message( FATAL_ERROR "No python executable found to generate ic_regmaps!" )
    #endif(NOT PYTHON_EXECUTABLE)
    ##make the outfile depend on the python script
    #add_custom_command(
        #OUTPUT ${outfile} DEPENDS ${pyfile} ${ETTUS_PYTHON_GEN_SOURCE_DEPS}
        #COMMAND ${PYTHON_EXECUTABLE} -B ${pyfile} ${outfile}
        #COMMENT "Generating ${outfile}"
    #)

    ##make lmk04828 depend on the outfile
    #list(APPEND lmk04828_srcs ${ARGV})
#endmacro(ETTUS_PYTHON_GEN_SOURCE)

####################################################
# LMK04828
####################################################

# Register definitions need to be generated
#set(UHD_SOURCE_DIR ${CMAKE_SOURCE_DIR}/../host)
#message("uhd host root: ${UHD_SOURCE_DIR}")
#set(UHD_IC_REG_MAP_PATH ${UHD_SOURCE_DIR}/lib/ic_reg_maps)

#set(ETTUS_PYTHON_GEN_SOURCE_DEPS ${UHD_IC_REG_MAP_PATH}/common.py)
#ETTUS_PYTHON_GEN_SOURCE(
    #${UHD_IC_REG_MAP_PATH}/gen_lmk04828_regs.py
    #${CMAKE_CURRENT_BINARY_DIR}/lmk04828_regs.hpp
#)
#set(LIBUHD_PYTHON_GEN_SOURCE_DEPS)

# Define the object
#USRP_PERIPHS_ADD_OBJECT(chips
    #lmk04828_spi_iface.cpp
#)
