#!/usr/bin/env python3
"""Minimal demo for the `example` pybind11 extension module."""

import importlib.machinery
import importlib.util
import sysconfig
from pathlib import Path
import sys


def _load_example_module():
    """Load the compiled extension module from common build locations."""
    repo_root = Path(__file__).resolve().parents[1]
    extension_suffixes = list(importlib.machinery.EXTENSION_SUFFIXES)

    # Include Python's configured extension suffix as an additional fallback.
    ext_suffix = sysconfig.get_config_var("EXT_SUFFIX")
    if isinstance(ext_suffix, str) and ext_suffix:
        extension_suffixes.append(ext_suffix)

    candidate_dirs = [
        repo_root / "build" / "clang-p2996-debug" / "python",
        repo_root / "build" / "clang-p2996-coverage" / "python",
        repo_root / "build" / "clang-p2996-asan" / "python",
        repo_root / "build" / "clang-p2996-ubsan" / "python",
        repo_root / "build" / "Release" / "python_bindings",
        repo_root / "cmake_builds" / "debug" / "python_bindings",
        repo_root / "cmake_builds" / "release" / "python_bindings",
        Path(__file__).resolve().parent,
    ]

    for module_dir in candidate_dirs:
        if not module_dir.is_dir():
            continue

        for suffix in extension_suffixes:
            candidate = module_dir / f"example{suffix}"
            if not candidate.is_file():
                continue

            spec = importlib.util.spec_from_file_location("example", candidate)
            if spec is None or spec.loader is None:
                continue

            module = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(module)
            return module

    # Fallback for custom presets: locate example<ext> anywhere under known build roots.
    build_roots = [repo_root / "build", repo_root / "cmake_builds"]
    for build_root in build_roots:
        if not build_root.is_dir():
            continue

        for suffix in extension_suffixes:
            for candidate in build_root.rglob(f"example{suffix}"):
                if not candidate.is_file():
                    continue

                spec = importlib.util.spec_from_file_location("example", candidate)
                if spec is None or spec.loader is None:
                    continue

                module = importlib.util.module_from_spec(spec)
                spec.loader.exec_module(module)
                return module

    searched = "\n".join(str(path) for path in candidate_dirs)
    raise ImportError(
        "Could not locate compiled 'example' extension module. "
        "Build python_bindings first and ensure one of these paths contains example<ext>:\n"
        f"{searched}"
    )


example = _load_example_module()


if __name__ == "__main__":
    # Display function documentation
    print("=== Function Documentation ===")
    print(f"add: {example.add.__doc__}\n")
    print(f"multiply: {example.multiply.__doc__}\n")
    print(f"greet: {example.greet.__doc__}\n")
    print(f"say_hello: {example.say_hello.__doc__}\n")
    
    print("=== Function Calls ===")
    lhs = 2
    rhs = 3
    result = example.add(lhs, rhs)
    print(f"example.add({lhs}, {rhs}) = {result}")

    example.say_hello()
    name = "Alice"
    result = example.greet(name)
    print(f"example.greet({name}) = {result}")

    lhs = 4.13
    rhs = 2.71
    result = example.multiply(lhs, rhs)
    print(f"example.multiply({lhs}, {rhs}) = {result}")

    #test with named arguments
    result = example.add(i=5, j=7)
    print(f"example.add(i=5, j=7) = {result}")  

    result = example.multiply(a=1.5, b=2.5)
    print(f"example.multiply(a=1.5, b=2.5) = {result}") 

    result = example.greet(name="Bob")
    print(f"example.greet(name='Bob') = {result}")

