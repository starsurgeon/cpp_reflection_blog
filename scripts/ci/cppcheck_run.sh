#!/usr/bin/env bash
set -euo pipefail

if ! command -v cppcheck >/dev/null 2>&1; then
  echo "cppcheck is required" >&2
  exit 2
fi

cppcheck \
  --enable=warning,style,performance,portability \
  --error-exitcode=2 \
  --inline-suppr \
  --quiet \
  --std=c++26 \
  -I jsonserializer/include \
  jsonserializer/src jsonserializer/include jsonserializer/tests
