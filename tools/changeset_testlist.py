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
    parser.add_argument("-v", "--verbose", action="store_true", help="Verbose output")
    return parser.parse_args()


def get_changed_files(repo_path, target_branch, source_branch, include_target):
    """
    Returns a list of paths in the UHD repository that have are different between
    two branches.
    """
    assert target_branch
    # If include_target is false, then current (unstaged/uncommited) changes are
    # not included. If we want to change this, then couple this with
    # git diff --name-only (no further arguments)
    if not include_target:
        target_branch += "..."
    git_cmd = shutil.which("git")
    get_diff_args = [git_cmd, "diff", "--name-only", target_branch]
    if source_branch:
        get_diff_args.append(source_branch)
    files = subprocess.check_output(get_diff_args, cwd=repo_path, encoding="utf-8")
    return files.strip().split("\n")


def load_rules(rule_file):
    """Return the rules as a Python list."""
    with open(rule_file, "r", encoding="utf-8") as rfd:
        y = yaml.YAML(typ="safe", pure=True)
        return y.load(rfd)


class RuleApplier:
    """Helper class to update an internal test list based on a set of rules."""

    def __init__(self, rules, labels):
        """Initialize.

        Arguments:
        rules: List of rules to apply.
        """
        self.rules = rules
        self.labels = labels
        self.test_list = set()

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
            return False
        if "label" in rule and verbose:
            sys.stderr.write(f"Label {rule['label']} found\n")
        if "add" in rule:
            self.test_list.update(rule["add"])
        if "remove" in rule:
            self.test_list.difference_update(rule["remove"])
        # If stop is specified as False, then we can still apply more rules.
        if "stop" in rule and not rule["stop"]:
            return False
        return True


def get_labels(api_endpoint, token):
    """Get a list of labels from the GitHub API."""
    if not api_endpoint:
        return []
    if token:
        headers = {"Authorization": f"Bearer {token}"}
    else:
        headers = {}
    data = requests.get(api_endpoint, headers=headers).json()
    labels = [label["name"] for label in data]
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
    rule_applier = RuleApplier(load_rules(rule_file), labels)
    for filename in file_list:
        rule_applier.apply(filename, args.verbose)
    rule_applier.apply_labels()
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
