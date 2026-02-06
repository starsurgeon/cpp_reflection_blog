#include <algorithm>
#include <array>
#include <cstdint>
#include <format>
#include <meta>
#include <print>
#include <string>
#include <type_traits>

// Note: I would have like to use modules here, but module support is explicitely disabled in the experimental compiler build.

using namespace std::meta;
using namespace std;

enum class lineType : std::uint8_t
{
  None,
  Solid,
  Dashed,
  Dotted
};

template<typename T>
concept Enum = is_enum_v<T>;

template<Enum E>
struct enumItem
{
  string_view name;
  E value{};
};

template<Enum E>
consteval auto getEnumData()
{
  constexpr auto values = define_static_array(enumerators_of(^^E));
  array<enumItem<E>, values.size()> result;
  int k = 0;
  for (auto mem : values)
  {
    result[k++] = enumItem<E>{ identifier_of(mem), extract<E>(mem) };
  }
  return result;
}

// compute enum data at compile time, usable at runtime
template<Enum E>
static inline constexpr auto enum_data = getEnumData<E>();

template<Enum E>
consteval std::string_view toString(E v)
{
  using std::ranges::find_if;
  const auto it = find_if(enum_data<E>, [&](auto const &item) { return item.value == v; });
  return (it != end(enum_data<E>)) ? it->name : "<unknown>";
}

template<Enum E>
constexpr std::optional<E> string_to_enum(std::string_view name)
{
  static constexpr auto data = getEnumData<E>();
  static constexpr auto end = std::end(data);
  const auto it = std::ranges::find_if(data, [&](auto const &item) { return item.name == name; });
  if (it != end)
  {
    return it->value;
  }

  return std::nullopt;
}

int main(int argc, char **argv)
{
  const auto lt = static_cast<lineType>(42);
  static_assert(enum_data<lineType>.size() == 4);
  static_assert(toString(lineType::Dashed) == "Dashed");
  static_assert(toString(lt) == "<unknown>");
  using underlyingEnumType = underlying_type_t<lineType>;
  for (const auto &item : enum_data<lineType>)
  {
    std::println("Name: {}, Value: {}", item.name, static_cast<underlyingEnumType>(item.value));
  }

  auto optEnum = string_to_enum<lineType>("Dotted");
  if (optEnum)
  {
    std::println("Dotted has value: {}", static_cast<underlyingEnumType>(*optEnum));
  }
  else
  {
    std::println("Enum not found");
  }

  std::string input = "Solid";
  if (auto optEnum2 = string_to_enum<lineType>(input); optEnum2)
  {
    std::println("Parsed {} to enum value {}", input, static_cast<underlyingEnumType>(*optEnum2));
  }
  else
  {
    std::println("Failed to parse enum from string: {}", input);
  }

  return 0;
}
