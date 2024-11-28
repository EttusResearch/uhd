#!/usr/bin/env python3
"""RFNoC Modtool: Step Executor.

Contains the class that can run individual steps of a command script.
"""
#
# Copyright 2024 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

import datetime
import fnmatch
import glob
import logging
import os
import re
import shutil
import sys
from pathlib import Path

import mako.lookup
from ruamel.yaml import YAML

from .template import Template
from .utils import resolve


def get_file_list(**kwargs):
    """Extract a list of filenames from kwargs.

    The following rules apply:
    - If "glob" is present, use it to generate a list of filenames.
    - If "files" is present, use it as a list of filenames.
    - If "file" is present, use it as a single filename.
    """
    file_list = []
    if "glob" in kwargs:
        glob_pattern = kwargs["glob"]
        # FIXME: Maybe the user wants to glob somewhere else
        glob_pattern = os.path.join(os.getcwd(), glob_pattern)
        file_list = [
            f
            for f in glob.glob(kwargs["glob"], recursive=kwargs.get("glob_recursive", True))
            if Path(f).is_file()
        ]
    file_list += kwargs.get("files", [])
    if "file" in kwargs:
        file_list.append(kwargs["file"])
    return file_list


class StepExecutor:
    """Main engine for executing steps in a command script."""

    def __init__(self, global_vars, args, cmd):
        """Initialize the executor."""
        self.cmd = cmd
        self.args = args
        self.global_vars = global_vars
        self.template_base = os.path.join(os.path.dirname(__file__), "templates")
        self.log = logging.getLogger(__name__)

    def _resolve(self, value):
        """Shorthand for resolving a value."""
        return resolve(value, args=self.args, **self.global_vars, **self.cmd.get("variables", {}))

    def run(self, steps):
        """Run all the steps in the command."""
        for step in steps:
            for step_type, step_args in step.items():
                getattr(self, step_type)(**{k: self._resolve(v) for k, v in step_args.items()})

    def run_if(self, condition, steps):
        """Run a block of steps if a condition is true."""
        if self._resolve(condition):
            self.run(steps)

    def copy_dir(self, src, dst, **kwargs):
        """Copy a directory from src to dest, recursively."""
        self.log.debug("Copying directory %s to %s", src, dst)
        ignore = None
        if "ignore_globs" in kwargs:

            def ignore_patterns(path, names):
                relpath = os.path.relpath(path, os.getcwd())
                return [
                    name
                    for name in names
                    if any(
                        fnmatch.fnmatch(os.path.normpath(os.path.join(relpath, name)), pattern)
                        for pattern in kwargs["ignore_globs"]
                    )
                ]

            ignore = ignore_patterns
        copytree_kwargs = {"ignore": ignore}
        if sys.version_info >= (3, 8):
            copytree_kwargs["dirs_exist_ok"] = True
        shutil.copytree(src, dst, **copytree_kwargs)

    def search_and_replace(self, **kwargs):
        """Search and replace text in a file."""
        file_list = get_file_list(**kwargs)
        for file in file_list:
            self.log.debug(
                "Editing file %s (replacing `%s' with `%s')",
                file,
                kwargs["pattern"],
                kwargs["repl"],
            )
            with open(file, "r", encoding="utf-8") as f:
                contents = f.read()
            count = kwargs.get("count", 0)
            contents, sub_count = re.subn(
                kwargs["pattern"],
                kwargs["repl"],
                contents,
                count=count,
                flags=re.MULTILINE | re.DOTALL,
            )
            if sub_count == 0 and not kwargs.get("quiet", False):
                self.log.warning("Pattern not found in file %s", os.path.abspath(file))
            with open(file, "w", encoding="utf-8") as f:
                f.write(contents)

    def chdir(self, dir):
        """Change the working directory."""
        os.chdir(os.path.normpath(dir))
        self.global_vars["CWD"] = os.getcwd()
        self.log.debug("Changed working directory to %s", os.getcwd())

    def multi_rename(self, pattern, repl, **kwargs):
        """Rename multiple files with a regex pattern."""
        paths_to_rename = glob.glob(kwargs["glob"], recursive=kwargs.get("glob_recursive", True))
        # make sure files are renamed before the directories which contain the files by
        # sorting and reversing the list, example:
        # - first rename file "old/old.cc" to "old/new.cc"
        # - then rename directory "old" to "new"
        paths_to_rename.sort(reverse=True)
        for path in paths_to_rename:
            head, tail = os.path.split(path)
            new_path = os.path.join(head, re.sub(pattern, repl, tail))
            if new_path != path:
                self.log.debug("Renaming %s to %s", path, new_path)
                shutil.move(path, new_path)

    def parse_descriptor(self, source, var="config", **kwargs):
        """Load a block descriptor file."""
        yaml = YAML(typ="safe", pure=True)
        self.log.debug("Loading descriptor file %s into variable %s", source, var)
        with open(source, "r", encoding="utf-8") as f:
            self.global_vars[var] = yaml.load(f)

    def write_template(self, template, dest, **kwargs):
        """Write a template file."""
        template_dir = os.path.join(
            self.template_base, kwargs.get("namespace", self.cmd.get("template_namespace", ""))
        )
        lookup = mako.lookup.TemplateLookup(directories=[template_dir])
        Path(dest).parent.mkdir(parents=True, exist_ok=True)
        tpl = Template(filename=os.path.join(template_dir, template), lookup=lookup)
        vars = self.cmd.get("variables", {}).copy()
        # Make sure standard template variables are available
        if "year" in kwargs:
            vars["year"] = kwargs["year"]
        elif "year" in vars:
            pass
        else:
            vars["year"] = datetime.datetime.now().year
        self.log.debug("Writing template %s to %s", template, dest)
        with open(dest, "w", encoding="utf-8") as f:
            f.write(tpl.render(**self.global_vars, **vars))

    def insert_after(self, pattern, text, **kwargs):
        """Insert text after a pattern in a file."""
        file_list = get_file_list(**kwargs)
        for file in file_list:
            self.log.debug("Editing file %s (inserting `%s')", file, text.strip())
            with open(file, "r", encoding="utf-8") as f:
                contents = f.read()
            count = kwargs.get("count", 1)
            contents, sub_count = re.subn(
                pattern, r"\g<0>" + text, contents, count=count, flags=re.MULTILINE | re.DOTALL
            )
            if sub_count == 0:
                self.log.warning("Pattern not found in file %s", file)
            with open(file, "w", encoding="utf-8") as f:
                f.write(contents)

    def insert_before(self, pattern, text, **kwargs):
        """Insert text after a pattern in a file."""
        file_list = get_file_list(**kwargs)
        for file in file_list:
            self.log.debug("Editing file %s (inserting `%s')", file, text.strip())
            with open(file, "r", encoding="utf-8") as f:
                contents = f.read()
            count = kwargs.get("count", 1)
            contents, sub_count = re.subn(
                pattern, text + r"\g<0>", contents, count=count, flags=re.MULTILINE | re.DOTALL
            )
            if sub_count == 0:
                self.log.warning("Pattern not found in file %s", file)
            with open(file, "w", encoding="utf-8") as f:
                f.write(contents)

    def append(self, text, **kwargs):
        """Append text to a file."""
        file_list = get_file_list(**kwargs)
        for file in file_list:
            self.log.debug("Appending to file %s", file)
            with open(file, "a", encoding="utf-8") as f:
                f.write(text)

    def comment_out(self, character="#", **kwargs):
        """Modify all lines in range to prepend character.

        Note that the line range starts at 1, as it would with sed or in an
        editor.
        """
        line_range = kwargs.get("range", [1, -1])
        file_list = get_file_list(**kwargs)
        for file in file_list:
            self.log.debug("Commenting out lines in %s", file)
            with open(file, "r", encoding="utf-8") as f:
                contents = f.readlines()
            for line_no_minus_one, line in enumerate(contents):
                if line_no_minus_one + 1 >= line_range[0] and (
                    line_no_minus_one + 1 <= line_range[1] or line_range[1] == -1
                ):
                    contents[line_no_minus_one] = character + line
            with open(file, "w", encoding="utf-8") as f:
                f.writelines(contents)

    def rmtree(self, **kwargs):
        """Remove a directory tree."""
        dir_to_remove = kwargs["dir"]
        self.log.debug("Removing directory %s", dir_to_remove)
        if os.path.exists(dir_to_remove) and os.path.isdir(dir_to_remove):
            shutil.rmtree(dir_to_remove)
