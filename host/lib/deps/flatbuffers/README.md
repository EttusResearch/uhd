# FlatBuffers: Third-Party Dependency for UHD

Version: 1.11.0

FlatBuffers is a serialization library. We use it to serialize/deserialize
calibration data (and possibly other data) that we want to store in binary
format either on device EEPROMs, or on the local filesystem.

A full installation of FlatBuffers is not required to run UHD, and the headers
are only required to compile libuhd. Therefore, we ship a version of FlatBuffers
with UHD to avoid the requirement for users to install their own version of
FlatBuffers.

## License for FlatBuffers

As a separate, third-party project, FlatBuffers has a different license from UHD.
See the LICENSE file in the same directory as this readme. FlatBuffers has an
Apache license.

## Importing into UHD

In order to copy FlatBuffers into UHD, only the `include/` subdirectory from the
FlatBuffers repository was copied into UHD, along with the LICENSE file.

