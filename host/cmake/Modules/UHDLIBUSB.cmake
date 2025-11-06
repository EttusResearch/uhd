#
# Copyright 2024 Ettus Research, an NI Brand, now part of Emerson
#
# SPDX-License-Identifier: GPL-3.0-or-later

if(WIN32)
    ########################################################################
    # Find libusb dynamically linked library from libusb library path
    ########################################################################
    list (GET LIBUSB_LIBRARIES 0 UHD_LIBUSB_LIBRARY_PATH)
    get_filename_component(UHD_LIBUSB_LIBRARIERS_DIR ${UHD_LIBUSB_LIBRARY_PATH} DIRECTORY)
    if(DEFINED PC_LIBUSB_LIBRARIES)
        set(UHD_LIBUSB_BASENAME ${PC_LIBUSB_LIBRARIES})
    else()
        get_filename_component(UHD_LIBUSB_LIBRARIERS_NAME_WEXT ${UHD_LIBUSB_LIBRARY_PATH} NAME_WLE)
        string(REGEX REPLACE "${CMAKE_STATIC_LIBRARY_SUFFIX}$" "" UHD_LIBUSB_BASENAME ${UHD_LIBUSB_LIBRARIERS_NAME_WEXT})
    endif(DEFINED PC_LIBUSB_LIBRARIES)

    # search for DLL in folder hierarchies expected in downloaded binaries (sourceforge) and vcpkg
    find_file(UHD_LIBUSBDLL_ABSPATH
        NAMES ${UHD_LIBUSB_BASENAME}${CMAKE_SHARED_LIBRARY_SUFFIX}
        PATHS 
            ${UHD_LIBUSB_LIBRARIERS_DIR}
            ${UHD_LIBUSB_LIBRARIERS_DIR}/../bin
        DOC "Path to libusb DLL file"
    )
    if(NOT ${UHD_LIBUSBDLL_ABSPATH} STREQUAL "")
        file(RELATIVE_PATH  UHD_LIBUSBDLLPATH "${CMAKE_CURRENT_SOURCE_DIR}" "${UHD_LIBUSBDLL_ABSPATH}")
        message(STATUS "Found ${UHD_LIBUSB_BASENAME}${CMAKE_SHARED_LIBRARY_SUFFIX} at ${UHD_LIBUSBDLL_ABSPATH}")
    endif(NOT ${UHD_LIBUSBDLL_ABSPATH} STREQUAL "")

    ########################################################################
    # Generation LIBUSB LICENSE file using libusb.h comment block
    ########################################################################
    if(NOT DEFINED UHD_LIBUSB_LICENSEFILE)
        # Variable should be defined in UHDPackage module. 
        # If not, just in case set here to the same value
        set(UHD_LIBUSB_LICENSEFILE "${UHD_BINARY_DIR}/LIBUSB_LICENSE")
    endif(NOT DEFINED UHD_LIBUSB_LICENSEFILE)

    #extract comment block from libusb include file
    # Define the path to the file
    set(FILE_PATH "${LIBUSB_INCLUDE_DIRS}/libusb.h")

    # Read the contents of the file into a variable
    file(READ ${FILE_PATH} FILE_CONTENTS)

    # Define a variable to hold the comment block
    set(COMMENT_BLOCK "")

    # Use a regular expression to extract the comment block
    # This example assumes the comment block is a C-style comment (/* ... */)
    string(REGEX MATCH "/\\*([^*]|\\*+[^*/])*\\*+/" COMMENT_BLOCK_MATCH ${FILE_CONTENTS})
    string(LENGTH ${COMMENT_BLOCK_MATCH} COMMENT_BLOCK_MATCH_LENGTH) # get length of the match

    # Extract the comment block from the match
    if(COMMENT_BLOCK_MATCH)
        # Note: cmake regex uses " " and has no representation of "\n"
        # Remove the lines with /*, */ delimiters from the comment block
        string(REGEX REPLACE "^/\\* *\n|\n *\\*/$" "" COMMENT_BLOCK_MATCH ${COMMENT_BLOCK_MATCH})

        # Remove first line that typically identifies the source of the comment
        # end reads ".. Public libusb header file"
        string(REGEX REPLACE "^[^\n]*\n(.+)?" "\\1" COMMENT_BLOCK_MATCH ${COMMENT_BLOCK_MATCH})

        # Remove * at the beginning of each line 
        # (line identified by begin of string or newline followed by " * ")
        string(REGEX REPLACE "^ *\\* *|\n *\\* *" "\n" COMMENT_BLOCK_MATCH ${COMMENT_BLOCK_MATCH})
        message(STATUS "Extracted comment block from libusb.h: \n${COMMENT_BLOCK_MATCH}")
    else()
        message(WARNING "Comment block not found in file: ${FILE_PATH}")
    endif()

    ########################################################################
    # Generation LIBUSB LICENSE file using
    # - libusb version (from pkg-config)
    # - extracted comment block from libusb.h
    ########################################################################
    set (LIBUSB_VERSION ${PC_LIBUSB_VERSION})
    set (LIBUSB_INCLUDE_COMMENT_HEADER ${COMMENT_BLOCK_MATCH})
    configure_file(
        ${UHD_SOURCE_DIR}/cmake/Modules/LIBUSB_LICENSE.in
        ${UHD_LIBUSB_LICENSEFILE}
    @ONLY)

endif(WIN32)
