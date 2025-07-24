"""RFNoC Modtool: Template wrapper.

This is a wrapper around the Mako templating engine.
"""

#
# Copyright 2024 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

import mako.template


def render_wire_width(width, pad=8):
    """Render a wire's width (e.g., 8 -> [7:0]).

    If a pad value is provided, it will make the total width of the string as
    wide as the pad value.

    Examples:
    >>> render_wire_width(8, 10)
    "[     7:0]"
    >>> render_wire_width("WIDTH", 10) # Does not fit into 10 characters, will be longer
    "[WIDTH-1:0]"
    >>> len(render_wire_width(8, 10)) == 10
    True
    >>> render_wire_width(1, 4) # Single-bit wires don't need a width
    "    "

    Arguments:
        width: The wire width. If this is a string, it is interpreted as a
               Verilog constant and added verbatim.
        pad: Integer value, length of resulting string.
    """
    if isinstance(width, int):
        start_idx = width - 1
    else:
        start_idx = f"{width}-1"
    if start_idx == 0:
        return "".rjust(pad)
    range_str = f"{start_idx}:0".rjust(pad - 2)
    return f"[{range_str}]"


def quote(s):
    """Return a quoted string.

    The intention is to use this in templates to quote strings, such that they
    have quotes around them in the final output.
    """
    return f"'\"{s}\"'"


class Template(mako.template.Template):
    """Wrapper around the Mako template engine."""

    def __init__(self, *args, **kwargs):
        """Initialize the template.

        Differences to mako.template.Template:
        - strict_undefined is True by default
        """
        if "strict_undefined" not in kwargs:
            kwargs["strict_undefined"] = True
        super().__init__(*args, **kwargs)

    def render(self, **kwargs):
        """Render the template.

        This is identical to the parent class, but with the following extensions:

        - A 'quote' filter is provided. It is aliased to 'q'.
        """
        kwargs["quote"] = quote
        kwargs["q"] = quote
        return super().render(**kwargs)
