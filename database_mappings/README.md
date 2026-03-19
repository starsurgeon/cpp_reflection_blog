# database_mappings (sqlite MVP)

Small working example of reflection-driven database mapping using sqlite.

This module demonstrates:
- deriving SQL column names from non-static data members
- generating `INSERT` and `SELECT` SQL from the C++ type
- binding values to a sqlite prepared statement through reflected members
- extracting a sqlite row back into a C++ object

## Prerequisites

Install sqlite in the dev container:

```bash
apt-get update
apt-get install -y sqlite3 libsqlite3-dev
```

## Build

From the repository root:

```bash
cmake -S . -B build/clang-p2996-debug -DCMAKE_CXX_COMPILER=/opt/clang-p2996/bin/clang++
cmake --build build/clang-p2996-debug -j
```

## Run

```bash
./build/clang-p2996-debug/database_mappings/database_mapping_demo
```

Expected output includes generated SQL and one loaded row.

## Files

- `include/sqlite_mapper.h`: reflection-based mapper helpers
- `src/database_mapping_demo.cpp`: sqlite in-memory round-trip demo

## Current MVP limits

- sqlite only
- supported field types: `int`, `std::string`, and `std::string_view` for binding; `int` and `std::string` for extraction
- table and column names use C++ identifiers directly
- member declaration order defines SQL column order
