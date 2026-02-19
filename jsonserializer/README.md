# Reflection-Based JSON Serialization Library

This module provides compile-time reflection-based JSON serialization and deserialization for C++26 (P2996).

## Include

```cpp
#include "json.h"
```

The project adds `jsonserializer/include` to include paths for the test targets.

## Features

- Header-only API (`json::to_json`, `json::try_from_json`, `json::from_json`)
- Reflection-based object support (including base classes and private/protected members)
- Pretty-print output via `json::options`
- Round-trip support for many STL/container types
- Non-throwing parse API with structured errors (`parse_error`)

## Core API

```cpp
namespace json {
    struct options {
        bool pretty = false;
        int indent_size = 2;
        int current_indent = 0;
    };

    struct parse_error {
        std::string message;
        std::size_t position = 0;
    };

    template<typename T>
    std::string to_json(const T& value, const options& opts = {});

    template<typename T>
    std::expected<T, parse_error> try_from_json(std::string_view json_text);

    template<typename T>
    T from_json(std::string_view json_text); // throws std::invalid_argument on parse failure
}
```

## Serialization Examples

```cpp
// Primitives
json::to_json(42);        // "42"
json::to_json(true);      // "true"
json::to_json("hello");   // "\"hello\""

// Optional
json::to_json(std::optional<int>{42}); // "42"
json::to_json(std::optional<int>{});   // "null"

// Variant (current format)
std::variant<int, std::string> v = "hello";
json::to_json(v); // {"index":1,"value":"hello"}

// String-key map
json::to_json(std::map<std::string, int>{{"a", 1}}); // {"a":1}

// Non-string-key map
json::to_json(std::map<int, std::string>{{1, "one"}}); // [[1,"one"]]
```

## Reflection Example

```cpp
struct Person {
    std::string name;
    int age;
    bool employed;
};

Person p{"Alice", 30, true};
auto text = json::to_json(p);
auto copy = json::from_json<Person>(text);
```

## Deserialization Notes

- `try_from_json<T>` returns `std::expected<T, parse_error>` and never throws
- `from_json<T>` throws `std::invalid_argument` with position/message on failure
- Trailing characters after a valid JSON value are rejected
- For variants, deserialization accepts both:
    - `{"index":N,"value":...}` (primary format)
    - `{"type":"...","value":...}` (name-based form)

## Supported Type Categories

- Scalars: `bool`, integral types (except `char`), floating-point
- Strings: `std::string`, `std::string_view`, `char*`, `const char*`
- Pointers: raw pointers (non-void), `std::unique_ptr`, `std::shared_ptr`
- Optionals/variants/tuples: `std::optional`, `std::variant`, tuple-like (`std::tuple`, `std::pair`, `std::array`)
- Ranges/containers: vector/list/deque/set/unordered_set/forward_list and other range-like containers with insertion support
- Maps:
    - string-key maps as JSON objects
    - non-string-key maps as arrays of `[key,value]`
- Adaptors/special: `std::stack`, `std::queue`, `std::priority_queue`, `std::bitset<N>`
- Reflectable user-defined class/struct types

## Pretty Printing

```cpp
json::options opts;
opts.pretty = true;
opts.indent_size = 2;

auto formatted = json::to_json(p, opts);
```

## Build & Test (from repository root)

```bash
cmake --preset clang-p2996-debug
cmake --build --preset build-debug
ctest --preset test-debug --output-on-failure
```

## Requirements

- Clang toolchain with P2996 reflection support
- C++26
- libc++ (configured by project toolchain presets)
