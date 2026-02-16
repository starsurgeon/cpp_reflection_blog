#include <meta>
#include <print>
#include <string>

struct S {
  int a{};
  std::string s;
};

template <typename T> void dump_members(T const &obj) {
  using namespace std::meta;
  constexpr auto ctx = access_context::current();

  template for (constexpr auto m :
                std::define_static_array(nonstatic_data_members_of(^^T, ctx))) {
    // Each iteration has a different compile-time member reflection 'm'
    std::println("{} = {}", identifier_of(m), obj.[:m:]);
  }
}

int main() {
  S s{.a = 42, .s = "Hello"};
  dump_members(s);
}

// prints
// a = 42
// s = Hello