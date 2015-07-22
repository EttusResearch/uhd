#
# Allow the user to specify a MinGW prefix, but fill in
# most likely default if none given.
#

SET(CMAKE_SYSTEM_NAME Windows)

IF(NOT DEFINED MINGW_PREFIX)
    SET(POSSIBLE_PREFIXES
        i586-mingw32msvc
        i686-pc-mingw32
        x86_64-pc-mingw32
        i686-w64-mingw32
        x86_64-w64-mingw32
    )

    SET(MINGW_FOUND 0)
    FOREACH(prefix ${POSSIBLE_PREFIXES})
        IF(EXISTS /usr/${prefix})
            SET(MINGW_PREFIX ${prefix})
            SET(MINGW_FOUND 1)
            BREAK()
        ENDIF(EXISTS /usr/${prefix})
    ENDFOREACH(prefix ${POSSIBLE_PREFIXES})

    IF(NOT MINGW_FOUND)
        MESSAGE(FATAL_ERROR "No MinGW type specified, but none detected in the usual locations.")
    ENDIF(NOT MINGW_FOUND)
ENDIF(NOT DEFINED MINGW_PREFIX)

SET(MINGW_PREFIX ${MINGW_PREFIX} CACHE STRING "MinGW prefix")

SET(CMAKE_C_COMPILER ${MINGW_PREFIX}-gcc)
SET(CMAKE_CXX_COMPILER ${MINGW_PREFIX}-g++)
SET(CMAKE_RC_COMPILER ${MINGW_PREFIX}-windres)

IF(NOT DEFINED CMAKE_FIND_ROOT_PATH)
    SET(CMAKE_FIND_ROOT_PATH /usr/${MINGW_PREFIX})
ENDIF(NOT DEFINED CMAKE_FIND_ROOT_PATH)

SET(CMAKE_INCLUDE_PATH
    ${CMAKE_FIND_ROOT_PATH}/local/include
    ${CMAKE_FIND_ROOT_PATH}/include
)

SET(CMAKE_LIBRARY_PATH
    ${CMAKE_FIND_ROOT_PATH}/local/lib
    ${CMAKE_FIND_ROOT_PATH}/lib
)

# Adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Tell pkg-config not to look at the target environment's .pc files.
# Setting PKG_CONFIG_LIBDIR sets the default search directory, but we have to
# set PKG_CONFIG_PATH as well to prevent pkg-config falling back to the host's
# path.
set(ENV{PKG_CONFIG_LIBDIR} ${CMAKE_FIND_ROOT_PATH}/lib/pkgconfig)
set(ENV{PKG_CONFIG_PATH} ${CMAKE_FIND_ROOT_PATH}/lib/pkgconfig)

set(ENV{MINGDIR} ${CMAKE_FIND_ROOT_PATH})
