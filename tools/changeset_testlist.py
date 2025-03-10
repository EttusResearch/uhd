#!/usr/bin/env python3
#
# Copyright 2023 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""Changeset analyzer: Create list of required tests based on changes in a branch.

This script reads changes made on a particular branch compared to
another and computes a list of tests that need to be run to validate this branch.
"""

import argparse
import os
import pathlib
import re
import shutil
import subprocess
import sys

import requests
from ruamel import yaml


def parse_args():
    """Parse and return args."""
    parser = argparse.ArgumentParser(
        description=__doc__,
    )
    parser.add_argument(
        "--source-branch", help="Source branch off diff. Defaults to the current branch."
    )
    parser.add_argument(
        "--target-branch", default="master", help="Target branch off diff. Defaults to 'master'."
    )
    parser.add_argument(
        "--repo-path",
        default=".",
        help="Path to the UHD git repository. Defaults to the current directory.",
    )
    parser.add_argument("--rule-file", help="Path to rules file.")
    parser.add_argument("--set-azdo-var", help="Generate output to set an AzDO variable")
    parser.add_argument("--list-tests", action="store_true", help="Show the generated test-list.")
    parser.add_argument(
        "--include-target",
        action="store_true",
        help="Include changes that originate from the target branch.",
    )
    parser.add_argument(
        "--github-label-api-endpoint",
        help="Endpoint to use for GitHub API requests read back labels.",
    )
    parser.add_argument("--github-token", help="GitHub token to use for API requests.")
    parser.add_argument(
        "--remove-test", nargs="*", help="Remove a test from the list of tests.", default=[]
    )
    parser.add_argument(
        "--add-test", nargs="*", help="Add a test to the list of tests.", default=[]
    )
    parser.add_argument("-v", "--verbose", action="store_true", help="Verbose output")
    return parser.parse_args()


def get_git_commit_range(target_branch, source_branch=None, include_target=False):
    """Return the commit range arguments for git diff."""
    assert target_branch, "target_branch must be specified"
    separator = ".." if include_target else "..."
    if source_branch:
        return f"{target_branch}{separator}{source_branch}"
    else:
        return f"{target_branch}{separator}"


def check_changeset_content(file, **kwargs):
    """Check changeset content for code and/or comments changes.

    For example, if a .cpp file only has a comment change, we can use this to
    remove it from the "changed files" list. This is useful when we only want to
    only include tests if the underlying code was actually changed, and not just
    a comment (which cannot change the outcome of a hardware test).

    The values of the include_content argument have the following meaning:
    - 'code': at least one line with code changed
    - 'code-only': all changed lines must be code changes
    - 'comments': at least one line with comments changed
    - 'comments-only': all changed lines must be comment changes
    - 'code-and-comments': either code or comments changes (always True)

    Arguments:
    file: List of files that have been changed.
    repo_path: Path to the UHD repository.
    git_cmd: Path to the git command.
    include_content: allowed values are: code, code-only, comments,
        comments-only, code-and-comments

    Returns: True if the file should be included, False otherwise.
    """

    def identify_comment_line_cpp(line):
        """Identify if a line is a comment in a C++ file."""
        line = line.strip()
        return line.startswith("//") or line.startswith("/*") or line.startswith("*")

    def identify_comment_line_py(line):
        """Identify if a line is a comment in a Python file."""
        line = line.strip()
        return line.startswith("#")

    comment_identifer = {
        ".cpp": identify_comment_line_cpp,
        ".hpp": identify_comment_line_cpp,
        ".c": identify_comment_line_cpp,
        ".h": identify_comment_line_cpp,
        ".py": identify_comment_line_py,
    }

    include_content = kwargs.get("include_content", None)
    if include_content == "code-and-comments":
        # No need to check, result will always be true
        return True
    elif include_content in ["code", "code-only"]:
        invert = True
    elif include_content in ["comments", "comments-only"]:
        invert = False
    else:
        raise ValueError(f"Unsupported argument: include_content={include_content}")

    git_commit_range = get_git_commit_range(
        kwargs["target_branch"],
        kwargs.get("source_branch", None),
        kwargs.get("include_target", False)
    )
    get_diff_args = [
        shutil.which("git"),
        "-C",
        kwargs["repo_path"],
        "diff",
        "--no-color",
        "--unified=0",
        git_commit_range,
        "--",
    ]

    ext = os.path.splitext(file)[1]
    if ext not in comment_identifer:
        # If we have no rule to check, then always include the file.
        return True
    diff_lines = (
        subprocess.check_output(get_diff_args + [file], encoding="utf-8").strip().split("\n")[4:]
    )
    line_matches = [
        comment_identifer[ext](line[1:]) ^ invert for line in diff_lines if line[0] in ("-", "+")
    ]
    if include_content.endswith("-only"):
        return all(line_matches)
    else:
        return any(line_matches)


def get_changed_files(repo_path, target_branch, source_branch, include_target):
    """Return a list of paths in the UHD repository that have are different between two branches.

    Arguments:
    repo_path: Path to the UHD repository.
    target_branch: Branch to compare against (e.g., master).
    source_branch: Branch to compare from. Defaults to the current branch.
    include_target: Include changes that originate from the target branch.
    """
    git_commit_range = get_git_commit_range(target_branch, source_branch, include_target)
    git_cmd = shutil.which("git")
    get_diff_args = [git_cmd, "diff", "--name-only", git_commit_range]
    files = subprocess.check_output(get_diff_args, cwd=repo_path, encoding="utf-8")
    return files.strip().split("\n")


def load_rules(rule_file):
    """Return the rules as a Python list."""
    with open(rule_file, "r", encoding="utf-8") as rfd:
        y = yaml.YAML(typ="safe", pure=True)
        return y.load(rfd)


class RuleApplier:
    """Helper class to update an internal test list based on a set of rules."""

    def __init__(self, rules, labels, initial_list=[], **kwargs):
        """Initialize.

        Arguments:
        rules: List of rules to apply.
        labels: List of labels relevant to the current changeset.
        initial_list: Initial list of tests to apply rules against.
        """
        self.rules = rules
        self.labels = labels
        self.args = kwargs
        self.test_list = set(initial_list)

    def apply(self, filename, verbose=False):
        """Apply rules against a file."""
        for rule in self.rules:
            if self._apply_rule(rule, filename, verbose):
                break

    def apply_labels(self):
        """Apply rules based only on labels."""
        for rule in self.rules:
            if (
                "label" in rule
                and rule["label"] in self.labels
                and ("re" not in rule and "name" not in rule)
            ):
                self.test_list.update(rule.get("add", []))
                self.test_list.difference_update(rule.get("remove", []))
                for remove_re in rule.get("remove_re", []):
                    self.test_list.difference_update(
                        set([test for test in self.test_list if re.search(remove_re, test)])
                    )

    def _apply_rule(self, rule, filename, verbose=False):
        """Apply a single rule.

        Returns True if you can stop applying rules against this filename.
        """
        # First: Check if this rule even applies to this file
        if (
            ("re" in rule and not re.search(rule["re"], filename))
            or ("name" in rule and rule["name"] != filename)
            or ("name" not in rule and "re" not in rule)
        ):
            return False
        if verbose:
            sys.stderr.write(f"Filename {filename} matches rule: {rule}\n")
        if "label" in rule and not rule["label"] in self.labels:
            if verbose:
                sys.stderr.write(f"Skipping file based on missing label: {rule['label']}\n")
            return False
        if "label" in rule and verbose:
            sys.stderr.write(f"Label {rule['label']} found\n")
        include_content = rule.get("include_content", "code")
        if not check_changeset_content(filename, **self.args, include_content=include_content):
            if verbose:
                sys.stderr.write(
                    f"Skipping {filename} based on content rule: include_content='{include_content}'\n"
                )
            return False
        if "add" in rule:
            self.test_list.update(rule["add"])
        if "remove" in rule:
            self.test_list.difference_update(rule["remove"])
        # If stop is specified as False, then we can still apply more rules.
        if "stop" in rule and not rule["stop"]:
            return False
        return True

    def remove_tests(self, test_patterns, verbose=False):
        """Remove tests from the test list."""
        if verbose:
            sys.stderr.write(f"Removing tests based on patterns: {test_patterns}\n")
            sys.stderr.write(f"Before: {self.test_list}\n")
        for test_pattern in test_patterns:
            self.test_list = set(
                [test for test in self.test_list if not re.search(test_pattern, test)]
            )
        if verbose:
            sys.stderr.write(f"After: {self.test_list}\n")


def get_labels(api_endpoint, token):
    """Get a list of labels from the GitHub API."""
    if not api_endpoint:
        return []
    if token:
        headers = {"Authorization": f"Bearer {token}"}
    else:
        headers = {}
    data = requests.get(api_endpoint, headers=headers).json()
    try:
        labels = [label["name"] for label in data]
    except (KeyError, TypeError):
        print("WARNING: Could not get labels from API. Return data: ", data)
        return []
    print("Labels:", ", ".join(labels))
    return labels


def main():
    """Gogogo! Run script."""
    args = parse_args()
    rule_file = args.rule_file
    if not rule_file:
        rule_file = os.path.join(pathlib.Path(__file__).parent.resolve(), "changeset_testlist.yaml")
    file_list = get_changed_files(
        args.repo_path,
        args.target_branch,
        args.source_branch,
        args.include_target,
    )
    labels = get_labels(args.github_label_api_endpoint, args.github_token)
    rule_applier = RuleApplier(
        load_rules(rule_file),
        labels,
        initial_list=args.add_test,
        repo_path=args.repo_path,
        target_branch=args.target_branch,
        source_branch=args.source_branch,
        include_target=args.include_target,
        verbose=args.verbose,
    )
    for filename in file_list:
        rule_applier.apply(filename, args.verbose)
    rule_applier.apply_labels()
    if args.remove_test:
        rule_applier.remove_tests(args.remove_test, args.verbose)
    if args.set_azdo_var:
        print(
            f"##vso[task.setvariable variable={args.set_azdo_var};isoutput=true]"
            + ";".join(rule_applier.test_list)
        )
    if args.list_tests:
        print("Required tests:")
        print("---------------", end="")
        print("\n* ".join(sorted([""] + list(rule_applier.test_list))))


if __name__ == "__main__":
    sys.exit(main())
