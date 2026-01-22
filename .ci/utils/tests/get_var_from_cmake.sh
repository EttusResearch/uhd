#!/usr/bin/env bash

set -e

if [ ! -f "extract_cmake_var.py" ]; then
    echo "Missing script extract_cmake_var.py in current folder $(pwd)"
    exit 1
fi
if [[ -z "$1" ]]; then
    echo "Missing input parameter to local path of uhd repo"
    exit 1
fi

python_base_cmd="python extract_cmake_var.py"

_var_source="$1/host/build/CMakeCache.txt"
_vars="VCPKG_INSTALLED_DIR VCPKG_TARGET_TRIPLET CPACK_SOURCE_PACKAGE_FILE_NAME"
python_cmd="$python_base_cmd cmake_cache --cache_file \"$_var_source\" --variables $_vars"
if [[ -f "$_var_source" ]]; then
    python -VV
    cmake --version
    echo "Running Cmd: $python_cmd"
    eval $python_cmd | while IFS== read -r var value; do
        export "$var=$value"
    done
    for var in $_vars; do
        eval "echo $var=\${$var}"
    done
fi

_var_source="$1/host/build/CPackConfig.cmake"
_vars="CPACK_PACKAGE_COPYRIGHT CPACK_PACKAGE_VENDOR"
python_cmd="$python_base_cmd cmake_source --cmake_file \"$_var_source\" --variables $_vars"
if [[ -f "$_var_source" ]]; then
    python -VV
    echo "Running Cmd: $python_cmd"
    eval $python_cmd | while IFS== read -r var value; do
        export "$var=$value"
    done
    for var in $_vars; do
        eval "echo $var=\${$var}"
    done
fi

_var_source="$1/host/build/cmake/Modules/UHDConfigVersion.cmake"
_vars="PACKAGE_VERSION MAJOR_VERSION API_VERSION ABI_VERSION PATCH_VERSION DEVEL_VERSION"
python_cmd="$python_base_cmd cmake_source --cmake_file \"$_var_source\" --variables $_vars"
if [[ -f "$_var_source" ]]; then
    python -VV
    echo "Running Cmd: $python_cmd"
    eval $python_cmd | while IFS== read -r var value; do
        export "$var=$value"
    done
    for var in $_vars; do
        eval "echo $var=\${$var}"
    done
fi