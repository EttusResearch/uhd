#
# Copyright 2022 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Provides class CompatNumber, a convenience class for handling compat numbers.
"""

class CompatNumber:
    """
    Utility class for handling compat numbers.

    CompatNumber instances can be generated from floats, from string
    representations of floats, from two integers, and from tuples (when
    instantiating from tuples, only the first two elements are used and are
    interpreted as major, minor. The third element is used as a build
    timestamp, and is ignored for compat number comparisons).
    These are identical:

    >>> cn = CompatNumber(4, 3)
    >>> cn = CompatNumber(4.3)
    >>> cn = CompatNumber("4.3")
    >>> cn = CompatNumber((4, 3, 1234)) # Only first two elements are used!
    >>> cn = CompatNumber((4, 3, 5678))

    Compat numbers can be compared using regular operators. All of the following
    statements are True:

    >>> cn < CompatNumber(4, 5)
    >>> cn == 4.3
    >>> cn >= 3
    """

    def __init__(self, major, minor=None):
        if minor is None and isinstance(major, tuple):
            self.major = major[0]
            self.minor = major[1]
        elif minor is None:
            major, minor = str(float(major)).split('.', 1)
            self.major, self.minor = int(major), int(minor)
        else:
            if not isinstance(major, int) or not isinstance(minor, int):
                raise ValueError("Invalid major/minor values!")
            self.major = major
            self.minor = minor

    def __str__(self):
        return f"{self.major}.{self.minor}"

    def __repr__(self):
        return f"CompatNumber({self.major}, {self.minor})"

    def __eq__(self, rhs):
        if not isinstance(rhs, CompatNumber):
            return self == CompatNumber(rhs)
        return self.major == rhs.major and self.minor == rhs.minor

    def __gt__(self, rhs):
        if not isinstance(rhs, CompatNumber):
            return self > CompatNumber(rhs)
        return self.major > rhs.major or \
                (self.major == rhs.major and self.minor > rhs.minor)

    def __lt__(self, rhs):
        if not isinstance(rhs, CompatNumber):
            return self < CompatNumber(rhs)
        return self.major < rhs.major or \
                (self.major == rhs.major and self.minor < rhs.minor)

    def __ge__(self, rhs):
        return self == rhs or self > rhs

    def __le__(self, rhs):
        return self == rhs or self < rhs

if __name__ == "__main__":
    c = CompatNumber(4.3)
    d = CompatNumber(4.4)
    e = CompatNumber(5.1)
    print(c)
    print(c == d)
    print(c != 4.3)
    print(c == 4.4)
    print(e > d)
