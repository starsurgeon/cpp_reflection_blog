#!/usr/bin/env bash
set -euo pipefail

if command -v mutate++ >/dev/null 2>&1; then
  mutate++ --help >/dev/null
  exit 0
fi

if command -v mutatepp >/dev/null 2>&1; then
  mutatepp --help >/dev/null
  exit 0
fi

echo "mutate++ is required" >&2
exit 2
