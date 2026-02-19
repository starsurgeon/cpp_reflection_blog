#!/usr/bin/env bash
set -euo pipefail

strict=0
if [[ "${1:-}" == "--strict" ]]; then
  strict=1
fi

required_cmds=(cmake ninja python3 git clang-format cppcheck gcovr)
optional_cmds=(include-what-you-use iwyu mutate++ mutatepp)

missing_required=()
missing_optional=()

check_cmd() {
  local cmd="$1"
  if ! command -v "$cmd" >/dev/null 2>&1; then
    return 1
  fi
  return 0
}

for cmd in "${required_cmds[@]}"; do
  check_cmd "$cmd" || missing_required+=("$cmd")
done

for cmd in "${optional_cmds[@]}"; do
  check_cmd "$cmd" || missing_optional+=("$cmd")
done

missing_toolchain=()
if [[ ! -x /opt/clang-p2996/bin/clang++ ]]; then
  missing_toolchain+=("/opt/clang-p2996/bin/clang++")
fi
if [[ ! -f /opt/clang-p2996/cmake/clang-p2996-libcxx.toolchain.cmake ]]; then
  missing_toolchain+=("/opt/clang-p2996/cmake/clang-p2996-libcxx.toolchain.cmake")
fi

missing_runtimes=()
runtime_dir="/opt/clang-p2996/lib/clang/21/lib/x86_64-unknown-linux-gnu"
if [[ -d "$runtime_dir" ]]; then
  [[ -f "$runtime_dir/libclang_rt.asan_static.a" ]] || missing_runtimes+=("libclang_rt.asan_static.a")
  [[ -f "$runtime_dir/libclang_rt.asan-preinit.a" ]] || missing_runtimes+=("libclang_rt.asan-preinit.a")
  [[ -f "$runtime_dir/libclang_rt.ubsan_standalone.so" ]] || missing_runtimes+=("libclang_rt.ubsan_standalone.so")
  [[ -f "$runtime_dir/libclang_rt.profile.a" ]] || missing_runtimes+=("libclang_rt.profile.a")
else
  missing_runtimes+=("runtime dir missing: $runtime_dir")
fi

summary_file="preflight-summary.md"
{
  echo "# CI Preflight Summary"
  echo
  echo "## Missing required commands"
  if [[ ${#missing_required[@]} -eq 0 ]]; then
    echo "- none"
  else
    for item in "${missing_required[@]}"; do
      echo "- $item"
    done
  fi
  echo
  echo "## Missing toolchain paths"
  if [[ ${#missing_toolchain[@]} -eq 0 ]]; then
    echo "- none"
  else
    for item in "${missing_toolchain[@]}"; do
      echo "- $item"
    done
  fi
  echo
  echo "## Missing sanitizer/coverage runtimes"
  if [[ ${#missing_runtimes[@]} -eq 0 ]]; then
    echo "- none"
  else
    for item in "${missing_runtimes[@]}"; do
      echo "- $item"
    done
  fi
  echo
  echo "## Missing optional commands"
  if [[ ${#missing_optional[@]} -eq 0 ]]; then
    echo "- none"
  else
    for item in "${missing_optional[@]}"; do
      echo "- $item"
    done
  fi
} >"$summary_file"

cat "$summary_file"

if [[ -n "${GITHUB_STEP_SUMMARY:-}" ]]; then
  cat "$summary_file" >>"$GITHUB_STEP_SUMMARY"
fi

if [[ $strict -eq 1 ]]; then
  if [[ ${#missing_required[@]} -gt 0 || ${#missing_toolchain[@]} -gt 0 || ${#missing_runtimes[@]} -gt 0 ]]; then
    echo "Preflight failed due to missing required dependencies." >&2
    exit 2
  fi
fi
