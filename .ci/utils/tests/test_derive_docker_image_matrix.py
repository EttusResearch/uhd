#!/usr/bin/env python3
# ruff: noqa: D100,D101,D102,E402
#
# Copyright 2026 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""Tests for deriving package matrices from DockerImageNames artifact content.

Supports Python 3.8 or later.
"""

# pylint: disable=wrong-import-position

from __future__ import annotations

import argparse
import ast
import subprocess
import sys
import unittest
import warnings
from pathlib import Path
from typing import Optional

if sys.version_info < (3, 8):
    raise RuntimeError("This script requires Python 3.8 or later")

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

DEFAULT_FIXTURE = (
    Path(__file__).resolve().parent / "inputs" / "DockerImageNames" / "DockerImageNames"
)
ARTIFACT_FILE_OVERRIDE: Optional[Path] = None


class DeriveDockerImageMatrixTests(unittest.TestCase):
    """Validate docker image matrix derivation behavior for current artifacts."""

    @classmethod
    def setUpClass(cls):
        """Resolve and report the artifact file once per test run."""
        cls.fixture = ARTIFACT_FILE_OVERRIDE or DEFAULT_FIXTURE
        print(f"Artifact file: {cls.fixture}")

    def setUp(self):
        """Load DockerImageNames from override path or default fixture."""
        from derive_docker_image_matrix import (  # type: ignore[import-not-found]
            derive_typed_matrix,
            derive_package_matrix,
            get_artifact_value,
            parse_docker_image_names_file,
        )

        self.derive_typed_matrix = derive_typed_matrix
        self.derive_package_matrix = derive_package_matrix
        self.get_artifact_value = get_artifact_value
        self.parse_docker_image_names_file = parse_docker_image_names_file

        self.fixture = self.__class__.fixture
        self.using_artifact_override = ARTIFACT_FILE_OVERRIDE is not None
        self.artifact_data = self.parse_docker_image_names_file(self.fixture)
        self._warn_for_empty_optional_matrices()

    def _warn_for_empty_optional_matrices(self) -> None:
        """Warn if expected matrix sections are present but empty."""
        line_map = {
            "linux_matrix": 3,
            "win_matrix": 4,
            "macos_builders": 5,
        }
        for key, line_no in line_map.items():
            value = self.artifact_data.get(key)
            if isinstance(value, dict) and not value:
                warnings.warn(
                    f"DockerImageNames line {line_no} ({key}) is an empty dict; "
                    "tests requiring populated matrix entries may be skipped.",
                    UserWarning,
                )

    def _skip_if_matrix_empty(self, key: str, line_no: int) -> None:
        """Skip tests that need populated matrix content when section is empty."""
        value = self.artifact_data.get(key)
        if isinstance(value, dict) and not value:
            warnings.warn(
                f"Skipping test because DockerImageNames line {line_no} ({key}) is empty.",
                UserWarning,
            )
            self.skipTest(f"Empty matrix at line {line_no}: {key}")

    def test_parses_fixture_file(self):
        """Parse the fixture and expose expected top-level fields."""
        self.assertTrue(self.artifact_data["docker_build_number"])
        self._skip_if_matrix_empty("linux_matrix", 3)
        self.assertIn("Ubuntu-2404-builder", self.artifact_data["linux_matrix"])

    def test_derives_deb_matrix_from_linux_matrix(self):
        """Derive Ubuntu deb matrix using releaseName from linux matrix entries."""
        self._skip_if_matrix_empty("linux_matrix", 3)
        matrix = self.derive_package_matrix(self.artifact_data, os_name="linux", package="deb")
        self.assertIn("Ubuntu-2204-builder", matrix)
        self.assertIn("Ubuntu-2404-builder", matrix)
        self.assertIn("Ubuntu-2510-builder", matrix)
        self.assertEqual(matrix["Ubuntu-2204-builder"]["ubuntuReleaseName"], "jammy")
        self.assertEqual(matrix["Ubuntu-2404-builder"]["ubuntuReleaseName"], "noble")

    def test_derives_rpm_matrix_from_linux_matrix(self):
        """Derive Fedora rpm matrix using releaseName from linux matrix entries."""
        self._skip_if_matrix_empty("linux_matrix", 3)
        matrix = self.derive_package_matrix(self.artifact_data, os_name="linux", package="rpm")
        self.assertIn("Fedora-42-builder", matrix)
        self.assertEqual(matrix["Fedora-42-builder"]["fedoraReleaseName"], "42")
        self.assertEqual(matrix["Fedora-44-builder"]["fedoraReleaseName"], "44")

    def test_filter_out_matches_release_name(self):
        """Exclude entries when filter-out matches a derived release field."""
        self._skip_if_matrix_empty("linux_matrix", 3)
        matrix = self.derive_package_matrix(
            self.artifact_data,
            os_name="linux",
            package="deb",
            filter_out="questing,focal",
        )
        self.assertNotIn("Ubuntu-2510-builder", matrix)
        self.assertNotIn("Ubuntu-2004-builder", matrix)
        self.assertIn("Ubuntu-2404-builder", matrix)

    def test_invalid_os_package_combination_rejected(self):
        """Reject unsupported OS/package combinations."""
        with self.assertRaises(ValueError):
            self.derive_package_matrix(self.artifact_data, os_name="windows", package="deb")

    def test_build_type_derivation_with_filter_out(self):
        """Build type returns linux matrix and honors filter_out by releaseName."""
        self._skip_if_matrix_empty("linux_matrix", 3)
        matrix = self.derive_typed_matrix(
            self.artifact_data,
            target_type="build",
            os_name="linux",
            filter_out="questing",
        )
        self.assertNotIn("Ubuntu-2510-builder", matrix)
        self.assertIn("Ubuntu-2404-builder", matrix)

    def test_build_type_derivation_for_windows_returns_win_matrix(self):
        """Build type windows path should return the windows matrix unchanged."""
        matrix = self.derive_typed_matrix(
            self.artifact_data,
            target_type="build",
            os_name="windows",
        )
        self.assertEqual(matrix, self.artifact_data["win_matrix"])

    def test_source_type_select_returns_single_entry(self):
        """Source type should return a single selected Ubuntu entry."""
        self._skip_if_matrix_empty("linux_matrix", 3)
        matrix = self.derive_typed_matrix(
            self.artifact_data,
            target_type="source",
            os_name="linux",
            select="noble",
        )
        self.assertEqual(set(matrix.keys()), {"Ubuntu-2404-builder"})
        self.assertEqual(matrix["Ubuntu-2404-builder"]["ubuntuReleaseName"], "noble")

    def test_source_type_requires_select(self):
        """Source/docs types should require explicit select value."""
        with self.assertRaises(ValueError):
            self.derive_typed_matrix(
                self.artifact_data,
                target_type="source",
                os_name="linux",
            )

    def test_source_type_select_must_resolve_single_entry(self):
        """Source/docs select must resolve to exactly one entry."""
        with self.assertRaises(ValueError):
            self.derive_typed_matrix(
                self.artifact_data,
                target_type="source",
                os_name="linux",
                select="jammy,noble",
            )

    def test_select_and_filter_out_are_mutually_exclusive(self):
        """Type derivation should reject using select and filter_out together."""
        with self.assertRaises(ValueError):
            self.derive_typed_matrix(
                self.artifact_data,
                target_type="build",
                os_name="linux",
                select="noble",
                filter_out="questing",
            )

    def test_get_artifact_value_returns_windows_matrix(self):
        """Unit-level get_artifact_value path should return windows matrix section."""
        value = self.get_artifact_value(self.artifact_data, "windows-matrix")
        self.assertEqual(value, self.artifact_data["win_matrix"])

    def test_cli_section_returns_windows_matrix(self):
        """CLI --section path should return windows matrix via get_artifact_value."""
        script_path = Path(__file__).resolve().parents[1] / "derive_docker_image_matrix.py"
        result = subprocess.run(
            [
                sys.executable,
                str(script_path),
                "--input",
                str(self.fixture),
                "--section",
                "windows-matrix",
            ],
            check=False,
            capture_output=True,
            text=True,
        )

        self.assertEqual(result.returncode, 0, msg=result.stderr)
        parsed = ast.literal_eval(result.stdout.strip())
        self.assertEqual(parsed, self.artifact_data["win_matrix"])


if __name__ == "__main__":
    # Run this file directly, or use unittest discovery from the repo root.
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument(
        "--artifact-file",
        default="",
        help="Path to generated DockerImageNames file",
    )
    args, remaining_args = parser.parse_known_args()

    if args.artifact_file:
        ARTIFACT_FILE_OVERRIDE = Path(args.artifact_file)

    unittest.main(argv=[sys.argv[0], *remaining_args])
