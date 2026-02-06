#include <meta>
#include <optional>
#include <print>
#include <string_view>

using namespace std;
using namespace std::meta;

template<typename E>
constexpr string_view enum_to_string(E value)
{
  template for (constexpr auto e : define_static_array(enumerators_of(^^E)))
  {
    if (value == [:e:])
    {
      return identifier_of(e);
    }
  }
  return "<unnamed>";
}

template<typename E>
constexpr optional<E> string_to_enum(string_view name)
{
  template for (constexpr auto e : define_static_array(enumerators_of(^^E)))
  {
    if (name == identifier_of(e))
    {
      return [:e:];
    }
  }
  return nullopt;
}

int main()
{
  enum class Color : int
  {
    red,
    green,
    blue
  };

  static_assert(enum_to_string(Color::red) == "red");
  static_assert(enum_to_string(Color(42)) == "<unnamed>");
  static_assert(string_to_enum<Color>("green") == Color::green);
  static_assert(!string_to_enum<Color>("nope").has_value());

  println("Color::red  -> {}", enum_to_string(Color::red));
  println("\"blue\"     -> {}", ( int ) *string_to_enum<Color>("blue"));
  println("\"nope\"     -> {}", string_to_enum<Color>("nope").has_value());
}