#
# This file is mostly inspired by
# https://github.com/llvm-mirror/llvm/blob/master/cmake/modules/CheckAtomic.cmake
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

include(CheckCXXSourceCompiles)
include(CheckLibraryExists)

# Sometimes linking against libatomic is required for atomic ops, if
# the platform doesn't support lock-free atomics.

function(CHECK_WORKING_CXX_ATOMICS varname)
    CHECK_CXX_SOURCE_COMPILES("
        #include <atomic>
        std::atomic<int> x;
        int main() {
        return x;
        }
        " ${varname})
endfunction(CHECK_WORKING_CXX_ATOMICS)

function(CHECK_WORKING_CXX_ATOMICS64 varname)
    CHECK_CXX_SOURCE_COMPILES("
        #include <atomic>
        #include <cstdint>
        std::atomic<uint64_t> x (0);
        int main() {
          uint64_t i = x.load(std::memory_order_relaxed);
          return 0;
        }
        " ${varname})
endfunction(CHECK_WORKING_CXX_ATOMICS64)

macro(CHECK_ATOMICS_LIB_REQUIRED required_var)
    set(${required_var} FALSE)
    if(MSVC)
        set(${required_var} FALSE)
    else()
        # First check if atomics work without the library.
        CHECK_WORKING_CXX_ATOMICS(HAVE_CXX_ATOMICS_WITHOUT_LIB)
        # If not, check if the library exists, and atomics work with it.
        if(NOT HAVE_CXX_ATOMICS_WITHOUT_LIB)
            check_library_exists(atomic __atomic_fetch_add_4 "" HAVE_LIBATOMIC)
            if(HAVE_LIBATOMIC)
                set(${required_var} TRUE)
                set(CMAKE_REQUIRED_LIBRARIES "atomic")
                CHECK_WORKING_CXX_ATOMICS(HAVE_CXX_ATOMICS_WITH_LIB)
                if (NOT HAVE_CXX_ATOMICS_WITH_LIB)
                    message(FATAL_ERROR "Host compiler must support std::atomic!")
                endif()
                unset(CMAKE_REQUIRED_LIBRARIES)
            else()
                message(
                    FATAL_ERROR
                    "Host compiler appears to require libatomic, but cannot find it.")
            endif()
        endif()
        # Same check, but for 64-bit atomics.
        CHECK_WORKING_CXX_ATOMICS64(HAVE_CXX_ATOMICS64_WITHOUT_LIB)
        # If not, check if the library exists, and atomics work with it.
        if(NOT HAVE_CXX_ATOMICS64_WITHOUT_LIB)
            check_library_exists(atomic __atomic_load_8 "" HAVE_CXX_LIBATOMICS64)
            if(HAVE_CXX_LIBATOMICS64)
                set(${required_var} TRUE)
                set(CMAKE_REQUIRED_LIBRARIES "atomic")
                CHECK_WORKING_CXX_ATOMICS64(HAVE_CXX_ATOMICS64_WITH_LIB)
                unset(CMAKE_REQUIRED_LIBRARIES)
                if (NOT HAVE_CXX_ATOMICS64_WITH_LIB)
                    message(FATAL_ERROR "Host compiler must support 64-bit std::atomic!")
                endif()
            else()
                message(
                    FATAL_ERROR
                    "Host compiler appears to require libatomic for 64-bit operations, but cannot find it.")
            endif()
        endif()
    endif()
endmacro(CHECK_ATOMICS_LIB_REQUIRED)
