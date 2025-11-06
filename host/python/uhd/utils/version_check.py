#
# Copyright 2024 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Implements version check for a windows file including version comparison functionality.
"""

from ctypes import ( POINTER, byref , c_char, c_uint, cast, pointer, sizeof, Structure, windll, WinError)
from ctypes.wintypes import (
    BOOL, DWORD, LPCVOID, LPCWSTR, LPVOID
)

# Contains version information for a windows file to be accessed by windll.version.* Win32 API functions.
class VS_FIXEDFILEINFO(Structure):
    _fields_ = [
        ("dwSignature", DWORD),  # will be 0xFEEF04BD
        ("dwStrucVersion", DWORD),
        ("dwFileVersionMS", DWORD),
        ("dwFileVersionLS", DWORD),
        ("dwProductVersionMS", DWORD),
        ("dwProductVersionLS", DWORD),
        ("dwFileFlagsMask", DWORD),
        ("dwFileFlags", DWORD),
        ("dwFileOS", DWORD),
        ("dwFileType", DWORD),
        ("dwFileSubtype", DWORD),
        ("dwFileDateMS", DWORD),
        ("dwFileDateLS", DWORD)
    ]


def get_windll_version(filename):
    """
    Retrieve the version information of a specified file.
    Args:
        filename (str): The path to the file for which version information is to be retrieved.
    Returns:
        str: The version of the file in the format "major.minor.build.revision".
    Raises:
        WinError: If the operating system cannot retrieve version information for the file.
    """

    # Check whether the operating system can retrieve version information for the file. If not, raise an error.
    dwLen = windll.version.GetFileVersionInfoSizeW(filename, None)
    if not dwLen:
        raise WinError()
    
    # Retrieve version information for the file and store it into lpData buffer
    lpData = (c_char * dwLen)()
    if not windll.version.GetFileVersionInfoW(filename, 0, sizeof(lpData), lpData):
        raise WinError()
    
    # Retrieve version information from lpData
    uLen = c_uint()
    lpffi = POINTER(VS_FIXEDFILEINFO)()
    lplpBuffer = cast(pointer(lpffi), POINTER(LPVOID))
    if not windll.version.VerQueryValueW(lpData, u"\\", lplpBuffer, byref(uLen)):
        raise WinError()
    
    # dwFileVersion defines the binary version number for the file. The version consists of two 32-bit integers, 
    # defined by four 16-bit integers.
    ffi = lpffi.contents
    major = ffi.dwFileVersionMS >> 16
    minor = ffi.dwFileVersionMS & 0xFFFF
    build = ffi.dwFileVersionLS >> 16
    revision = ffi.dwFileVersionLS & 0xFFFF
    return f"{major}.{minor}.{build}.{revision}"


def compare_versions(version1, version2):
    """
    Compare two version strings.
    This function compares two version strings in the format 'X.Y.Z' where X, Y, and Z are integers.
    It considers only the first three components of the version strings.
    Parameters:
    version1 (str): The first version string to compare.
    version2 (str): The second version string to compare.
    Returns:
    int: -1 if version1 is less than version2, 1 if version1 is greater than version2, 0 if they are equal.
    """
    #print(f"compare {version1} against {version2}")

    # Extract the first three digits
    v1 = (version1.split('.'))[:3]
    v2 = (version2.split('.'))[:3]
    
    # Convert to tuple of integers for comparison
    v1_tuple = tuple(map(int, v1))
    v2_tuple = tuple(map(int, v2))
    
    # return the comparison result
    if v1_tuple < v2_tuple:
        return -1
    elif v1_tuple > v2_tuple:
        return 1
    else:
        return 0
