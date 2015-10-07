# Device Tests

These are a set of tests to be run with one or more attached devices.
None of these tests require special configuration; e.g., the X3x0 test
will work regardless of attached daughterboards, FPGIO wiring etc.

## Adding new tests

To add new tests, add new files with classes that derive from unittest.TestCase.
Most of the time, you'll want to derive from `uhd_test_case` or
`uhd_example_test_case`.

## Adding new devices

To add new devices, follow these steps:

1) Add an entry to the CMakeLists.txt file in this directory using the
   `ADD_DEVTEST()` macro.
2) Add a `devtest_pattern.py` file to this directory, where `pattern` is
   the same pattern used in the `ADD_DEVTEST()` macro.
3) Edit this devtest file to import all the tests you want to run. Some
   may require parameterization.

The devtest file is 'executed' using Python's unittest module, so it doesn't
require any actual commands. If the device needs special initialization,
commands inside this file will be executed *if* they are *not* in a
`if __name__ == "__main__"` conditional.

