#pragma once

#include "json_common.h"
#include <vector>

namespace json
{

  // json_writer implementation
  inline void json_writer::prepare_value_write()
  {
    if (needs_comma_)
    {
      write_comma();
    }
    else if (opts_.pretty && !after_key_ && opts_.current_indent > 0)
    {
      write_newline();
      write_indent();
    }
    after_key_ = false;
  }

  inline void json_writer::write_null()
  {
    prepare_value_write();
    buffer_ << "null";
    needs_comma_ = true;
  }

  inline void json_writer::write_bool(bool value)
  {
    prepare_value_write();
    buffer_ << (value ? "true" : "false");
    needs_comma_ = true;
  }

  inline void json_writer::write_string(std::string_view value)
  {
    prepare_value_write();
    buffer_ << '"' << detail::escape_string(value) << '"';
    needs_comma_ = true;
  }

  template<std::integral T>
  void json_writer::write_number(T value)
  {
    prepare_value_write();
    std::array<char, std::numeric_limits<T>::digits10 + 3> buf;
    auto [ptr, ec] = std::to_chars(buf.data(), buf.data() + buf.size(), value);
    if (ec != std::errc{})
    {
      throw std::runtime_error("Failed to convert integer to string");
    }
    buffer_.write(buf.data(), ptr - buf.data());
    needs_comma_ = true;
  }

  template<std::floating_point T>
  void json_writer::write_number(T value)
  {
    if (std::isnan(value) || std::isinf(value))
    {
      throw std::invalid_argument("Cannot serialize NaN or Infinity to JSON");
    }

    prepare_value_write();

    std::array<char, std::numeric_limits<T>::max_digits10 + kFloatBufferExtra> buf;
    auto [ptr, ec] = std::to_chars(buf.data(), buf.data() + buf.size(), value, std::chars_format::general,
                                   std::numeric_limits<T>::max_digits10);
    if (ec != std::errc{})
    {
      throw std::runtime_error("Failed to convert float to string");
    }
    buffer_.write(buf.data(), ptr - buf.data());
    needs_comma_ = true;
  }

  inline void json_writer::begin_array()
  {
    prepare_value_write();
    buffer_ << '[';
    if (opts_.pretty)
    {
      opts_.current_indent += opts_.indent_size;
    }
    needs_comma_ = false;
  }

  inline void json_writer::end_array()
  {
    if (opts_.pretty && needs_comma_)
    {
      write_newline();
      opts_.current_indent -= opts_.indent_size;
      write_indent();
    }
    else if (opts_.pretty)
    {
      opts_.current_indent -= opts_.indent_size;
    }
    buffer_ << ']';
    needs_comma_ = true;
  }

  inline void json_writer::begin_object()
  {
    prepare_value_write();
    buffer_ << '{';
    if (opts_.pretty)
    {
      opts_.current_indent += opts_.indent_size;
    }
    needs_comma_ = false;
  }

  inline void json_writer::end_object()
  {
    if (opts_.pretty && needs_comma_)
    {
      write_newline();
      opts_.current_indent -= opts_.indent_size;
      write_indent();
    }
    else if (opts_.pretty)
    {
      opts_.current_indent -= opts_.indent_size;
    }
    buffer_ << '}';
    needs_comma_ = true;
  }

  inline void json_writer::write_comma()
  {
    buffer_ << ',';
    if (opts_.pretty)
    {
      write_newline();
      write_indent();
    }
    needs_comma_ = false;
    after_key_ = false;
  }

  inline void json_writer::write_key(std::string_view key)
  {
    if (needs_comma_)
    {
      write_comma();
    }
    else if (opts_.pretty && opts_.current_indent > 0)
    {
      write_newline();
      write_indent();
    }
    buffer_ << '"' << detail::escape_string(key) << '"';
    buffer_ << ':';
    if (opts_.pretty)
    {
      buffer_ << ' ';
    }
    needs_comma_ = false;
    after_key_ = true;
  }

  inline void json_writer::write_indent()
  {
    for (int i = 0; i < opts_.current_indent; ++i)
    {
      buffer_ << ' ';
    }
  }

  inline void json_writer::write_newline()
  {
    buffer_ << '\n';
  }

  // Forward declaration
  template<typename T>
  void write_value(json_writer &writer, const T &value, const options &opts);

  template<typename ReflectedType, typename ValueType>
  void write_members_for_type(json_writer &writer, const ValueType &value, const options &opts)
  {
    constexpr auto bases = std::define_static_array(
      std::meta::bases_of(^^ReflectedType, std::meta::access_context::unchecked()));
    template for (constexpr auto base : bases)
    {
      using BaseType = [:std::meta::type_of(base):];
      write_members_for_type<BaseType>(writer, value, opts);
    }

    constexpr auto members = std::define_static_array(
      std::meta::nonstatic_data_members_of(^^ReflectedType, std::meta::access_context::unchecked()));
    template for (constexpr auto member : members)
    {
      constexpr auto name = std::meta::identifier_of(member);
      writer.write_key(name);
      write_value(writer, value.[:member:], opts);
    }
  }

  template<typename T>
  void write_reflected_members(json_writer &writer, const T &value, const options &opts)
  {
    using Decayed = std::remove_cvref_t<T>;
    write_members_for_type<Decayed>(writer, value, opts);
  }

  // Main serialization function using if constexpr for type dispatch
  template<typename T>
  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  void write_value(json_writer &writer, const T &value, const options &opts)
  {
    using Decayed = std::remove_cvref_t<T>;

    if constexpr (std::same_as<Decayed, std::nullptr_t>)
    {
      writer.write_null();
    }
    else if constexpr (bool_like<T>)
    {
      writer.write_bool(value);
    }
    // NOLINTBEGIN(bugprone-branch-clone) because we want to distinguish floating point from integral types
    else if constexpr (integral_like<T>)
    {
      writer.write_number(value);
    }
    else if constexpr (floating_like<T>)
    {
      writer.write_number(value);
    }
    // NOLINTEND(bugprone-branch-clone)
    else if constexpr (string_like<T>)
    {
      if constexpr (std::same_as<Decayed, std::string> || std::same_as<Decayed, std::string_view>)
      {
        writer.write_string(value);
      }
      else if constexpr (std::is_pointer_v<Decayed>
                         && std::same_as<std::remove_cv_t<std::remove_pointer_t<Decayed>>, char>)
      {
        if (value == nullptr)
        {
          writer.write_null();
        }
        else
        {
          writer.write_string(std::string_view(value));
        }
      }
      else
      {
        writer.write_string(std::string_view(value));
      }
    }
    else if constexpr (std::is_pointer_v<Decayed>
                       && !std::same_as<std::remove_cv_t<std::remove_pointer_t<Decayed>>, char>)
    {
      using Pointee = std::remove_pointer_t<Decayed>;
      static_assert(!std::same_as<std::remove_cv_t<Pointee>, void>, "void pointers are not serializable");

      if (value == nullptr)
      {
        writer.write_null();
      }
      else
      {
        write_value(writer, *value, opts);
      }
    }
    else if constexpr (is_unique_ptr_specialization<Decayed>::value || is_shared_ptr_specialization<Decayed>::value)
    {
      if (!value)
      {
        writer.write_null();
      }
      else
      {
        write_value(writer, *value, opts);
      }
    }
    else if constexpr (optional_like<T>)
    {
      if (value.has_value())
      {
        write_value(writer, *value, opts);
      }
      else
      {
        writer.write_null();
      }
    }
    else if constexpr (variant_like<T>)
    {
      writer.begin_object();
      writer.write_key("index");
      writer.write_number(value.index());
      writer.write_key("value");

      std::visit([&writer, &opts](const auto &alternative) { write_value(writer, alternative, opts); }, value);

      writer.end_object();
    }
    else if constexpr (tuple_like<T>)
    {
      writer.begin_array();

      [&]<std::size_t... Is>(std::index_sequence<Is...>) { (write_value(writer, std::get<Is>(value), opts), ...); }(
        std::make_index_sequence<std::tuple_size_v<std::remove_cvref_t<T>>>{});

      writer.end_array();
    }
    else if constexpr (string_key_map_like<T>)
    {
      writer.begin_object();

      for (const auto &[key, val] : value)
      {
        writer.write_key(key);
        write_value(writer, val, opts);
      }

      writer.end_object();
    }
    else if constexpr (non_string_map_like<T>)
    {
      writer.begin_array();

      for (const auto &[key, val] : value)
      {
        writer.begin_array();
        write_value(writer, key, opts);
        write_value(writer, val, opts);
        writer.end_array();
      }

      writer.end_array();
    }
    else if constexpr (stack_like<T>)
    {
      auto copy = value;
      std::vector<typename std::remove_cvref_t<T>::value_type> elements;
      while (!copy.empty())
      {
        elements.push_back(copy.top());
        copy.pop();
      }

      writer.begin_array();
      for (const auto &element : elements)
      {
        write_value(writer, element, opts);
      }
      writer.end_array();
    }
    else if constexpr (queue_like<T>)
    {
      auto copy = value;
      std::vector<typename std::remove_cvref_t<T>::value_type> elements;
      while (!copy.empty())
      {
        elements.push_back(copy.front());
        copy.pop();
      }

      writer.begin_array();
      for (const auto &element : elements)
      {
        write_value(writer, element, opts);
      }
      writer.end_array();
    }
    else if constexpr (priority_queue_like<T>)
    {
      auto copy = value;
      std::vector<typename std::remove_cvref_t<T>::value_type> elements;
      while (!copy.empty())
      {
        elements.push_back(copy.top());
        copy.pop();
      }

      writer.begin_array();
      for (const auto &element : elements)
      {
        write_value(writer, element, opts);
      }
      writer.end_array();
    }
    else if constexpr (range_like<T>)
    {
      using RangeType = std::remove_cvref_t<T>;
      using RangeValueType = std::ranges::range_value_t<RangeType>;
      using RangeReferenceType = std::ranges::range_reference_t<RangeType>;

      writer.begin_array();

      for (const auto &element : value)
      {
        if constexpr (std::same_as<std::remove_cvref_t<RangeReferenceType>, RangeValueType>)
        {
          write_value(writer, element, opts);
        }
        else if constexpr (std::convertible_to<RangeReferenceType, RangeValueType>)
        {
          write_value(writer, static_cast<RangeValueType>(element), opts);
        }
        else
        {
          write_value(writer, element, opts);
        }
      }

      writer.end_array();
    }
    else if constexpr (bitset_like<T>)
    {
      writer.begin_array();

      for (std::size_t index = 0; index < value.size(); ++index)
      {
        writer.write_bool(value.test(index));
      }

      writer.end_array();
    }
    else if constexpr (reflectable_object<T>)
    {
      writer.begin_object();
      write_reflected_members(writer, value, opts);
      writer.end_object();
    }
    else
    {
      static_assert(sizeof(T) == 0, "Type not serializable");
    }
  }

} // namespace json
