#include <cstdint>
#include <meta>

namespace demo
{
  struct S
  {
    int x;
  };

  enum class E : std::uint8_t
  {
    A = 1,
    B = 2
  };
  void f();
  inline constexpr int k = 42;
}

consteval int count_things_in_demo()
{
  using namespace std::meta;

  constexpr auto ctx = access_context::current();
  auto ms = members_of(^^demo, ctx);

  int types = 0;
  int funcs = 0;
  int vars = 0;

  for (info m : ms)
  {
    if (is_type(m))
    {
      ++types;
    }
    if (is_function(m))
    {
      ++funcs;
    }
    if (is_variable(m))
    {
      ++vars;
    }
  }
  // S + E, f, k => 2 / 1 / 1 => overall 4
  return types + funcs + vars;
}

int main()
{
  constexpr int count = count_things_in_demo();
  static_assert(count == 4, "Expected 4 members in namespace 'demo'");
}