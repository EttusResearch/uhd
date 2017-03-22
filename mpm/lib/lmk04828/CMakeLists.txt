MACRO(ETTUS_PYTHON_GEN_SOURCE pyfile outfile)
    #ensure that the directory exists for outfile
    GET_FILENAME_COMPONENT(outfile_dir ${outfile} PATH)
    FILE(MAKE_DIRECTORY ${outfile_dir})
    IF(NOT PYTHON_EXECUTABLE)
        MESSAGE( FATAL_ERROR "No python executable found to generate ic_regmaps!" )
    ENDIF(NOT PYTHON_EXECUTABLE)
    #make the outfile depend on the python script
    ADD_CUSTOM_COMMAND(
        OUTPUT ${outfile} DEPENDS ${pyfile} ${ETTUS_PYTHON_GEN_SOURCE_DEPS}
        COMMAND ${PYTHON_EXECUTABLE} -B ${pyfile} ${outfile}
        COMMENT "Generating ${outfile}"
    )

    #make lmk04828 depend on the outfile
    LIST(APPEND lmk04828_srcs ${ARGV})
ENDMACRO(ETTUS_PYTHON_GEN_SOURCE)

####################################################
# LMK04828
####################################################

# Register definitions need to be generated
SET(UHD_HOST_ROOT ${CMAKE_SOURCE_DIR}/../host)
MESSAGE("uhd host root: ${UHD_HOST_ROOT}")
SET(UHD_IC_REG_MAP_PATH ${UHD_HOST_ROOT}/lib/ic_reg_maps)

SET(ETTUS_PYTHON_GEN_SOURCE_DEPS ${UHD_IC_REG_MAP_PATH}/common.py)
ETTUS_PYTHON_GEN_SOURCE(
    ${UHD_IC_REG_MAP_PATH}/gen_lmk04828_regs.py
    ${CMAKE_CURRENT_BINARY_DIR}/lmk04828_regs.hpp
)
SET(LIBUHD_PYTHON_GEN_SOURCE_DEPS)


# LMK04828 driver files
LIST(APPEND lmk04828_srcs
    ${UHD_HOST_ROOT}/lib/usrp/common/lmk04828.cpp
    ${UHD_HOST_ROOT}/lib/types/serial.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/lmk04828_spi_iface.cpp
    )

# Extra files needed
LIST(APPEND lmk04828_srcs ${UHD_HOST_ROOT}/lib/exception.cpp)

# For include/uhd/exception.hpp and include/uhd/config.h

USRP_PERIPHS_ADD_OBJECT(lmk04828 ${lmk04828_srcs})
TARGET_INCLUDE_DIRECTORIES(lmk04828 PUBLIC ${UHD_HOST_ROOT}/include ${CMAKE_CURRENT_BINARY_DIR} ${UHD_HOST_ROOT}/lib/usrp/common)
