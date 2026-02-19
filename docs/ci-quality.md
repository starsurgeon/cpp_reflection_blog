# CI Quality Pipeline

This repository uses CMake presets and GitHub Actions workflows to enforce quality checks for pull requests and post-merge builds.

## Required PR checks

- `preflight-tools`
- `build-debug`
- `test-debug`
- `asan-tests`
- `ubsan-tests`
- `cppcheck`
- `clang-format-check`
- `coverage-70`

The `preflight-tools` job runs first and reports missing runner dependencies in `preflight-summary` artifact before the build/test matrix executes.

## Optional PR checks

- `iwyu-check`
- `fuzztest-build`
- `mutation-smoke`

Optional checks provide tool integration coverage and can be promoted to required once runner tooling is stable.

## Post-merge checks

- `preflight-tools`
- `post-merge-build-debug`
- `post-merge-test-debug`

The same preflight dependency gate is also executed on post-merge runs to detect runner drift.

## Runner prerequisites

Self-hosted Linux runners must provide:

- `/opt/clang-p2996` toolchain matching presets in `CMakePresets.json`
- `cmake`, `ninja`, `python3`, `git`
- `clang-format`
- `cppcheck`
- `gcovr`
- (optional) include-what-you-use
- (optional) FuzzTest CMake package
- (optional) mutate++

## Branch protection setup

In GitHub branch protection for `main`:

1. Restrict direct pushes to repository owner/admin policy.
2. Require pull requests for all changes.
3. Require status checks to pass before merging.
4. Add all required PR checks by exact job name.
5. Require at least one review (team policy).

This repository stores workflows in `.github/workflows` and expects branch protection settings to be managed in GitHub repository settings.
