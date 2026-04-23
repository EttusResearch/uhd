#!/usr/bin/env python3
#
# Copyright 2025 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
r"""Validate a Doxygen HTML build output directory.

Checks performed:
  1. (If log file provided) The log file exists and contains no "CMake Error" entries.
  2. The sentinel HTML file (05__driver__api_8dox.html) exists in the html/ subfolder.
  3. (If log file provided) No per-file warnings in the build log.
     If warnings_as_errors is enabled, the script exits 1 if any warnings are found.

Usage:
    python check_doxygen_build.py \\
        --output-dir <build_output_dir> \\
        [--log-file <path/to/build.log>] \\
        [--warnings-as-errors]

    --output-dir   Directory that contains the html/ subfolder.
    --log-file     Optional path to a build log to scan for per-file warnings.
    --warnings-as-errors  Treat warnings as fatal errors.

Exit codes:
    0  All checks passed.
    1  A check failed.
"""

import argparse
import logging
import re
import sys
from pathlib import Path

logging.basicConfig(
    level=logging.INFO,
    format="%(levelname)s: %(message)s",
)
logger = logging.getLogger(__name__)

SENTINEL_HTML = "05__driver__api_8dox.html"
WARNING_PATTERN = re.compile(r"^[^ ]+:\d+: warning:", re.MULTILINE)


def check_doxygen_build(
    output_dir: Path,
    log_file: Path | None,
    warnings_as_errors: bool,
) -> bool:
    """Run HTML-output post-build checks.

    Args:
        output_dir:         Directory containing the html/ subfolder.
        log_file:           Optional path to a build log to scan for warnings.
        warnings_as_errors: Whether warnings should fail the check.

    Returns:
        True if all checks pass.
    """
    # --- Check 1: log file existence and CMake errors (if log provided) ---
    log_text = None
    if log_file is not None:
        if not log_file.is_file():
            logger.error("Log file not found: %s", log_file)
            return False
        log_text = log_file.read_text(encoding="utf-8", errors="replace")
        if "CMake Error" in log_text:
            logger.error("CMake Error detected in build log. Failing pipeline.")
            # Print matching lines with context
            for line in log_text.split("\n"):
                if "CMake Error" in line:
                    logger.error(line)
            return False

    html_dir = output_dir / "html"

    # --- Check 2: sentinel HTML file must exist ---
    sentinel = html_dir / SENTINEL_HTML
    if not sentinel.is_file():
        logger.error(
            "Doxygen build did not complete successfully. %s not found in %s.",
            SENTINEL_HTML,
            html_dir,
        )
        return False
    logger.info("Sentinel file found: %s", sentinel)

    # --- Check 3: per-file warnings (only when a log file is provided) ---
    if log_text is not None:
        warnings = WARNING_PATTERN.findall(log_text)
        warning_count = len(warnings)
        if warning_count > 0:
            logger.warning("Detected %d warning(s) in build log.", warning_count)
            if warnings_as_errors:
                logger.error("Warnings treated as errors. Failing.")
                return False
            else:
                logger.warning("Warnings are non-fatal.")
        else:
            logger.info("No per-file warnings found in build log.")
    else:
        logger.info("No log file provided; skipping warning check.")

    logger.info("All Doxygen build checks passed.")
    return True


def parse_args() -> argparse.Namespace:
    """Parse command-line arguments for Doxygen build output validation."""
    parser = argparse.ArgumentParser(description="Validate Doxygen HTML build output.")
    parser.add_argument(
        "--output-dir",
        required=True,
        type=Path,
        help="Directory containing the html/ subfolder.",
    )
    parser.add_argument(
        "--log-file",
        default=None,
        type=Path,
        help="Optional path to a build log to scan for per-file warnings.",
    )
    parser.add_argument(
        "--warnings-as-errors",
        action="store_true",
        default=False,
        help="Treat warnings as fatal errors.",
    )
    return parser.parse_args()


def main() -> None:
    """Run validation CLI entrypoint and exit with success/failure status."""
    args = parse_args()
    output_dir = args.output_dir.resolve()
    log_file = args.log_file.resolve() if args.log_file else None

    if not output_dir.is_dir():
        logger.error("Output directory does not exist: %s", output_dir)
        sys.exit(1)

    ok = check_doxygen_build(
        output_dir=output_dir,
        log_file=log_file,
        warnings_as_errors=args.warnings_as_errors,
    )
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
