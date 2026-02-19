#pragma once

#include <vector>

#include "json_common.h"

namespace json
{

  namespace detail
  {
    // NOLINTBEGIN(readability-function-cognitive-complexity)
    template<typename T>
    std::expected<T, parse_error> read_value(json_reader &reader) // NOLINT(readability-function-cognitive-complexity)
    {
      using Decayed = std::remove_cvref_t<T>;

      if constexpr (bool_like<Decayed>)
      {
        reader.skip_whitespace();
        if (reader.remaining().starts_with("true"))
        {
          auto ok = reader.read_literal("true");
          if (!ok.has_value())
          {
            return std::unexpected(ok.error());
          }
          return true;
        }

        if (reader.remaining().starts_with("false"))
        {
          auto ok = reader.read_literal("false");
          if (!ok.has_value())
          {
            return std::unexpected(ok.error());
          }
          return false;
        }

        return std::unexpected(parse_error{ .message = "Expected boolean", .position = reader.position() });
      }
      else if constexpr (integral_like<Decayed>)
      {
        const auto number_position = reader.position();
        auto token = reader.read_number_token();
        if (!token.has_value())
        {
          return std::unexpected(token.error());
        }
        return parse_integer<Decayed>(token.value(), number_position);
      }
      else if constexpr (floating_like<Decayed>)
      {
        const auto number_position = reader.position();
        auto token = reader.read_number_token();
        if (!token.has_value())
        {
          return std::unexpected(token.error());
        }
        return parse_floating<Decayed>(token.value(), number_position);
      }
      else if constexpr (string_like<Decayed>)
      {
        auto text = reader.read_string();
        if (!text.has_value())
        {
          return std::unexpected(text.error());
        }

        if constexpr (std::same_as<Decayed, std::string>)
        {
          return std::move(text.value());
        }
        else if constexpr (std::constructible_from<Decayed, std::string>)
        {
          return Decayed(text.value());
        }
        else
        {
          static_assert(sizeof(Decayed) == 0, "Target string-like type is not constructible from std::string");
        }
      }
      else if constexpr (std::is_pointer_v<Decayed>
                         && !std::same_as<std::remove_cv_t<std::remove_pointer_t<Decayed>>, char>)
      {
        using Pointee = std::remove_pointer_t<Decayed>;
        using PlainPointee = std::remove_cv_t<Pointee>;
        static_assert(!std::same_as<PlainPointee, void>, "void pointers are not deserializable");

        reader.skip_whitespace();
        if (reader.remaining().starts_with("null"))
        {
          auto ok = reader.read_literal("null");
          if (!ok.has_value())
          {
            return std::unexpected(ok.error());
          }
          return static_cast<Decayed>(nullptr);
        }

        auto inner = read_value<PlainPointee>(reader);
        if (!inner.has_value())
        {
          return std::unexpected(inner.error());
        }

        auto owned = std::make_unique<PlainPointee>(std::move(inner.value()));
        return static_cast<Decayed>(owned.release());
      }
      else if constexpr (is_unique_ptr_specialization<Decayed>::value)
      {
        using Element = typename Decayed::element_type;
        using PlainElement = std::remove_cv_t<Element>;

        reader.skip_whitespace();
        if (reader.remaining().starts_with("null"))
        {
          auto ok = reader.read_literal("null");
          if (!ok.has_value())
          {
            return std::unexpected(ok.error());
          }
          return Decayed{};
        }

        auto inner = read_value<PlainElement>(reader);
        if (!inner.has_value())
        {
          return std::unexpected(inner.error());
        }

        using Deleter = typename Decayed::deleter_type;
        if constexpr (std::default_initializable<Deleter>)
        {
          auto owned = std::make_unique<Element>(std::move(inner.value()));
          return Decayed{ owned.release() };
        }
        else
        {
          return std::unexpected(
            parse_error{ .message = "Cannot deserialize unique_ptr with non-default-constructible deleter",
                         .position = reader.position() });
        }
      }
      else if constexpr (is_shared_ptr_specialization<Decayed>::value)
      {
        using Element = typename Decayed::element_type;
        using PlainElement = std::remove_cv_t<Element>;

        reader.skip_whitespace();
        if (reader.remaining().starts_with("null"))
        {
          auto ok = reader.read_literal("null");
          if (!ok.has_value())
          {
            return std::unexpected(ok.error());
          }
          return Decayed{};
        }

        auto inner = read_value<PlainElement>(reader);
        if (!inner.has_value())
        {
          return std::unexpected(inner.error());
        }

        return std::make_shared<Element>(std::move(inner.value()));
      }
      else if constexpr (optional_like<Decayed>)
      {
        reader.skip_whitespace();
        if (reader.remaining().starts_with("null"))
        {
          auto ok = reader.read_literal("null");
          if (!ok.has_value())
          {
            return std::unexpected(ok.error());
          }
          return Decayed{};
        }

        using ValueType = typename Decayed::value_type;
        auto inner = read_value<ValueType>(reader);
        if (!inner.has_value())
        {
          return std::unexpected(inner.error());
        }
        return Decayed{ std::move(inner.value()) };
      }
      else if constexpr (variant_like<Decayed>)
      {
        auto open = reader.read_char('{');
        if (!open.has_value())
        {
          return std::unexpected(open.error());
        }

        std::size_t variant_index = 0;
        bool has_variant_index = false;
        std::size_t variant_index_position = reader.position();
        std::string type_name;
        std::size_t type_name_position = reader.position();
        std::string_view raw_value;
        std::size_t raw_value_offset = 0;
        bool has_raw_value = false;

        reader.skip_whitespace();
        if (!reader.at_end() && reader.current() == '}')
        {
          return std::unexpected(
            parse_error{ .message = "Variant object is missing index/value fields", .position = reader.position() });
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

          if (key.value() == "index")
          {
            variant_index_position = reader.position();
            auto parsed_index = read_value<std::size_t>(reader);
            if (!parsed_index.has_value())
            {
              return std::unexpected(parsed_index.error());
            }
            variant_index = parsed_index.value();
            has_variant_index = true;
          }
          else if (key.value() == "type")
          {
            type_name_position = reader.position();
            auto parsed_type = reader.read_string();
            if (!parsed_type.has_value())
            {
              return std::unexpected(parsed_type.error());
            }
            type_name = std::move(parsed_type.value());
          }
          else if (key.value() == "value")
          {
            auto parsed_value = reader.consume_value_view();
            if (!parsed_value.has_value())
            {
              return std::unexpected(parsed_value.error());
            }
            raw_value = parsed_value.value().view;
            raw_value_offset = parsed_value.value().position;
            has_raw_value = true;
          }
          else
          {
            auto skipped = reader.skip_value();
            if (!skipped.has_value())
            {
              return std::unexpected(skipped.error());
            }
          }

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

        if (!has_raw_value)
        {
          return std::unexpected(
            parse_error{ .message = "Variant value field is missing", .position = reader.position() });
        }

        if (has_variant_index)
        {
          auto parsed_variant = parse_variant_by_index<Decayed>(variant_index, raw_value, raw_value_offset,
                                                                variant_index_position);
          if (!parsed_variant.has_value())
          {
            return std::unexpected(parsed_variant.error());
          }
          return parsed_variant.value();
        }

        if (!type_name.empty())
        {
          auto parsed_variant = parse_variant_by_name<Decayed>(type_name, raw_value, raw_value_offset,
                                                               type_name_position);
          if (!parsed_variant.has_value())
          {
            return std::unexpected(parsed_variant.error());
          }
          return parsed_variant.value();
        }

        return std::unexpected(
          parse_error{ .message = "Variant index field is missing", .position = reader.position() });
      }
      else if constexpr (tuple_like<Decayed>)
      {
        auto open = reader.read_char('[');
        if (!open.has_value())
        {
          return std::unexpected(open.error());
        }

        Decayed tuple{};
        constexpr std::size_t tuple_size = std::tuple_size_v<Decayed>;

        if constexpr (tuple_size > 0)
        {
          auto elements = parse_tuple_elements(tuple, reader);
          if (!elements.has_value())
          {
            return std::unexpected(elements.error());
          }
        }

        reader.skip_whitespace();
        if (!reader.at_end() && reader.current() == ',')
        {
          return std::unexpected(
            parse_error{ .message = "Tuple size does not match JSON array size", .position = reader.position() });
        }

        auto close = reader.read_char(']');
        if (!close.has_value())
        {
          return std::unexpected(close.error());
        }

        return tuple;
      }
      else if constexpr (string_key_map_like<Decayed>)
      {
        using KeyType = typename Decayed::key_type;
        using MappedType = typename Decayed::mapped_type;
        static_assert(std::constructible_from<KeyType, std::string>,
                      "Map key type must be constructible from std::string");

        Decayed map;
        auto open = reader.read_char('{');
        if (!open.has_value())
        {
          return std::unexpected(open.error());
        }

        reader.skip_whitespace();
        if (!reader.at_end() && reader.current() == '}')
        {
          auto close = reader.read_char('}');
          if (!close.has_value())
          {
            return std::unexpected(close.error());
          }
          return map;
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

          auto value = read_value<MappedType>(reader);
          if (!value.has_value())
          {
            return std::unexpected(value.error());
          }

          map.emplace(KeyType{ std::move(key.value()) }, std::move(value.value()));

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

        return map;
      }
      else if constexpr (non_string_map_like<Decayed>)
      {
        using KeyType = typename Decayed::key_type;
        using MappedType = typename Decayed::mapped_type;

        Decayed map;
        auto open = reader.read_char('[');
        if (!open.has_value())
        {
          return std::unexpected(open.error());
        }

        reader.skip_whitespace();
        if (!reader.at_end() && reader.current() == ']')
        {
          auto close = reader.read_char(']');
          if (!close.has_value())
          {
            return std::unexpected(close.error());
          }
          return map;
        }

        while (true)
        {
          auto pair_open = reader.read_char('[');
          if (!pair_open.has_value())
          {
            return std::unexpected(pair_open.error());
          }

          auto key = read_value<KeyType>(reader);
          if (!key.has_value())
          {
            return std::unexpected(key.error());
          }

          auto comma = reader.read_char(',');
          if (!comma.has_value())
          {
            return std::unexpected(
              parse_error{ .message = "Expected ',' between map key and value", .position = reader.position() });
          }

          auto value = read_value<MappedType>(reader);
          if (!value.has_value())
          {
            return std::unexpected(value.error());
          }

          auto pair_close = reader.read_char(']');
          if (!pair_close.has_value())
          {
            return std::unexpected(
              parse_error{ .message = "Expected ']' after map key/value pair", .position = reader.position() });
          }

          map.emplace(std::move(key.value()), std::move(value.value()));

          reader.skip_whitespace();
          if (!reader.at_end() && reader.current() == ',')
          {
            auto next = reader.read_char(',');
            if (!next.has_value())
            {
              return std::unexpected(next.error());
            }
            continue;
          }
          break;
        }

        auto close = reader.read_char(']');
        if (!close.has_value())
        {
          return std::unexpected(close.error());
        }

        return map;
      }
      else if constexpr (stack_like<Decayed>)
      {
        using ValueType = typename Decayed::value_type;
        std::vector<ValueType> elements;

        auto open = reader.read_char('[');
        if (!open.has_value())
        {
          return std::unexpected(open.error());
        }

        reader.skip_whitespace();
        if (!reader.at_end() && reader.current() == ']')
        {
          auto close = reader.read_char(']');
          if (!close.has_value())
          {
            return std::unexpected(close.error());
          }
          return Decayed{};
        }

        while (true)
        {
          auto value = read_value<ValueType>(reader);
          if (!value.has_value())
          {
            return std::unexpected(value.error());
          }
          elements.push_back(std::move(value.value()));

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

        auto close = reader.read_char(']');
        if (!close.has_value())
        {
          return std::unexpected(close.error());
        }

        Decayed result;
        for (auto it = elements.rbegin(); it != elements.rend(); ++it)
        {
          result.push(std::move(*it));
        }
        return result;
      }
      else if constexpr (queue_like<Decayed>)
      {
        using ValueType = typename Decayed::value_type;
        std::vector<ValueType> elements;

        auto open = reader.read_char('[');
        if (!open.has_value())
        {
          return std::unexpected(open.error());
        }

        reader.skip_whitespace();
        if (!reader.at_end() && reader.current() == ']')
        {
          auto close = reader.read_char(']');
          if (!close.has_value())
          {
            return std::unexpected(close.error());
          }
          return Decayed{};
        }

        while (true)
        {
          auto value = read_value<ValueType>(reader);
          if (!value.has_value())
          {
            return std::unexpected(value.error());
          }
          elements.push_back(std::move(value.value()));

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

        auto close = reader.read_char(']');
        if (!close.has_value())
        {
          return std::unexpected(close.error());
        }

        Decayed result;
        for (auto &element : elements)
        {
          result.push(std::move(element));
        }
        return result;
      }
      else if constexpr (priority_queue_like<Decayed>)
      {
        using ValueType = typename Decayed::value_type;
        std::vector<ValueType> elements;

        auto open = reader.read_char('[');
        if (!open.has_value())
        {
          return std::unexpected(open.error());
        }

        reader.skip_whitespace();
        if (!reader.at_end() && reader.current() == ']')
        {
          auto close = reader.read_char(']');
          if (!close.has_value())
          {
            return std::unexpected(close.error());
          }
          return Decayed{};
        }

        while (true)
        {
          auto value = read_value<ValueType>(reader);
          if (!value.has_value())
          {
            return std::unexpected(value.error());
          }
          elements.push_back(std::move(value.value()));

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

        auto close = reader.read_char(']');
        if (!close.has_value())
        {
          return std::unexpected(close.error());
        }

        Decayed result;
        for (auto &element : elements)
        {
          result.push(std::move(element));
        }
        return result;
      }
      else if constexpr (deserializable_range_like<Decayed>)
      {
        using ValueType = std::ranges::range_value_t<Decayed>;

        Decayed range;
        std::vector<ValueType> elements;
        auto open = reader.read_char('[');
        if (!open.has_value())
        {
          return std::unexpected(open.error());
        }

        reader.skip_whitespace();
        if (!reader.at_end() && reader.current() == ']')
        {
          auto close = reader.read_char(']');
          if (!close.has_value())
          {
            return std::unexpected(close.error());
          }
          return range;
        }

        while (true)
        {
          auto value = read_value<ValueType>(reader);
          if (!value.has_value())
          {
            return std::unexpected(value.error());
          }

          elements.push_back(std::move(value.value()));

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

        auto close = reader.read_char(']');
        if (!close.has_value())
        {
          return std::unexpected(close.error());
        }

        if constexpr (requires {
                        range.before_begin();
                        range.insert_after(range.before_begin(), std::declval<ValueType>());
                      })
        {
          auto tail = range.before_begin();
          for (auto &&element : elements)
          {
            tail = range.insert_after(tail, std::move(element));
          }
        }
        else
        {
          for (auto &&element : elements)
          {
            append_to_container(range, std::move(element));
          }
        }

        return range;
      }
      else if constexpr (bitset_like<Decayed>)
      {
        Decayed bits{};

        auto open = reader.read_char('[');
        if (!open.has_value())
        {
          return std::unexpected(open.error());
        }

        std::size_t index = 0;
        reader.skip_whitespace();
        if (!reader.at_end() && reader.current() == ']')
        {
          auto close = reader.read_char(']');
          if (!close.has_value())
          {
            return std::unexpected(close.error());
          }

          if (bits.size() != 0)
          {
            return std::unexpected(
              parse_error{ .message = "Bitset size does not match JSON array size", .position = reader.position() });
          }

          return bits;
        }

        while (true)
        {
          if (index >= bits.size())
          {
            return std::unexpected(
              parse_error{ .message = "Bitset size does not match JSON array size", .position = reader.position() });
          }

          auto value = read_value<bool>(reader);
          if (!value.has_value())
          {
            return std::unexpected(value.error());
          }

          bits.set(index, value.value());
          ++index;

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

        auto close = reader.read_char(']');
        if (!close.has_value())
        {
          return std::unexpected(close.error());
        }

        if (index != bits.size())
        {
          return std::unexpected(
            parse_error{ .message = "Bitset size does not match JSON array size", .position = reader.position() });
        }

        return bits;
      }
      else if constexpr (reflectable_object<Decayed>)
      {
        auto field_views = parse_object_field_views(reader);
        if (!field_views.has_value())
        {
          return std::unexpected(field_views.error());
        }

        if constexpr (std::default_initializable<Decayed>)
        {
          Decayed object{};
          auto assigned = assign_members_for_type(object, field_views.value());
          if (!assigned.has_value())
          {
            return std::unexpected(assigned.error());
          }

          return object;
        }
        else
        {
          return std::unexpected(parse_error{
            .message = "Reflected type is not default-constructible; provide a custom from_json/try_from_json overload",
            .position = reader.position() });
        }
      }
      else
      {
        static_assert(sizeof(T) == 0, "Type not deserializable");
      }
    }
    // NOLINTEND(readability-function-cognitive-complexity)
  } // namespace detail

} // namespace json
