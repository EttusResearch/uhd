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
import subprocess
import sys
import traceback
from argparse import Namespace
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
    if "glob" in kwargs and kwargs["glob"]:
        glob_pattern = kwargs["glob"]
        # FIXME: Maybe the user wants to glob somewhere else
        glob_pattern = os.path.join(os.getcwd(), glob_pattern)
        file_list = [
            f
            for f in glob.glob(kwargs["glob"], recursive=kwargs.get("glob_recursive", True))
            if Path(f).is_file()
        ]
    file_list += kwargs.get("files", [])
    if "file" in kwargs and kwargs["file"]:
        file_list.append(kwargs["file"])
    # Only return unique files that exist
    return [f for f in set(file_list) if os.path.isfile(f)]


class StepExecutor:
    """Main engine for executing steps in a command script."""

    class StepError(Exception):
        """Exception raised when a step fails."""

    def __init__(self, global_vars, args, cmd):
        """Initialize the executor."""
        assert isinstance(cmd, dict), "Command must be a dictionary"
        self.cmd = cmd
        self.args = args
        self.global_vars = global_vars
        self.template_base = os.path.join(os.path.dirname(__file__), "templates")
        self._log = logging.getLogger(__name__)

    def _setv(self, name, val):
        """Set variable name to val."""
        self.cmd["variables"][name] = val

    def _resolve(self, value, **extra_kwargs):
        """Shorthand for resolving a value."""
        return resolve(
            value,
            args=self.args,
            **self.global_vars,
            **self.cmd.get("variables", {}),
            os=os,
            **extra_kwargs,
        )

    def run(self, steps):
        """Run all the steps in the command."""
        for step in steps:
            for step_type, step_args in step.items():
                args_resolved = {}
                for k, v in step_args.items():
                    try:
                        args_resolved[k] = self._resolve(v)
                    except Exception as e:
                        raise StepExecutor.StepError(
                            f"Error resolving argument {k}={v} for step {step_type}: {str(e)}"
                        )
                args_resolved_visual = args_resolved.copy()
                if "steps" in args_resolved_visual:
                    args_resolved_visual["steps"] = str(len(args_resolved_visual["steps"]))
                self._log.debug(
                    "Executing step %s(%s)",
                    step_type,
                    ", ".join([f"{k}={v}" for k, v in args_resolved_visual.items()]),
                )
                try:
                    getattr(self, step_type)(**args_resolved)
                except StepExecutor.StepError as e:
                    self._log.error(str(e))
                    sys.exit(1)
                except Exception as e:
                    self._log.error("Unexpected error running step %s: %s", step_type, str(e))
                    self._log.debug(traceback.format_exc())
                    sys.exit(1)

    def run_if(self, condition, steps):
        """Run a block of steps if a condition is true."""
        if self._resolve(condition):
            self.run(steps)

    def run_subprocess(self, cmd, **kwargs):
        """Run a command as a subprocess.

        Arguments:
        - cmd: The command to run as a list of arguments.
        - dst_var: If provided, the output will be stored in this variable.
        """
        cmd = [self._resolve(arg) for arg in cmd]
        cmd = [arg for arg in cmd if arg]
        dst_var = kwargs.get("dst_var")
        try:
            self._log.debug("Running command: %s", cmd)
            proc_result = subprocess.run(
                cmd,
                check=True,
                env=os.environ.copy(),
                capture_output=dst_var is not None,
            )
        except subprocess.CalledProcessError as e:
            if "error_msg" in kwargs:
                self._log.error(kwargs["error_msg"])
            else:
                self._log.error("Command failed with return code %s", e.returncode)
            raise StepExecutor.StepError(f"Command failed: {e}")
        if dst_var:
            if proc_result.stdout:
                self._setv(dst_var, proc_result.stdout.decode("utf-8").strip())
            else:
                self._setv(dst_var, "")

    def copy_dir(self, src, dst, **kwargs):
        """Copy a directory from src to dest, recursively."""
        self._log.debug("Copying directory %s to %s", src, dst)
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
        """Search and replace text in a file.

        pattern: The pattern to search for
        repl: Replacement string
        count: Max number of replacements (default: 0, all)
        """
        file_list = get_file_list(**kwargs)
        for file in file_list:
            self._log.debug(
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
                self._log.warning("Pattern not found in file %s", os.path.abspath(file))
            with open(file, "w", encoding="utf-8") as f:
                f.write(contents)

    def chdir(self, dir):
        """Change the working directory."""
        os.chdir(os.path.normpath(dir))
        self.global_vars["CWD"] = os.getcwd()
        self._log.debug("Changed working directory to %s", os.getcwd())

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
                self._log.debug("Renaming %s to %s", path, new_path)
                shutil.move(path, new_path)

    def parse_descriptor(self, source, var="config", **kwargs):
        """Load a block descriptor file."""
        if not source:
            raise StepExecutor.StepError("Cannot parse: No descriptor file specified.")
        yaml = YAML(typ="safe", pure=True)
        self._log.debug("Loading descriptor file %s into variable %s", source, var)
        try:
            with open(source, "r", encoding="utf-8") as f:
                self.global_vars[var] = yaml.load(f)
        except FileNotFoundError:
            raise StepExecutor.StepError(f"Descriptor file {source} not found.")
        except Exception as e:
            raise StepExecutor.StepError(f"Error loading descriptor file {source}: {str(e)}")

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
        self._log.debug("Writing template %s to %s", template, dest)
        if os.path.exists(dest):
            self._log.warning("Overwriting existing file %s", dest)
        with open(dest, "w", encoding="utf-8") as f:
            f.write(tpl.render(**self.global_vars, **vars))

    def _insert_text(self, pattern, text, repl, **kwargs):
        """Insert text into a file based on a regex."""
        file_list = get_file_list(**kwargs)
        for file in file_list:
            self._log.debug("Editing file %s (inserting `%s')", file, text.strip())
            with open(file, "r", encoding="utf-8") as f:
                contents = f.read()
            count = kwargs.get("count", 1)
            contents, sub_count = re.subn(
                pattern, repl, contents, count=count, flags=re.MULTILINE | re.DOTALL
            )
            if sub_count == 0:
                self._log.warning("Pattern not found in file %s", file)
            with open(file, "w", encoding="utf-8") as f:
                f.write(contents)

    def insert_after(self, pattern, text, **kwargs):
        """Insert text after a pattern in a file."""
        self._insert_text(pattern, text, r"\g<0>" + text, **kwargs)

    def insert_before(self, pattern, text, **kwargs):
        """Insert text before a pattern in a file."""
        self._insert_text(pattern, text, text + r"\g<0>", **kwargs)

    def append(self, text, **kwargs):
        """Append text to a file."""
        file_list = get_file_list(**kwargs)
        for file in file_list:
            self._log.debug("Appending to file %s", file)
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
            self._log.debug("Commenting out lines in %s", file)
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
        self._log.debug("Removing directory %s", dir_to_remove)
        if os.path.exists(dir_to_remove) and os.path.isdir(dir_to_remove):
            shutil.rmtree(dir_to_remove)

    def log(self, **kwargs):
        """Print a message to the log."""
        level = kwargs.get("level", "info")
        message = kwargs.get("msg", "")
        symbol = kwargs.get("symbol", "✅")
        if level == "info":
            message = f"{symbol}   {message}"
        getattr(self._log, level)(message)

    def find_file(self, dst_var, required=False, **kwargs):
        """Find a file matching a pattern and store its path in a variable.

        Arguments:
        - glob/file/files: The pattern to match files against. See get_file_list for details.
        - dst_var: The name of the variable to store the path in.
        - required: If true, then an error is raised if no file is found. If
                    false, the variable is set to None if no file is found. This
                    is the default.
        - error_msg: The message to display if no file is found and required is true.
        """
        file_list = get_file_list(**kwargs)
        if len(file_list) > 1:
            raise StepExecutor.StepError(f"More than one file found matching pattern.")
        if len(file_list) == 0:
            if required:
                raise StepExecutor.StepError(
                    kwargs.get("error_msg", "No file found matching pattern.")
                )
            self.cmd["variables"][dst_var] = None
        else:
            self.cmd["variables"][dst_var] = file_list[0]

    def find_executable(self, dst_var, name, **kwargs):
        """Find an executable in the PATH and store it in a variable.

        Parameters:
        - dst_var: The name of the variable to store the executable in.
        - name: The name of the executable to find.

        Optional:
        - error_msg: The message to display if the executable is not found.
        """
        if "error_msg" in kwargs:
            error_msg = kwargs["error_msg"]
        else:
            error_msg = f"Executable {name} not found in PATH."
        path = shutil.which(name)
        if path is None:
            raise StepExecutor.StepError(error_msg)
        self._log.debug("Found executable %s at %s", name, path)
        self._setv(dst_var, os.path.abspath(path))

    def exit(self, **kwargs):
        """Exit the script."""
        if "msg" in kwargs:
            self._log.info(kwargs["msg"])
        sys.exit(kwargs.get("code", 1))

    def input(self, dst_var, prompt, **kwargs):
        """Request input from the user.

        The input is captured by calling input(). Then, the following rules apply:
        - If nothing was entered, and a default value is provided, the default
          is used and this function returns.
        - If a type is provided, the input is converted to that type. This may
          fail, in which case the user is prompted again.
        - Next, if a check is provided, the input is checked against the
          predicate. If the check fails, the user is prompted again.
        - When the user is prompted to re-enter the input, we use the string
          from "check_msg" to provide additional information.
        """
        default = kwargs.get("default")
        prompt = prompt.strip()
        if default:
            prompt += f" [{default}]"
        prompt += " "
        val = input(prompt)

        def _filter_bool(s):
            """A 'smart' conversion from string to Boolean."""
            if isinstance(s, str) and s.lower() in ["y", "yes", "1", "true", "on"]:
                return True
            else:
                return False

        filters = {"bool": _filter_bool, "int": lambda s: int(s, 0), "float": float}
        if val == "" and "default" in kwargs:
            self._setv(dst_var, kwargs["default"])
            return
        elif "type" in kwargs:
            if kwargs["type"] in filters:
                try:
                    val = filters[kwargs["type"]](val.strip())
                except:
                    val = None
            else:
                raise StepExecutor.StepError(f"Unknown input type {kwargs['type']}")
        else:
            val = val.strip()

        def _check_input(val):
            """Check if the input is valid."""
            if val is None:
                return False
            if "check" not in kwargs:
                return True
            try:
                check_val = self._resolve("${ " + kwargs["check"] + "}", **{dst_var: val})
                if check_val:
                    return True
            except:
                pass
            return False

        if not _check_input(val):
            check_msg = kwargs.get("check_msg", "Invalid input!")
            print(check_msg)
            # Try again
            return self.input(dst_var, prompt, **kwargs)

        self._setv(dst_var, val)

    def fork(self, command, args, **kwargs):
        """Fork a command and wait for it to finish.

        The commands that can be forked are other rfnoc_modtool subcommands.
        """
        from .rfnoc_modtool import collect_commands, resolve_vars

        if not isinstance(args, Namespace):
            args = Namespace(**{k: self._resolve(v) for k, v in args.items()})
        cmds = collect_commands()
        if command not in cmds:
            raise StepExecutor.StepError(f"Unknown command {command}")
        cmd = cmds[command]
        for arg_name, arg_info in cmd.get("args", {}).items():
            if arg_name not in args:
                if "default" in arg_info:
                    setattr(args, arg_name, arg_info["default"])
                else:
                    raise StepExecutor.StepError(f"Missing required argument {arg_name}")
        assert not cmd.get(
            "skip_identify_module", False
        ), "Cannot fork a command that skips module identification"
        cmd = resolve_vars(cmd, self.global_vars, args)
        sub_executor = StepExecutor(self.global_vars.copy(), args, cmd)
        self._log.debug(
            "Forking command %s(%s)",
            command,
            ", ".join([f"{k}={v}" for k, v in args.__dict__.items()]),
        )
        sub_executor.run(cmds[command]["steps"])

    def set(self, name, value, is_global=False, **kwargs):
        """Set a variable in the command script."""
        self._log.debug("Setting variable %s to %s", name, value)
        if is_global:
            self.global_vars[name] = self._resolve(value)
        else:
            self._setv(name, self._resolve(value))
