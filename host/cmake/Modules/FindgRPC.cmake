# FindgRPC.cmake
#
# Find the gRPC C++ library and code-generation plugin.
#
# Tries cmake CONFIG mode first (vcpkg, Ubuntu 24+, Fedora 38+).
# Falls back to pkg-config on Linux for older distros (e.g. Ubuntu 20/22)
# that ship gRPC without cmake config files.
#
# Imported targets:
#   gRPC::grpc++       The gRPC C++ library
#
# Result variables:
#   gRPC_FOUND         True if gRPC was found
#   gRPC_VERSION       Version string (when available)
#   GRPC_CPP_PLUGIN    Full path to the grpc_cpp_plugin executable

# Step 1: Try cmake CONFIG mode (preferred).
# This works on Windows/vcpkg, Ubuntu 24.04+, and Fedora 38+.
find_package(gRPC CONFIG QUIET)

if(gRPC_FOUND)
    if(TARGET gRPC::grpc++)
        set(_gRPC_grpcpp_FOUND TRUE)
    endif()
elseif(NOT WIN32)
    # Step 2: Fall back to pkg-config.
    # Required for Ubuntu 20.04/22.04 where libgrpc++-dev does not ship
    # cmake config files.
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(_grpc_pc QUIET IMPORTED_TARGET grpc++)
        if(_grpc_pc_FOUND)
            if(NOT TARGET gRPC::grpc++)
                add_library(gRPC::grpc++ INTERFACE IMPORTED)
                set_target_properties(gRPC::grpc++ PROPERTIES
                    INTERFACE_LINK_LIBRARIES "PkgConfig::_grpc_pc")
            endif()
            set(gRPC_VERSION "${_grpc_pc_VERSION}")
            set(_gRPC_grpcpp_FOUND TRUE)
        endif()
    endif()
endif()

# Find the protobuf code-generation plugin shipped with gRPC.
# In CONFIG mode (e.g., vcpkg), this is often exposed as an imported target.
if(TARGET gRPC::grpc_cpp_plugin)
    set(GRPC_CPP_PLUGIN $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
else()
    find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(gRPC
    REQUIRED_VARS _gRPC_grpcpp_FOUND GRPC_CPP_PLUGIN
    VERSION_VAR   gRPC_VERSION
)

mark_as_advanced(GRPC_CPP_PLUGIN)
