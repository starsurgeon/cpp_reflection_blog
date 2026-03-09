#include <meta>
#include <print>
#include <pybind11/pybind11.h>

namespace py = pybind11;

namespace py_export
{
  int add(int i, int j)
  {
    return i + j;
  }

  double multiply(double a, double b)
  {
    return a * b;
  }

  std::string greet(const std::string &name)
  {
    return "Hello, " + name + "!";
  }

  // function without parameters to test edge case
  void say_hello()
  {
    std::println("{}", "Hello, World!");
  }

}

PYBIND11_MODULE(example, m)
{
  m.doc() = "pybind11 example plugin";

  constexpr auto ctx = std::meta::access_context::unchecked();
  constexpr auto function_list = std::define_static_array(std::meta::members_of(^^py_export, ctx));

  template for (constexpr auto f : function_list)
  {
    if constexpr (!std::meta::is_special_member_function(f))
    {
      constexpr auto name = std::meta::display_string_of(f);
      // extremly simple binding without named arguments for demonstration purposes
      m.def(name.data(), &[:f:]);
    }
  }
}
