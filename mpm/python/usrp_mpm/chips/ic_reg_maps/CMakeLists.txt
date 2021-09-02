#
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

# macro for creating ic_reg_map files from a given generation file
macro(REG_MAPS_GEN_SOURCE pyfile outfile)
    # ensure that the directory exists for outfile
    get_filename_component(outfile_dir ${outfile} PATH)
    file(MAKE_DIRECTORY ${outfile_dir})

    # get name of outfile for the Python submodule __init__ file
    get_filename_component(outfile_name ${outfile} NAME_WE)
    string(APPEND REG_MAP_IMPORT "\nfrom ." ${outfile_name} " import " ${outfile_name} "_t")

    # make the outfile depend on the python script
    add_custom_command(
        OUTPUT ${outfile} DEPENDS ${pyfile} ${REG_MAPS_GEN_SOURCE_DEPS}
        COMMAND ${PYTHON_EXECUTABLE} -B ${pyfile} ${outfile}
        COMMENT "Generating ${outfile}"
    )

    # store all reg maps to be generated
    set(IC_REG_MAPS ${IC_REG_MAPS} ${outfile})
endmacro(REG_MAPS_GEN_SOURCE)

if(ENABLE_REGMAPS)
    # designate the file which will be used to generate all reg maps
    set(REG_MAPS_GEN_SOURCE_DEPS ${UHD_SOURCE_DIR}/lib/ic_reg_maps/common.py)

    # specify all reg maps you wish to generate
    REG_MAPS_GEN_SOURCE(
        ${UHD_SOURCE_DIR}/lib/ic_reg_maps/gen_lmk04816_regs.py
        ${CMAKE_CURRENT_BINARY_DIR}/lmk04816_regs.py
    )
    REG_MAPS_GEN_SOURCE(
        ${UHD_SOURCE_DIR}/lib/ic_reg_maps/gen_lmx2572_regs.py
        ${CMAKE_CURRENT_BINARY_DIR}/lmx2572_regs.py
    )
    REG_MAPS_GEN_SOURCE(
        ${UHD_SOURCE_DIR}/lib/ic_reg_maps/gen_zbx_cpld_regs.py
        ${CMAKE_CURRENT_BINARY_DIR}/zbx_cpld_regs.py
    )

    # add an ic_reg_maps target which can be referenced outside of this subdirectory
    add_custom_target(ic_reg_maps DEPENDS ${IC_REG_MAPS})

    # add new ic_reg_maps target to the list of usrp_mpm dependencies
    set(USRP_MPM_FILES ${USRP_MPM_FILES})
    list(APPEND USRP_MPM_FILES ic_reg_maps)
    set(USRP_MPM_FILES ${USRP_MPM_FILES} PARENT_SCOPE)
endif(ENABLE_REGMAPS)

# write the Python submodule __init__.py by appending all generated reg map file
# names to be imported. This is not within "if(ENABLE_REGMAPS)" because MPM
# depends on the ic_reg_maps submodule regardless of whether any regmaps are
# generated.
set(REG_MAP_INIT_IN "${CMAKE_CURRENT_SOURCE_DIR}/__init__.py.in")
set(REG_MAP_INIT "${CMAKE_CURRENT_BINARY_DIR}/__init__.py")
configure_file(${REG_MAP_INIT_IN} ${REG_MAP_INIT})
