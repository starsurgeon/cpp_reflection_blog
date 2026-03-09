#include <meta>
#include <print>
#include <string>
#include <string_view>
#include <vector>

constexpr static int max_age = 120;

struct ValidationError
{
  std::string path;
  std::string message;
};

inline void validate_value(const std::string &value, std::string_view path, std::vector<ValidationError> &errors)
{
  if (value.empty())
  {
    errors.push_back({ std::string(path), "must not be empty" });
  }
}

inline void validate_value(int value, std::string_view path, std::vector<ValidationError> &errors)
{
  if (value < 0 || value > max_age)
  {
    errors.push_back({ std::string(path), "must be between 0 and 120" });
  }
}

namespace meta = std::meta;

template<typename T>
concept ReflectableObject = std::is_class_v<T> && !std::same_as<std::remove_cvref_t<T>, std::string>
                            && !meta::is_consteval_only_type(^^T);

template<typename T>
void validate_object(const T &obj, std::string_view path, std::vector<ValidationError> &errors)
{
  constexpr auto ctx = meta::access_context::current();
  template for (constexpr auto member : std::define_static_array(meta::nonstatic_data_members_of(^^T, ctx)))
  {
    constexpr auto name = meta::identifier_of(member);
    std::string child_path = path.empty() ? std::string(name) : std::string(path) + "." + std::string(name);
    validate(obj.[:member:], child_path, errors);
  }
}

template<typename T>
void validate(const T &value, std::string_view path, std::vector<ValidationError> &errors)
{
  if constexpr (ReflectableObject<T>)
  {
    validate_object(value, path, errors);
  }
  else
  {
    validate_value(value, path, errors);
  }
}

template<typename T>
std::vector<ValidationError> validate(const T &value)
{
  std::vector<ValidationError> errors;
  validate(value, "", errors);
  return errors;
}

struct Address
{
  std::string street;
  std::string city;
};

struct User
{
  std::string name;
  int age;
  Address address;
};

int main()
{
  User u{ .name = "", .age = -1, .address = { .street = "", .city = "Berlin" } };
  for (auto const &e : validate(u))
  {
    std::println("{}: {}", e.path, e.message);
  }
}