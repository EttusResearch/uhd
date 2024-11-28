"""RFNoC Modtool: Template wrapper.

This is a wrapper around the Mako templating engine.
"""

#
# Copyright 2024 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

import mako.template


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
