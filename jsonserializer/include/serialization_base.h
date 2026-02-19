#include "json.h"

template<typename DERIVED>
class serialization_base
{
  friend DERIVED;

private:
  serialization_base() = default;

public:
  [[nodiscard]] constexpr std::string to_json() const
  {
    return json::to_json(static_cast<const DERIVED &>(*this));
  }

  [[nodiscard]] static constexpr DERIVED from_json(const std::string &json_text)
  {
    return json::from_json<DERIVED>(json_text);
  }
};

#ifdef SHOW_EXAMPLE_USAGE

#include <print>
#include <utility>

class x : public serialization_base<x>
{
public:
  x() = default;
  x(int a, std::string b) : a(a), b(std::move(b))
  {
  }
  int a{};
  std::string b;
};

int main()
{
  x obj{ 42, "Hello" };
  std::string json_text = obj.to_json();
  std::println("Serialized JSON: {}", json_text);

  x new_obj = x::from_json(json_text);
  std::println("Deserialized Object: a={}, b={}", new_obj.a, new_obj.b);

  assert(obj.a == new_obj.a);
  assert(obj.b == new_obj.b);

  // Test JSON syntax validation
  // json_syntax_validator validator(json_text);
  // assert(validator.validate());

  std::println("All tests passed!");
}

#endif
