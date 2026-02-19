#pragma once

#include "json_reader.h"
#include "json_writer.h"

namespace json
{

  // Main to_json implementation
  template<typename T>
  std::string to_json(const T &value, const options &opts)
  {
    json_writer writer(opts);
    write_value(writer, value, opts);
    return writer.result();
  }

  template<typename T>
  std::expected<T, parse_error> try_from_json(std::string_view json_text)
  {
    detail::json_reader reader(json_text);
    auto value = detail::read_value<T>(reader);
    if (!value.has_value())
    {
      return std::unexpected(value.error());
    }

    reader.skip_whitespace();
    if (!reader.at_end())
    {
      return std::unexpected(
        parse_error{ .message = "Trailing characters after JSON value", .position = reader.position() });
    }

    return value;
  }

  template<typename T>
  T from_json(std::string_view json_text)
  {
    auto value = try_from_json<T>(json_text);
    if (!value.has_value())
    {
      throw std::invalid_argument(std::string("JSON deserialization failed at position ")
                                  + std::to_string(value.error().position) + ": " + value.error().message);
    }
    return std::move(value.value());
  }

} // namespace json
