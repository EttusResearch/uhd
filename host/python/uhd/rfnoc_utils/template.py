"""RFNoC Modtool: Template wrapper.

This is a wrapper around the Mako templating engine.
"""

#
# Copyright 2024 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

import mako.template


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

        - n/a
        """
        return super().render(**kwargs)
