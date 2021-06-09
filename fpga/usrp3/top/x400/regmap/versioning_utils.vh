//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: versioning_utils
//
// Description:
//
//  Contains constants and functions for versioning purposes
//
//  IMPORTANT! The constants and functions defined in this file depend
//             on versioning_regs_regmap_utils.vh, which must be
//             included before this file.
//


// Each component consists of 3 x 32-bit values (96-bit total)
// The component's versions are located in the flat component's
// version vector as shown below, following the same order in which
// the registers' offsets are implemented.
//
// Version element             Bit ranges  32-bit word position
//  Current version             [31: 0]     0
//  Oldest compatible version   [63:32]     1
//  Last modified               [95:64]     2

localparam COMPONENT_VERSIONS_SIZE =
  TIMESTAMP_TYPE_SIZE + VERSION_TYPE_SIZE + VERSION_TYPE_SIZE; // 96

// There are up to 64 addressable components' versions in the
// versioning module's version_info input vector.
localparam MAX_NUM_OF_COMPONENTS = 64;

// Define constants for each field's LSB in the flat vector.
//   Start bit = 8-bit * (register offset with index 0)
localparam CURRENT_VERSION_LSB           = 8 * CURRENT_VERSION(0);           // 0
localparam OLDEST_COMPATIBLE_VERSION_LSB = 8 * OLDEST_COMPATIBLE_VERSION(0); // 32
localparam TIMESTAMP_LSB                 = 8 * VERSION_LAST_MODIFIED(0);     // 64

// This function takes the major, minor and build values for the current
// version field, and returns a vector of size VERSION_TYPE_SIZE
// that contains those fields at the proper location.
function automatic [VERSION_TYPE_SIZE-1:0] build_version;
  input [MAJOR_SIZE-1:0] major;
  input [MINOR_SIZE-1:0] minor;
  input [BUILD_SIZE-1:0] build;
begin
  build_version[MAJOR+:MAJOR_SIZE] = major;
  build_version[MINOR+:MINOR_SIZE] = minor;
  build_version[BUILD+:BUILD_SIZE] = build;
end
endfunction

// This function takes the 3 versioning fields that comprise a component's
// version (current, oldest compatible, timestamp), and concatenates them
// in the expected order (see details above).
// The function returns a vector of size COMPONENT_VERSIONS_SIZE with
// all the component's versions.
function automatic [COMPONENT_VERSIONS_SIZE-1:0] build_component_versions;
  input [TIMESTAMP_TYPE_SIZE-1:0]                timestamp;
  input [VERSION_TYPE_SIZE-1:0]  oldest_compatible_version;
  input [VERSION_TYPE_SIZE-1:0]            current_version;
begin
    // Current version mapping
    build_component_versions[CURRENT_VERSION_LSB +: VERSION_TYPE_SIZE] = current_version;
    // Oldest compatible version mapping
    build_component_versions[OLDEST_COMPATIBLE_VERSION_LSB +: VERSION_TYPE_SIZE] = oldest_compatible_version;
    // Last modified
    build_component_versions[TIMESTAMP_LSB +: TIMESTAMP_TYPE_SIZE] = timestamp;
end
endfunction

// This function retrieves a component's version information, based on the
// provided index, from the vector containing all the components' versions.
function automatic [COMPONENT_VERSIONS_SIZE-1:0] get_component_versions;
  input [MAX_NUM_OF_COMPONENTS*COMPONENT_VERSIONS_SIZE-1:0] version_info_vector;
  input integer                                             component_index;
begin
  get_component_versions = version_info_vector[COMPONENT_VERSIONS_SIZE*component_index +: COMPONENT_VERSIONS_SIZE];
end
endfunction

// This function takes a component's version info and returns the
// current version field.
function automatic [VERSION_TYPE_SIZE-1:0] current_version;
  input [COMPONENT_VERSIONS_SIZE-1:0] component_versions;
begin
  current_version = component_versions[CURRENT_VERSION_LSB +: VERSION_TYPE_SIZE];
end
endfunction

// This function takes a component's version info and returns the
// oldest compatible version field.
function automatic [VERSION_TYPE_SIZE-1:0] oldest_compatible_version;
  input [COMPONENT_VERSIONS_SIZE-1:0] component_versions;
begin
  oldest_compatible_version = component_versions[OLDEST_COMPATIBLE_VERSION_LSB +: VERSION_TYPE_SIZE];
end
endfunction

// This function takes a component's version info and returns the
// version last modified field.
function automatic [VERSION_TYPE_SIZE-1:0] version_last_modified;
  input [COMPONENT_VERSIONS_SIZE-1:0] component_versions;
begin
  version_last_modified = component_versions[TIMESTAMP_LSB +: TIMESTAMP_TYPE_SIZE];
end
endfunction
