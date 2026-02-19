#pragma once

#include <bitset>
#include <concepts>
#include <cstddef>
#include <expected>
#include <experimental/meta>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace json
{

  template<typename T>
  struct is_bitset_specialization : std::false_type
  {
  };

  template<std::size_t N>
  struct is_bitset_specialization<std::bitset<N>> : std::true_type
  {
  };

  // Configuration options for JSON serialization
  struct options
  {
    bool pretty = false;
    int indent_size = 2;
    int current_indent = 0;
  };

  // Forward declarations
  template<typename T>
  std::string to_json(const T &value, const options &opts = {});

  struct parse_error
  {
    std::string message;
    std::size_t position = 0;
  };

  template<typename T>
  std::expected<T, parse_error> try_from_json(std::string_view json_text);

  template<typename T>
  T from_json(std::string_view json_text);

  // Serializable concept
  template<typename T>
  concept serializable = requires(const T &value, const options &opts) {
    { to_json(value, opts) } -> std::same_as<std::string>;
  };

  // Internal writer class for efficient JSON generation
  class json_writer
  {
  public:
    explicit json_writer(const options &opts = {}) : opts_(opts)
    {
    }

    void write_null();
    void write_bool(bool value);
    void write_string(std::string_view value);

    template<std::integral T>
    void write_number(T value);

    template<std::floating_point T>
    void write_number(T value);

    void begin_array();
    void end_array();
    void begin_object();
    void end_object();
    void write_comma();
    void write_key(std::string_view key);

    std::string result() const
    {
      return buffer_.str();
    }

  private:
    void prepare_value_write();
    void write_indent();
    void write_newline();

    options opts_;
    std::ostringstream buffer_;
    bool needs_comma_ = false;
    bool after_key_ = false;
  };

  // Type categorization concepts
  template<typename T>
  concept bool_like = std::same_as<std::remove_cvref_t<T>, bool>;

  template<typename T>
  concept integral_like = std::integral<std::remove_cvref_t<T>> && !bool_like<T>
                          && !std::same_as<std::remove_cvref_t<T>, char>;

  template<typename T>
  concept floating_like = std::floating_point<std::remove_cvref_t<T>>;

  template<typename T>
  concept string_like = requires(const T &value) {
    { std::string_view(value) } -> std::same_as<std::string_view>;
  } || std::same_as<std::remove_cvref_t<T>, char *> || std::same_as<std::remove_cvref_t<T>, const char *>;

  template<typename T>
  concept optional_like = requires(const T &value) {
    typename T::value_type;
    { value.has_value() } -> std::same_as<bool>;
    { *value } -> std::convertible_to<const typename T::value_type &>;
  };

  template<typename T>
  concept variant_like = requires(const T &value) {
    {
      std::visit([](const auto &) {}, value)
    };
  };

  template<typename T>
  concept tuple_size_defined = requires { typename std::tuple_size<std::remove_cvref_t<T>>::type; };

  template<typename T>
  concept tuple_like = tuple_size_defined<T> && !string_like<T> && !variant_like<T>
                       && (std::tuple_size_v<std::remove_cvref_t<T>> == 0
                           || requires { std::get<0>(std::declval<std::remove_cvref_t<T> &>()); });

  template<typename T>
  concept map_like = requires(const T &value) {
    typename std::remove_cvref_t<T>::key_type;
    typename std::remove_cvref_t<T>::mapped_type;
    { value.begin() } -> std::input_iterator;
    { value.end() } -> std::input_iterator;
  };

  template<typename T>
  concept string_key_map_like = map_like<T> && string_like<typename std::remove_cvref_t<T>::key_type>;

  template<typename T>
  concept non_string_map_like = map_like<T> && !string_key_map_like<T>;

  template<typename T>
  concept range_like = std::ranges::range<T> && !string_like<T> && !map_like<T> && !tuple_like<T>;

  template<typename T>
  concept bitset_like = is_bitset_specialization<std::remove_cvref_t<T>>::value;

  template<typename T>
  concept insertable_range = requires(std::remove_cvref_t<T> &container, std::ranges::range_value_t<T> value) {
    container.push_back(value);
  } || requires(std::remove_cvref_t<T> &container, std::ranges::range_value_t<T> value) {
    container.emplace_back(value);
  } || requires(std::remove_cvref_t<T> &container, std::ranges::range_value_t<T> value) {
    container.insert(value);
  } || requires(std::remove_cvref_t<T> &container, std::ranges::range_value_t<T> value) {
    container.emplace(value);
  } || requires(std::remove_cvref_t<T> &container, std::ranges::range_value_t<T> value) {
    container.insert_after(container.before_begin(), value);
  } || requires(std::remove_cvref_t<T> &container, std::ranges::range_value_t<T> value) {
    container.push_front(value);
  };

  template<typename T>
  concept deserializable_range_like = range_like<T> && std::default_initializable<std::remove_cvref_t<T>>
                                      && insertable_range<T>;

  template<typename T>
  concept stack_like = requires(T &container, typename std::remove_cvref_t<T>::value_type value) {
    container.push(value);
    container.pop();
    { container.top() };
    { container.empty() } -> std::same_as<bool>;
  };

  template<typename T>
  concept queue_like = requires(T &container, typename std::remove_cvref_t<T>::value_type value) {
    container.push(value);
    container.pop();
    { container.front() };
    { container.empty() } -> std::same_as<bool>;
  };

  template<typename T>
  concept priority_queue_like = requires(T &container, typename std::remove_cvref_t<T>::value_type value) {
    container.push(value);
    container.pop();
    { container.top() };
    { container.empty() } -> std::same_as<bool>;
  };

  template<typename T>
  concept reflectable_object = std::is_class_v<T> && !optional_like<T> && !variant_like<T> && !tuple_like<T>
                               && !range_like<T> && !map_like<T> && !bitset_like<T> && !stack_like<T> && !queue_like<T>
                               && !priority_queue_like<T>;

} // namespace json
