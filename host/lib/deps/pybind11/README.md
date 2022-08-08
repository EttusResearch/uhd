# PyBind11: Third-Party Dependency for UHD

Version: 2.10.0 (git hash: aa304c9)

PyBind11 is a replacement for Boost.Python. Unlike Boost.Python, however, we
ship it with the UHD repository instead of relying on it to be there external to
the repository. The main reason for this is that PyBind11 is not packaged with
most distributions at this time. It also allows us to lock down the version of
PyBind11 used; with Boost, we often have to apply hacks to make sure UHD works
across all of the Boost versions that we support.

Note that the UHD CMake allows for using a different version of PyBind11 (e.g.,
the one installed via package manager).

## License for PyBind11

As a seperate, third-party project, PyBind11 has a different license from UHD.
See the LICENSE file in the same directory as this readme. PyBind11 has a
3-clause BSD-style license.

## Importing into UHD

In order to copy PyBind11 into UHD, only the `include/` subdirectory from the
PyBind11 repository was copied into UHD.
To save space, all comments were stripped from PyBind11 source files, except for
the copyright header, using the `remove_comments.py` script.
