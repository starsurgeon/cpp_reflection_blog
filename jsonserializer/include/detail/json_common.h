#pragma once

#include <charconv>
#include <cmath>
#include <expected>
#include <memory>
#include <unordered_map>

#include "../json_types.h"

namespace
{
  template<typename T>
  struct is_unique_ptr_specialization : std::false_type
  {
  };

  template<typename Element, typename Deleter>
  struct is_unique_ptr_specialization<std::unique_ptr<Element, Deleter>> : std::true_type
  {
  };

  template<typename T>
  struct is_shared_ptr_specialization : std::false_type
  {
  };

  template<typename Element>
  struct is_shared_ptr_specialization<std::shared_ptr<Element>> : std::true_type
  {
  };

  constexpr unsigned char kNibbleMask = 0x0F;
  constexpr unsigned kDecimalBase = 10;
  constexpr unsigned kHexShift = 4;
  constexpr std::size_t kSmallStringReserve = 16;
  constexpr unsigned kUnicodeAsciiMax = 0x7F;
  constexpr unsigned kUnicodeTwoByteMax = 0x7FF;
  constexpr unsigned kUnicodeThreeByteMax = 0xFFFF;
  constexpr unsigned kUtf8TwoByteLead = 0xC0U;
  constexpr unsigned kUtf8ThreeByteLead = 0xE0U;
  constexpr unsigned kUtf8FourByteLead = 0xF0U;
  constexpr unsigned kUtf8TrailLead = 0x80U;
  constexpr unsigned kUtf8LowSixBitsMask = 0x3FU;
  constexpr unsigned kUtf8LowFiveBitsMask = 0x1FU;
  constexpr unsigned kUtf8LowThreeBitsMask = 0x07U;
  constexpr unsigned kFloatBufferExtra = 10;
  constexpr unsigned kUtf8SixBitShift = 6U;
  constexpr unsigned kUtf8TwelveBitShift = 12U;
  constexpr unsigned kUtf8EighteenBitShift = 18U;
  constexpr unsigned kSurrogateHighStart = 0xD800U;
  constexpr unsigned kSurrogateHighEnd = 0xDBFFU;
  constexpr unsigned kSurrogateLowStart = 0xDC00U;
  constexpr unsigned kSurrogateLowEnd = 0xDFFFU;
  constexpr unsigned kSurrogateOffset = 0x10000U;
  constexpr unsigned kSurrogateTenBitMask = 0x03FFU;
  constexpr unsigned kSurrogateCombineShift = 10U;
  constexpr unsigned kEscapedCodeUnitLength = 6U;

  [[nodiscard]] constexpr char hex_digit(unsigned value) noexcept
  {
    return static_cast<char>(value < kDecimalBase ? ('0' + value) : ('a' + (value - kDecimalBase)));
  }

  inline void append_control_escape(std::string &result, unsigned char uc)
  {
    result += "\\u00";
    result += hex_digit((uc >> kHexShift) & kNibbleMask);
    result += hex_digit(uc & kNibbleMask);
  }

  [[nodiscard]] constexpr bool is_high_surrogate(unsigned code_unit) noexcept
  {
    return code_unit >= kSurrogateHighStart && code_unit <= kSurrogateHighEnd;
  }

  [[nodiscard]] constexpr bool is_low_surrogate(unsigned code_unit) noexcept
  {
    return code_unit >= kSurrogateLowStart && code_unit <= kSurrogateLowEnd;
  }

  inline void append_utf8_code_point(std::string &result, unsigned code_point)
  {
    if (code_point <= kUnicodeAsciiMax)
    {
      result.push_back(static_cast<char>(code_point));
      return;
    }

    if (code_point <= kUnicodeTwoByteMax)
    {
      result.push_back(static_cast<char>(kUtf8TwoByteLead | ((code_point >> kUtf8SixBitShift) & kUtf8LowFiveBitsMask)));
      result.push_back(static_cast<char>(kUtf8TrailLead | (code_point & kUtf8LowSixBitsMask)));
      return;
    }

    if (code_point <= kUnicodeThreeByteMax)
    {
      result.push_back(static_cast<char>(kUtf8ThreeByteLead | ((code_point >> kUtf8TwelveBitShift) & kNibbleMask)));
      result.push_back(static_cast<char>(kUtf8TrailLead | ((code_point >> kUtf8SixBitShift) & kUtf8LowSixBitsMask)));
      result.push_back(static_cast<char>(kUtf8TrailLead | (code_point & kUtf8LowSixBitsMask)));
      return;
    }

    result.push_back(
      static_cast<char>(kUtf8FourByteLead | ((code_point >> kUtf8EighteenBitShift) & kUtf8LowThreeBitsMask)));
    result.push_back(static_cast<char>(kUtf8TrailLead | ((code_point >> kUtf8TwelveBitShift) & kUtf8LowSixBitsMask)));
    result.push_back(static_cast<char>(kUtf8TrailLead | ((code_point >> kUtf8SixBitShift) & kUtf8LowSixBitsMask)));
    result.push_back(static_cast<char>(kUtf8TrailLead | (code_point & kUtf8LowSixBitsMask)));
  }
} // namespace


namespace json::detail
{
  // ASCII control character boundary
  constexpr unsigned char control_char_boundary = 0x20;

  // JSON string escaping
  inline std::string escape_string(std::string_view str)
  {
    std::string result;
    result.reserve(str.size() + 2);

    for (char c : str)
    {
      switch (c)
      {
      case '"':
        result += "\\\"";
        break;
      case '\\':
        result += "\\\\";
        break;
      case '\b':
        result += "\\b";
        break;
      case '\f':
        result += "\\f";
        break;
      case '\n':
        result += "\\n";
        break;
      case '\r':
        result += "\\r";
        break;
      case '\t':
        result += "\\t";
        break;
      default:
      {
        const auto uc = static_cast<unsigned char>(c);
        if (uc < control_char_boundary)
        {
          // Control characters
          append_control_escape(result, uc);
        }
        else
        {
          result += c;
        }
        break;
      }
      }
    }
    return result;
  }

  template<typename T>
  std::expected<T, parse_error> read_value(class json_reader &reader);

  [[nodiscard]] inline std::expected<std::string, parse_error> make_error(std::string message, std::size_t position)
  {
    return std::unexpected(parse_error{ .message = std::move(message), .position = position });
  }

  class json_reader
  {
  public:
    explicit json_reader(std::string_view input, std::size_t base_offset = 0) :
      input_(input), base_offset_(base_offset)
    {
    }

    [[nodiscard]] bool at_end() const
    {
      return position_ >= input_.size();
    }

    [[nodiscard]] std::size_t position() const
    {
      return base_offset_ + position_;
    }

    [[nodiscard]] char current() const
    {
      return input_[position_];
    }

    [[nodiscard]] std::string_view remaining() const
    {
      return input_.substr(position_);
    }

    [[nodiscard]] std::string_view input() const
    {
      return input_;
    }

    [[nodiscard]] std::size_t base_offset() const
    {
      return base_offset_;
    }

    struct consumed_value_view
    {
      std::string_view view;
      std::size_t position = 0;
    };

    void skip_whitespace()
    {
      while (!at_end() && std::isspace(static_cast<unsigned char>(current())) != 0)
      {
        ++position_;
      }
    }

    [[nodiscard]] std::expected<char, parse_error> read_char(char expected)
    {
      skip_whitespace();
      if (at_end())
      {
        return std::unexpected(
          parse_error{ .message = std::string("Expected '") + expected + "'", .position = position() });
      }
      if (current() != expected)
      {
        return std::unexpected(parse_error{
          .message = std::string("Expected '") + expected + "', got '" + current() + "'", .position = position() });
      }
      ++position_;
      return expected;
    }

    [[nodiscard]] std::expected<bool, parse_error> read_literal(std::string_view literal)
    {
      skip_whitespace();
      if (remaining().starts_with(literal))
      {
        position_ += literal.size();
        return true;
      }
      return std::unexpected(parse_error{ .message = std::string("Expected literal '") + std::string(literal) + "'",
                                          .position = position() });
    }

    [[nodiscard]] std::expected<std::string, parse_error>
    read_string() // NOLINT(readability-function-cognitive-complexity)
    {
      skip_whitespace();
      if (at_end() || current() != '"')
      {
        return std::unexpected(parse_error{ .message = "Expected string", .position = position() });
      }

      ++position_;
      std::string result;
      result.reserve(kSmallStringReserve);

      while (!at_end())
      {
        const char ch = current();
        ++position_;

        if (ch == '"')
        {
          return result;
        }

        if (ch == '\\')
        {
          if (at_end())
          {
            return std::unexpected(parse_error{ .message = "Incomplete escape sequence", .position = position() });
          }

          const char esc = current();
          ++position_;
          switch (esc)
          {
          case '"':
            result.push_back('"');
            break;
          case '\\':
            result.push_back('\\');
            break;
          case '/':
            result.push_back('/');
            break;
          case 'b':
            result.push_back('\b');
            break;
          case 'f':
            result.push_back('\f');
            break;
          case 'n':
            result.push_back('\n');
            break;
          case 'r':
            result.push_back('\r');
            break;
          case 't':
            result.push_back('\t');
            break;
          case 'u':
          {
            const auto parse_code_unit = [this]() -> std::expected<unsigned, parse_error>
            {
              if (position_ + 4 > input_.size())
              {
                return std::unexpected(parse_error{ .message = "Incomplete unicode escape", .position = position() });
              }

              unsigned code_unit = 0;
              for (int i = 0; i < 4; ++i)
              {
                const char hex = input_[position_++];
                code_unit <<= kHexShift;
                if (hex >= '0' && hex <= '9')
                {
                  code_unit += static_cast<unsigned>(hex - '0');
                }
                else if (hex >= 'a' && hex <= 'f')
                {
                  code_unit += static_cast<unsigned>(hex - 'a') + kDecimalBase;
                }
                else if (hex >= 'A' && hex <= 'F')
                {
                  code_unit += static_cast<unsigned>(hex - 'A') + kDecimalBase;
                }
                else
                {
                  return std::unexpected(
                    parse_error{ .message = "Invalid unicode escape", .position = position() - 1 });
                }
              }
              return code_unit;
            };

            auto first_code_unit = parse_code_unit();
            if (!first_code_unit.has_value())
            {
              return std::unexpected(first_code_unit.error());
            }

            unsigned code_point = first_code_unit.value();
            if (is_high_surrogate(code_point))
            {
              if (position_ + kEscapedCodeUnitLength > input_.size() || input_[position_] != '\\'
                  || input_[position_ + 1] != 'u')
              {
                return std::unexpected(
                  parse_error{ .message = "Missing low surrogate after high surrogate", .position = position() });
              }

              position_ += 2;
              auto second_code_unit = parse_code_unit();
              if (!second_code_unit.has_value())
              {
                return std::unexpected(second_code_unit.error());
              }

              if (!is_low_surrogate(second_code_unit.value()))
              {
                return std::unexpected(
                  parse_error{ .message = "Invalid low surrogate after high surrogate", .position = position() - 4 });
              }

              code_point = kSurrogateOffset
                            + (((code_point - kSurrogateHighStart) & kSurrogateTenBitMask) << kSurrogateCombineShift)
                            + ((second_code_unit.value() - kSurrogateLowStart) & kSurrogateTenBitMask);
            }
            else if (is_low_surrogate(code_point))
            {
              return std::unexpected(
                parse_error{ .message = "Unexpected low surrogate", .position = position() - 4 });
            }

            append_utf8_code_point(result, code_point);
            break;
          }
          default:
            return std::unexpected(parse_error{ .message = "Invalid escape sequence", .position = position() - 1 });
          }
          continue;
        }

        if (static_cast<unsigned char>(ch) < control_char_boundary)
        {
          return std::unexpected(parse_error{ .message = "Control character in string", .position = position() - 1 });
        }

        result.push_back(ch);
      }

      return std::unexpected(parse_error{ .message = "Unterminated string", .position = position() });
    }

    [[nodiscard]] std::expected<std::string_view, parse_error>
    read_number_token() // NOLINT(readability-function-cognitive-complexity)
    {
      skip_whitespace();
      if (at_end())
      {
        return std::unexpected(parse_error{ .message = "Expected number", .position = position() });
      }

      const std::size_t start = position_;
      if (current() == '-')
      {
        ++position_;
        if (at_end())
        {
          return std::unexpected(parse_error{ .message = "Invalid number", .position = position() });
        }
      }

      if (current() == '0')
      {
        ++position_;
        if (!at_end() && std::isdigit(static_cast<unsigned char>(current())) != 0)
        {
          return std::unexpected(parse_error{ .message = "Leading zeros are not allowed", .position = position() });
        }
      }
      else
      {
        if (std::isdigit(static_cast<unsigned char>(current())) == 0)
        {
          return std::unexpected(parse_error{ .message = "Expected digit", .position = position() });
        }

        while (!at_end() && std::isdigit(static_cast<unsigned char>(current())) != 0)
        {
          ++position_;
        }
      }

      if (!at_end() && current() == '.')
      {
        ++position_;
        if (at_end() || std::isdigit(static_cast<unsigned char>(current())) == 0)
        {
          return std::unexpected(
            parse_error{ .message = "Expected digits after decimal point", .position = position() });
        }
        while (!at_end() && std::isdigit(static_cast<unsigned char>(current())) != 0)
        {
          ++position_;
        }
      }

      if (!at_end() && (current() == 'e' || current() == 'E'))
      {
        ++position_;
        if (!at_end() && (current() == '+' || current() == '-'))
        {
          ++position_;
        }

        if (at_end() || std::isdigit(static_cast<unsigned char>(current())) == 0)
        {
          return std::unexpected(parse_error{ .message = "Expected exponent digits", .position = position() });
        }

        while (!at_end() && std::isdigit(static_cast<unsigned char>(current())) != 0)
        {
          ++position_;
        }
      }

      return input_.substr(start, position_ - start);
    }

    [[nodiscard]] std::expected<consumed_value_view, parse_error> consume_value_view()
    {
      skip_whitespace();
      const std::size_t start = position_;
      auto skipped = skip_value();
      if (!skipped.has_value())
      {
        return std::unexpected(skipped.error());
      }
      return consumed_value_view{ .view = input_.substr(start, position_ - start), .position = base_offset_ + start };
    }

    [[nodiscard]] std::expected<bool, parse_error> skip_value()
    {
      skip_whitespace();
      if (at_end())
      {
        return std::unexpected(parse_error{ .message = "Unexpected end of input", .position = position() });
      }

      switch (current())
      {
      case '{':
        return skip_object();
      case '[':
        return skip_array();
      case '"':
      {
        auto text = read_string();
        if (!text.has_value())
        {
          return std::unexpected(text.error());
        }
        return true;
      }
      case 't':
        return read_literal("true");
      case 'f':
        return read_literal("false");
      case 'n':
        return read_literal("null");
      default:
      {
        auto number = read_number_token();
        if (!number.has_value())
        {
          return std::unexpected(number.error());
        }
        return true;
      }
      }
    }

  private:
    [[nodiscard]] std::expected<bool, parse_error> skip_array()
    {
      auto open = read_char('[');
      if (!open.has_value())
      {
        return std::unexpected(open.error());
      }

      skip_whitespace();
      if (!at_end() && current() == ']')
      {
        ++position_;
        return true;
      }

      while (true)
      {
        auto value = skip_value();
        if (!value.has_value())
        {
          return std::unexpected(value.error());
        }

        skip_whitespace();
        if (!at_end() && current() == ',')
        {
          ++position_;
          continue;
        }

        if (!at_end() && current() == ']')
        {
          ++position_;
          return true;
        }

        return std::unexpected(parse_error{ .message = "Expected ',' or ']'", .position = position() });
      }
    }

    [[nodiscard]] std::expected<bool, parse_error> skip_object()
    {
      auto open = read_char('{');
      if (!open.has_value())
      {
        return std::unexpected(open.error());
      }

      skip_whitespace();
      if (!at_end() && current() == '}')
      {
        ++position_;
        return true;
      }

      while (true)
      {
        auto key = read_string();
        if (!key.has_value())
        {
          return std::unexpected(key.error());
        }

        auto colon = read_char(':');
        if (!colon.has_value())
        {
          return std::unexpected(colon.error());
        }

        auto value = skip_value();
        if (!value.has_value())
        {
          return std::unexpected(value.error());
        }

        skip_whitespace();
        if (!at_end() && current() == ',')
        {
          ++position_;
          continue;
        }

        if (!at_end() && current() == '}')
        {
          ++position_;
          return true;
        }

        return std::unexpected(parse_error{ .message = "Expected ',' or '}'", .position = position() });
      }
    }

    std::string_view input_;
    std::size_t base_offset_ = 0;
    std::size_t position_ = 0;
  };

  template<typename Container, typename Value>
  void append_to_container(Container &container, Value &&value)
  {
    if constexpr (requires { container.push_back(std::forward<Value>(value)); })
    {
      container.push_back(std::forward<Value>(value));
    }
    else if constexpr (requires { container.emplace_back(std::forward<Value>(value)); })
    {
      container.emplace_back(std::forward<Value>(value));
    }
    else if constexpr (requires { container.insert(std::forward<Value>(value)); })
    {
      container.insert(std::forward<Value>(value));
    }
    else if constexpr (requires { container.emplace(std::forward<Value>(value)); })
    {
      container.emplace(std::forward<Value>(value));
    }
    else if constexpr (requires { container.before_begin(); container.insert_after(container.before_begin(),
                                                                                  std::forward<Value>(value)); })
    {
      auto tail = container.before_begin();
      for (auto it = container.begin(); it != container.end(); ++it)
      {
        ++tail;
      }
      container.insert_after(tail, std::forward<Value>(value));
    }
    else if constexpr (requires { container.push_front(std::forward<Value>(value)); })
    {
      container.push_front(std::forward<Value>(value));
    }
    else
    {
      static_assert(sizeof(Container) == 0, "Container type does not support insertion");
    }
  }

  template<typename Number>
  [[nodiscard]] std::expected<Number, parse_error> parse_integer(std::string_view token, std::size_t position)
  {
    if (token.find_first_of(".eE") != std::string_view::npos)
    {
      return std::unexpected(parse_error{ .message = "Expected integer", .position = position });
    }

    if constexpr (std::unsigned_integral<Number>)
    {
      if (!token.empty() && token.front() == '-')
      {
        return std::unexpected(parse_error{ .message = "Negative value for unsigned type", .position = position });
      }
    }

    Number result{};
    const auto *begin = token.data();
    const auto *end = token.data() + token.size();
    const auto [ptr, ec] = std::from_chars(begin, end, result);
    if (ec != std::errc{} || ptr != end)
    {
      return std::unexpected(parse_error{ .message = "Invalid integer value", .position = position });
    }
    return result;
  }

  template<typename Number>
  [[nodiscard]] std::expected<Number, parse_error> parse_floating(std::string_view token, std::size_t position)
  {
    Number result{};
    const auto *begin = token.data();
    const auto *end = token.data() + token.size();
    const auto [ptr, ec] = std::from_chars(begin, end, result, std::chars_format::general);
    if (ec != std::errc{} || ptr != end)
    {
      return std::unexpected(parse_error{ .message = "Invalid floating-point value", .position = position });
    }

    if (!std::isfinite(result))
    {
      return std::unexpected(parse_error{ .message = "Non-finite floating-point value", .position = position });
    }
    return result;
  }

  template<typename Variant, std::size_t Index>
  [[nodiscard]] std::expected<Variant, parse_error> parse_variant_alternative(std::string_view raw_value,
                                                                              std::size_t raw_value_offset)
  {
    using AltType = std::variant_alternative_t<Index, Variant>;
    json_reader value_reader(raw_value, raw_value_offset);
    auto alt = read_value<AltType>(value_reader);
    if (!alt.has_value())
    {
      return std::unexpected(alt.error());
    }
    value_reader.skip_whitespace();
    if (!value_reader.at_end())
    {
      return std::unexpected(
        parse_error{ .message = "Trailing characters in variant value", .position = value_reader.position() });
    }
    return Variant{ std::in_place_index<Index>, std::move(alt.value()) };
  }

  template<typename Variant, std::size_t Index = 0>
  [[nodiscard]] std::expected<Variant, parse_error> parse_variant_by_index(std::size_t variant_index,
                                                                            std::string_view raw_value,
                                                                            std::size_t raw_value_offset,
                                                                            std::size_t index_position)
  {
    if constexpr (Index >= std::variant_size_v<Variant>)
    {
      return std::unexpected(parse_error{ .message = "Unknown variant index " + std::to_string(variant_index),
                                          .position = index_position });
    }
    else
    {
      if (variant_index == Index)
      {
        return parse_variant_alternative<Variant, Index>(raw_value, raw_value_offset);
      }
      return parse_variant_by_index<Variant, Index + 1>(variant_index, raw_value, raw_value_offset, index_position);
    }
  }

  template<typename Variant, std::size_t Index = 0>
  [[nodiscard]] std::expected<Variant, parse_error> parse_variant_by_name(std::string_view type_name,
                                                                          std::string_view raw_value,
                                                                          std::size_t raw_value_offset,
                                                                          std::size_t type_position)
  {
    if constexpr (Index >= std::variant_size_v<Variant>)
    {
      return std::unexpected(parse_error{
        .message = std::string("Unknown variant type '") + std::string(type_name) + "'", .position = type_position });
    }
    else
    {
      using AltType = std::variant_alternative_t<Index, Variant>;
      if (type_name == std::meta::identifier_of(^^AltType))
      {
        return parse_variant_alternative<Variant, Index>(raw_value, raw_value_offset);
      }
      return parse_variant_by_name<Variant, Index + 1>(type_name, raw_value, raw_value_offset, type_position);
    }
  }

  struct object_field_value
  {
    std::string_view value;
    std::size_t offset = 0;
  };

  using object_field_views = std::unordered_map<std::string, object_field_value>;

  template<typename ReflectedType>
  std::expected<void, parse_error> assign_members_for_type(ReflectedType &object,
                                                            const object_field_views &field_views)
  {
    constexpr auto bases = std::define_static_array(
      std::meta::bases_of(^^ReflectedType, std::meta::access_context::unchecked()));
    template for (constexpr auto base : bases)
    {
      using BaseType = [:std::meta::type_of(base):];
      auto base_result = assign_members_for_type<BaseType>(object, field_views);
      if (!base_result.has_value())
      {
        return std::unexpected(base_result.error());
      }
    }

    constexpr auto members = std::define_static_array(
      std::meta::nonstatic_data_members_of(^^ReflectedType, std::meta::access_context::unchecked()));
    template for (constexpr auto member : members)
    {
      constexpr auto name = std::meta::identifier_of(member);
      auto it = field_views.find(std::string(name));
      if (it == field_views.end())
      {
        continue;
      }

      using RawMemberType = [:std::meta::type_of(member):];
      using MemberType = std::remove_cvref_t<RawMemberType>;
      json_reader member_reader(it->second.value, it->second.offset);
      auto parsed = read_value<MemberType>(member_reader);
      if (!parsed.has_value())
      {
        return std::unexpected(parse_error{ .message = std::string("Failed to parse member '") + std::string(name)
                                                        + "': " + parsed.error().message,
                                            .position = parsed.error().position });
      }

      member_reader.skip_whitespace();
      if (!member_reader.at_end())
      {
        return std::unexpected(
          parse_error{ .message = std::string("Trailing characters for member '") + std::string(name) + "'",
                        .position = member_reader.position() });
      }

      object.[:member:] = std::move(parsed.value());
    }

    return {};
  }

  inline std::expected<object_field_views, parse_error> parse_object_field_views(json_reader &reader)
  {
    auto open = reader.read_char('{');
    if (!open.has_value())
    {
      return std::unexpected(open.error());
    }

    object_field_views field_views;
    reader.skip_whitespace();
    if (!reader.at_end() && reader.current() == '}')
    {
      auto close = reader.read_char('}');
      if (!close.has_value())
      {
        return std::unexpected(close.error());
      }
      return field_views;
    }

    while (true)
    {
      auto key = reader.read_string();
      if (!key.has_value())
      {
        return std::unexpected(key.error());
      }

      auto colon = reader.read_char(':');
      if (!colon.has_value())
      {
        return std::unexpected(colon.error());
      }

      auto value_view = reader.consume_value_view();
      if (!value_view.has_value())
      {
        return std::unexpected(value_view.error());
      }
      field_views.insert_or_assign(std::move(key.value()),
                                    object_field_value{ .value = value_view.value().view,
                                                        .offset = value_view.value().position });

      reader.skip_whitespace();
      if (!reader.at_end() && reader.current() == ',')
      {
        auto comma = reader.read_char(',');
        if (!comma.has_value())
        {
          return std::unexpected(comma.error());
        }
        continue;
      }
      break;
    }

    auto close = reader.read_char('}');
    if (!close.has_value())
    {
      return std::unexpected(close.error());
    }

    return field_views;
  }

  template<typename Tuple, std::size_t Index = 0>
  std::expected<void, parse_error> parse_tuple_elements(Tuple &tuple, json_reader &reader)
  {
    if constexpr (Index == std::tuple_size_v<Tuple>)
    {
      return {};
    }
    else
    {
      if constexpr (Index > 0)
      {
        auto comma = reader.read_char(',');
        if (!comma.has_value())
        {
          return std::unexpected(
            parse_error{ .message = "Tuple size does not match JSON array size", .position = reader.position() });
        }
      }

      using ElementType = std::tuple_element_t<Index, Tuple>;
      auto element = read_value<ElementType>(reader);
      if (!element.has_value())
      {
        return std::unexpected(element.error());
      }

      std::get<Index>(tuple) = std::move(element.value());
      return parse_tuple_elements<Tuple, Index + 1>(tuple, reader);
    }
  }

} // namespace json::detail
