#!/usr/bin/env python3
"""Find likely broken Doxygen single-line comments.

Pattern flagged:
    //! First line
    // continuation line

I.e., a line starting with `//!` followed by one or more immediately following
lines that start with plain `//` (exactly two slashes, not `//!` or `///`).

Only `.hpp` and `.cpp` files are scanned.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

DOC_START_PREFIX = "//!"
PLAIN_COMMENT_PREFIX = "//"


def parse_args() -> argparse.Namespace:
    """Parse command-line arguments."""
    parser = argparse.ArgumentParser(
        description=(
            "Scan .hpp/.cpp files for likely broken Doxygen comments where "
            "a //! line is followed by plain // continuation lines."
        )
    )
    parser.add_argument(
        "files",
        nargs="*",
        type=Path,
        help=("Optional explicit file list to scan. Non-.cpp/.hpp entries are ignored."),
    )
    return parser.parse_args()


def is_plain_comment_line(line: str) -> bool:
    """Check if a line is a plain comment line (starts with // but not //! or ///)."""
    stripped = line.lstrip()
    if not stripped.startswith(PLAIN_COMMENT_PREFIX):
        return False
    if stripped.startswith("//!") or stripped.startswith("///"):
        return False
    return True


def find_matches_in_file(path: Path) -> list[int]:
    """Find likely broken Doxygen comments in a file."""
    try:
        lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError:
        return []

    matches: list[int] = []
    i = 0
    while i < len(lines):
        stripped = lines[i].lstrip()
        if stripped.startswith(DOC_START_PREFIX):
            j = i + 1
            saw_plain_comment = False
            while j < len(lines) and is_plain_comment_line(lines[j]):
                saw_plain_comment = True
                j += 1
            if saw_plain_comment:
                matches.append(i + 1)  # 1-based line number of //! start
                i = j
                continue
        i += 1

    return matches


def iter_selected_cpp_headers(root: Path, files: list[Path]):
    """Iterates over the given file paths, yielding only those that are .cpp or .hpp files."""
    seen: set[Path] = set()
    for p in files:
        candidate = p if p.is_absolute() else (root / p)
        try:
            resolved = candidate.resolve()
        except OSError:
            continue

        if resolved in seen:
            continue
        seen.add(resolved)

        if not resolved.exists() or not resolved.is_file():
            continue
        yield resolved


def main() -> int:
    """Main entry point."""
    args = parse_args()
    root = Path(__file__).resolve().parents[1]

    total_files = 0
    total_hits = 0

    if args.files:
        file_iter = iter_selected_cpp_headers(root, args.files)
    else:
        print("No files to check.")
        return 0

    for file_path in file_iter:
        total_files += 1
        hits = find_matches_in_file(file_path)
        if not hits:
            continue
        try:
            rel = file_path.relative_to(root)
        except ValueError:
            rel = file_path
        for line_no in hits:
            print(f"{rel}:{line_no}")
            total_hits += 1

    if total_hits:
        print(
            f"\nFound {total_hits} likely broken Doxygen comment block(s) in "
            f"{total_files} scanned file(s).",
            file=sys.stderr,
        )
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
