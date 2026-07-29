#!/usr/bin/env python3
#
# Copyright 2026 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""Derive package-specific docker image matrices from DockerImageNames artifact data.

Supports Python 3.8 or later.

Strictness control:
- `UHD_MATRIX_STRICT_CI=true|1|yes|on`: force strict mode (errors are blocking)
- `UHD_MATRIX_STRICT_CI=false|0|no|off`: force non-strict mode
- `UHD_MATRIX_STRICT_CI` unset: auto-detect strictness
    - strict for Azure CI reasons `IndividualCI`, `BatchedCI`, `Schedule`
    - non-strict for Azure manual/debug runs
    - for non-Azure environments, strict if `CI=true`

When `--empty-on-error-non-ci` is provided, non-strict mode converts derivation
errors to `{}` and emits an Azure warning log issue.
"""

from __future__ import annotations

import argparse
import ast
import os
import sys
from pathlib import Path
from typing import Any

EXPECTED_ARTIFACT_LINES = 6

PACKAGE_OS_MAP = {
    "deb": "linux",
    "rpm": "linux",
    "macos": "macos",
    "nsis": "windows",
}

TYPE_OS_MAP = {
    "build": {"linux", "windows", "macos"},
    "source": {"linux"},
    "docs": {"linux"},
    "installer": {"linux", "windows", "macos"},
}

SECTION_DATA_MAP = {
    "docker-build-number": "docker_build_number",
    "docker-image-prefix": "docker_image_prefix",
    "linux-matrix": "linux_matrix",
    "windows-matrix": "win_matrix",
    "macos-builders": "macos_builders",
    "windows-python-build-matrix": "win_python_build_matrix",
}


def is_strict_ci_build() -> bool:
    """Return True when matrix validation/derivation failures are blocking.

    Override behavior via `UHD_MATRIX_STRICT_CI`:
      - true/1/yes/on: force strict
      - false/0/no/off: force non-strict
      - auto/unset: use environment-based detection
    """
    override = os.getenv("UHD_MATRIX_STRICT_CI", "").strip().lower()
    if override in {"1", "true", "yes", "on"}:
        return True
    if override in {"0", "false", "no", "off"}:
        return False

    if os.getenv("TF_BUILD", "").strip().lower() == "true":
        return os.getenv("BUILD_REASON", "").strip() in {
            "IndividualCI",
            "BatchedCI",
            "Schedule",
        }

    # Non-Azure fallback used by local and other CI systems.
    return os.getenv("CI", "").strip().lower() == "true"


def _is_strict_ci_build() -> bool:
    """Backward-compatible internal alias."""
    return is_strict_ci_build()


def _empty_fallback_message(exc: Exception) -> str:
    """Format a consistent warning message for non-strict fallback."""
    return (
        "derive_docker_image_matrix failed in non-CI build; "
        "defaulting to empty object. Error: "
        f"{exc}"
    )


def _emit_non_strict_fallback(exc: Exception) -> int:
    """Emit warning and fallback output for non-strict mode."""
    message = _empty_fallback_message(exc)
    print(f"##vso[task.logissue type=warning]{message}", file=sys.stderr)
    print("{}")
    return 0


def _emit_failure(exc: Exception) -> int:
    """Emit normal error output for strict mode."""
    print(str(exc), file=sys.stderr)
    return 1


def _error_exit_for_exception(exc: Exception, empty_on_error_non_ci: bool) -> int:
    """Map an exception to exit handling based on strictness and CLI mode."""
    if empty_on_error_non_ci and not is_strict_ci_build():
        return _emit_non_strict_fallback(exc)
    return _emit_failure(exc)


def _format_cli_output(args: argparse.Namespace, matrix_text: str) -> None:
    """Write normal CLI output, either plain text or Azure variable format."""
    if args.vso_variable:
        print(f"##vso[task.setvariable variable={args.vso_variable};isOutput=true;]{matrix_text}")
    else:
        print(matrix_text)


def _derive_cli_value(args: argparse.Namespace, artifact_data: dict[str, Any]) -> Any:
    """Resolve the selected CLI mode into an artifact value/matrix."""
    if args.section:
        return get_artifact_value(artifact_data, args.section)
    if args.type:
        if not args.os:
            raise ValueError("--os is required when --type is specified")
        return derive_typed_matrix(
            artifact_data,
            args.type,
            args.os,
            package=args.package,
            select=args.select,
            filter_out=args.filter_out,
        )

    if not args.package:
        raise ValueError("Either --section or --type or --package must be specified")
    if not args.os:
        raise ValueError("--os is required when --package is specified")
    if args.select:
        raise ValueError("--select is only supported with --type")
    return derive_package_matrix(artifact_data, args.os, args.package, args.filter_out)


def _parse_matrix_string(raw_value: str) -> dict[str, dict[str, Any]]:
    if not raw_value:
        return {}
    parsed = ast.literal_eval(raw_value)
    if not isinstance(parsed, dict):
        raise ValueError("Expected matrix content to parse as a dict")
    return parsed


def parse_docker_image_names_file(file_path: Path) -> dict[str, Any]:
    """Parse DockerImageNames artifact content into typed matrix components."""
    lines = file_path.read_text(encoding="utf-8").splitlines()
    if len(lines) != EXPECTED_ARTIFACT_LINES:
        raise ValueError(
            f"Expected exactly {EXPECTED_ARTIFACT_LINES} lines in artifact, got {len(lines)}"
        )

    return {
        "docker_build_number": lines[0].strip(),
        "docker_image_prefix": lines[1].strip(),
        "linux_matrix": _parse_matrix_string(lines[2].strip()),
        "win_matrix": _parse_matrix_string(lines[3].strip()),
        "macos_builders": _parse_matrix_string(lines[4].strip()),
        "win_python_build_matrix": _parse_matrix_string(lines[5].strip()),
    }


def _validate_os_package(os_name: str, package: str) -> None:
    expected_os = PACKAGE_OS_MAP.get(package)
    if expected_os is None:
        supported = ", ".join(sorted(PACKAGE_OS_MAP.keys()))
        raise ValueError(f"Unsupported package '{package}'. Supported packages: {supported}")
    if os_name != expected_os:
        raise ValueError(
            f"Invalid os/package combination: os='{os_name}', package='{package}'. "
            f"Expected os='{expected_os}' for package '{package}'."
        )


def _derive_deb_matrix(linux_matrix: dict[str, dict[str, Any]]) -> dict[str, dict[str, Any]]:
    derived: dict[str, dict[str, Any]] = {}
    for key, payload in linux_matrix.items():
        if not key.startswith("Ubuntu-"):
            continue

        ubuntu_release = str(payload.get("releaseName", ""))
        if not ubuntu_release:
            continue

        enriched = dict(payload)
        enriched["ubuntuReleaseName"] = ubuntu_release
        derived[key] = enriched
    return derived


def _derive_rpm_matrix(linux_matrix: dict[str, dict[str, Any]]) -> dict[str, dict[str, Any]]:
    derived: dict[str, dict[str, Any]] = {}
    for key, payload in linux_matrix.items():
        if not key.startswith("Fedora-"):
            continue

        fedora_release = str(payload.get("releaseName", ""))
        if not fedora_release:
            continue

        enriched = dict(payload)
        enriched["fedoraReleaseName"] = fedora_release
        derived[key] = enriched
    return derived


def _normalize_filter_values(filter_out: str | None) -> set[str]:
    if not filter_out:
        return set()
    values = [token.strip().lower() for token in filter_out.split(",")]
    return {token for token in values if token}


def _normalize_select_values(select: str | None) -> set[str]:
    if not select:
        return set()
    values = [token.strip().lower() for token in select.split(",")]
    return {token for token in values if token}


def _entry_fields_for_filter(key: str, payload: dict[str, Any]) -> set[str]:
    fields = {
        key,
        str(payload.get("dockerImageName", "")),
        str(payload.get("releaseName", "")),
    }
    return {field.lower() for field in fields if field}


def _apply_select(
    matrix: dict[str, dict[str, Any]], select: str | None
) -> dict[str, dict[str, Any]]:
    select_values = _normalize_select_values(select)
    if not select_values:
        return matrix

    selected: dict[str, dict[str, Any]] = {}
    for key, payload in matrix.items():
        fields = _entry_fields_for_filter(key, payload)
        if select_values.intersection(fields):
            selected[key] = payload
    return selected


def _apply_filter_out(
    matrix: dict[str, dict[str, Any]], filter_out: str | None
) -> dict[str, dict[str, Any]]:
    filter_values = _normalize_filter_values(filter_out)
    if not filter_values:
        return matrix

    filtered: dict[str, dict[str, Any]] = {}
    for key, payload in matrix.items():
        fields = _entry_fields_for_filter(key, payload)
        if filter_values.intersection(fields):
            continue
        filtered[key] = payload
    return filtered


def derive_package_matrix(
    artifact_data: dict[str, Any], os_name: str, package: str, filter_out: str | None = None
) -> dict[str, dict[str, Any]]:
    """Derive package-specific matrix from parsed DockerImageNames artifact data."""
    os_name = os_name.lower()
    package = package.lower()
    _validate_os_package(os_name, package)

    if package == "deb":
        matrix = _derive_deb_matrix(artifact_data["linux_matrix"])
    elif package == "rpm":
        matrix = _derive_rpm_matrix(artifact_data["linux_matrix"])
    elif package == "macos":
        matrix = artifact_data["macos_builders"]
    elif package == "nsis":
        matrix = artifact_data["win_matrix"]
    else:
        raise ValueError(f"Unsupported package '{package}'")

    return _apply_filter_out(matrix, filter_out)


def _validate_type_os(target_type: str, os_name: str) -> None:
    supported_os = TYPE_OS_MAP.get(target_type)
    if supported_os is None:
        supported = ", ".join(sorted(TYPE_OS_MAP.keys()))
        raise ValueError(f"Unsupported type '{target_type}'. Supported types: {supported}")
    if os_name not in supported_os:
        supported = ", ".join(sorted(supported_os))
        raise ValueError(
            f"Invalid os/type combination: os='{os_name}', type='{target_type}'. "
            f"Supported os values for this type: {supported}"
        )


def _derive_source_or_docs_matrix(
    linux_matrix: dict[str, dict[str, Any]], select: str | None
) -> dict[str, dict[str, Any]]:
    ubuntu_matrix = _derive_deb_matrix(linux_matrix)
    if not select:
        raise ValueError("--select is required for --type source and --type docs")

    selected = _apply_select(ubuntu_matrix, select)
    if len(selected) != 1:
        raise ValueError(
            "--type source/docs must resolve to exactly one matrix entry; "
            f"got {len(selected)} for --select='{select}'"
        )
    return selected


def derive_typed_matrix(
    artifact_data: dict[str, Any],
    target_type: str,
    os_name: str,
    package: str | None = None,
    select: str | None = None,
    filter_out: str | None = None,
) -> dict[str, dict[str, Any]]:
    """Derive matrix data using a high-level type selector.

    Selection/filter behavior:
    1. `--select` and `--filter-out` are mutually exclusive.
    2. For `build` and `installer`: apply `select` (include list) when present,
       otherwise apply `filter_out` (exclude list).
     3. For `source` and `docs`: `select` is required and must resolve to
         exactly one Ubuntu entry.
    4. Match fields for both selectors are: matrix key, `releaseName`,
       and `dockerImageName` (case-insensitive exact token match).
    """
    target_type = target_type.lower()
    os_name = os_name.lower()
    _validate_type_os(target_type, os_name)

    if select and filter_out:
        raise ValueError("--select and --filter-out are mutually exclusive")

    if target_type == "build":
        if os_name == "linux":
            matrix = artifact_data["linux_matrix"]
        elif os_name == "windows":
            matrix = artifact_data["win_matrix"]
        else:
            matrix = artifact_data["macos_builders"]
    elif target_type == "source":
        matrix = _derive_source_or_docs_matrix(artifact_data["linux_matrix"], select)
    elif target_type == "docs":
        matrix = _derive_source_or_docs_matrix(artifact_data["linux_matrix"], select)
    elif target_type == "installer":
        if not package:
            raise ValueError("--package is required when --type installer is specified")
        matrix = derive_package_matrix(artifact_data, os_name, package)
    else:
        raise ValueError(f"Unsupported type '{target_type}'")

    if select and target_type in {"build", "installer"}:
        matrix = _apply_select(matrix, select)
    elif filter_out:
        matrix = _apply_filter_out(matrix, filter_out)
    return matrix


def get_artifact_value(artifact_data: dict[str, Any], section: str) -> Any:
    """Read a named artifact section without depending on line numbers in YAML."""
    data_key = SECTION_DATA_MAP.get(section.lower())
    if data_key is None:
        supported = ", ".join(sorted(SECTION_DATA_MAP.keys()))
        raise ValueError(f"Unsupported section '{section}'. Supported sections: {supported}")
    return artifact_data[data_key]


def format_value_for_pipeline(value: Any) -> str:
    """Format values to match the artifact style expected by the pipeline."""
    if isinstance(value, str):
        return value
    return repr(value)


def _build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Read or derive docker matrix data from a DockerImageNames artifact file."
    )
    parser.add_argument("--input", required=True, help="Path to DockerImageNames file")
    selector_group = parser.add_mutually_exclusive_group(required=False)
    selector_group.add_argument(
        "--section",
        choices=sorted(SECTION_DATA_MAP.keys()),
        help="Named artifact section to return directly",
    )
    selector_group.add_argument(
        "--type",
        choices=sorted(TYPE_OS_MAP.keys()),
        help="High-level matrix derivation type",
    )
    parser.add_argument(
        "--package",
        choices=["deb", "rpm", "macos", "nsis"],
        help="Package-specific matrix to derive from the artifact",
    )
    parser.add_argument(
        "--os",
        choices=["linux", "windows", "macos"],
        help="Build OS for the derived package matrix",
    )
    parser.add_argument(
        "--filter-out",
        default="",
        help=(
            "Comma-separated exclude values (key, releaseName, dockerImageName). "
            "Mutually exclusive with --select."
        ),
    )
    parser.add_argument(
        "--select",
        default="",
        help=(
            "Comma-separated include values (key, releaseName, dockerImageName). "
            "Mutually exclusive with --filter-out."
        ),
    )
    parser.add_argument(
        "--vso-variable",
        default="",
        help="Optional variable name to emit in Azure DevOps setvariable format",
    )
    parser.add_argument(
        "--empty-on-error-non-ci",
        action="store_true",
        help=(
            "On non-CI Azure builds, convert derivation errors to '{}' and continue. "
            "CI builds still fail with non-zero exit."
        ),
    )
    return parser


def main() -> int:
    """Run CLI entry point for matrix derivation."""
    args = _build_arg_parser().parse_args()

    try:
        artifact_data = parse_docker_image_names_file(Path(args.input))
        value = _derive_cli_value(args, artifact_data)
        matrix_text = format_value_for_pipeline(value)
    except Exception as exc:  # pylint: disable=broad-except
        return _error_exit_for_exception(exc, args.empty_on_error_non_ci)

    _format_cli_output(args, matrix_text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
