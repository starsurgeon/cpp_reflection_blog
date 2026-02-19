#!/usr/bin/env bash
set -euo pipefail

if ! command -v clang-format >/dev/null 2>&1; then
  echo "clang-format is required" >&2
  exit 2
fi

mapfile -t files < <(git ls-files '*.h' '*.hpp' '*.hh' '*.c' '*.cc' '*.cpp' '*.cxx')

if [[ ${#files[@]} -eq 0 ]]; then
  echo "No C/C++ files to check"
  exit 0
fi

clang-format --dry-run --Werror "${files[@]}"
