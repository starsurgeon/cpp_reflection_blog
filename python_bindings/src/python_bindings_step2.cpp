#include <algorithm>
#include <array>
#include <meta>
#include <print>
#include <pybind11/pybind11.h>
#include <string_view>
#include <type_traits>
#include <utility>

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

#ifdef COMPILE_TIME_ERROR_TEST
  int subtract(int a, int b)
  {
    return a - b;
  }
#endif
}


template<auto FInfo, std::size_t I>
consteval const char *param_name_at()
{
  constexpr auto params = std::define_static_array(std::meta::parameters_of(FInfo));
  return std::define_static_string(std::meta::identifier_of(params[I]));
}

template<auto FInfo, std::size_t... I>
void bind_with_named_args_impl(py::module_ &m, const char *name, [[maybe_unused]] std::index_sequence<I...> indices)
{
  m.def(name, &[:FInfo:], py::arg(param_name_at<FInfo, I>())...);
}

template<auto FInfo>
void bind_with_named_args(py::module_ &m, const char *name)
{
  constexpr auto params = std::define_static_array(std::meta::parameters_of(FInfo));
  bind_with_named_args_impl<FInfo>(m, name, std::make_index_sequence<params.size()>{});
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
      bind_with_named_args<f>(m, name.data());
    }
  }
}
