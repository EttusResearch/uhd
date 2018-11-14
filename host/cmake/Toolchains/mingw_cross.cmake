#
# Allow the user to specify a MinGW prefix, but fill in
# most likely default if none given.
#

set(CMAKE_SYSTEM_NAME Windows)

if(NOT DEFINED MINGW_PREFIX)
    set(POSSIBLE_PREFIXES
        i586-mingw32msvc
        i686-pc-mingw32
        x86_64-pc-mingw32
        i686-w64-mingw32
        x86_64-w64-mingw32
    )

    set(MINGW_FOUND 0)
    foreach(prefix ${POSSIBLE_PREFIXES})
        if(EXISTS /usr/${prefix})
            set(MINGW_PREFIX ${prefix})
            set(MINGW_FOUND 1)
            break()
        endif(EXISTS /usr/${prefix})
    endforeach(prefix ${POSSIBLE_PREFIXES})

    if(NOT MINGW_FOUND)
        message(FATAL_ERROR "No MinGW type specified, but none detected in the usual locations.")
    endif(NOT MINGW_FOUND)
endif(NOT DEFINED MINGW_PREFIX)

set(MINGW_PREFIX ${MINGW_PREFIX} CACHE STRING "MinGW prefix")

set(CMAKE_C_COMPILER ${MINGW_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${MINGW_PREFIX}-g++)
set(CMAKE_RC_COMPILER ${MINGW_PREFIX}-windres)

if(NOT DEFINED CMAKE_FIND_ROOT_PATH)
    set(CMAKE_FIND_ROOT_PATH /usr/${MINGW_PREFIX})
endif(NOT DEFINED CMAKE_FIND_ROOT_PATH)

set(CMAKE_INCLUDE_PATH
    ${CMAKE_FIND_ROOT_PATH}/local/include
    ${CMAKE_FIND_ROOT_PATH}/include
)

set(CMAKE_LIBRARY_PATH
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
