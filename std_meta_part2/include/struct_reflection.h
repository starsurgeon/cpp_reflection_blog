#include <meta>
#include <utility>

struct MyStruct
{
private:
  struct Hidden
  {
  };

public:
  int m;
};

consteval std::pair<int, int> demo_access_context()
{
  using namespace std::meta;

  const auto pub = members_of(^^MyStruct, access_context::unprivileged());
  const auto publicCount = pub.size();

  const auto all = members_of(^^MyStruct, access_context::unchecked());
  const auto overallCount = all.size();

  return { publicCount, overallCount };
}

int main()
{
  constexpr auto counts = demo_access_context();
  static_assert(counts.first == 7, "Expected 7 public members in MyStruct");
  static_assert(counts.second == 8, "Expected 8 overall members in MyStruct");
}