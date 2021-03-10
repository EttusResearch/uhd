set(VCPKG_TARGET_ARCHITECTURE x86)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
# uhd dynamic links for boost-test and libusb.
# The other boost dependencies are static.
# This maintains historical compatibility
# with previous binaries.
if (PORT STREQUAL boost-test)
    set(VCPKG_LIBRARY_LINKAGE dynamic)
endif()

if (PORT STREQUAL libusb)
    set(VCPKG_LIBRARY_LINKAGE dynamic)
endif()
