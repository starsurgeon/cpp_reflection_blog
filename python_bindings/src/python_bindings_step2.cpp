#include <algorithm>
#include <array>
#include <meta>
#include <print>
#include <pybind11/pybind11.h>
#include <string_view>
#include <type_traits>
#include <utility>

namespace py = pybind11;

#ifdef GCC
struct doc
{
  char const *value;
};
#else
template<std::size_t N>
struct fixed_string
{
  std::array<char, N> chars{};

  template<typename S>
    requires(std::is_array_v<std::remove_reference_t<S>>
             && std::is_same_v<std::remove_cv_t<std::remove_extent_t<std::remove_reference_t<S>>>, char>
             && std::extent_v<std::remove_reference_t<S>> == N)
  consteval fixed_string(const S &s)
  {
    std::ranges::copy(s, chars.begin());
  }

  [[nodiscard]] constexpr std::string_view sv() const
  {
    return { chars.data(), N - 1 };
  }
};

template<typename S>
  requires(std::is_array_v<std::remove_reference_t<S>>
           && std::is_same_v<std::remove_cv_t<std::remove_extent_t<std::remove_reference_t<S>>>, char>)
fixed_string(const S &) -> fixed_string<std::extent_v<std::remove_reference_t<S>>>;

template<fixed_string Text>
struct doc
{
  static constexpr auto text = Text;
};
#endif

namespace py_export
{
#ifdef GCC
  [[nodiscard]][[= doc{ "Add two integers and return the sum." }]] int add(int i, int j)
#else
  [[nodiscard]][[= doc<"Add two integers and return the sum.">{}]] int add(int i, int j)
#endif
  {
    return i + j;
  }

#ifdef GCC
  [[= doc{ "Multiply two floating-point numbers." }]] double multiply(double a, double b)
  {
    return a * b;
  }
#else
  [[= doc<"Multiply two floating-point numbers.">{}]] double multiply(double a, double b)
  {
    return a * b;
  }
#endif

#ifdef GCC
  [[= doc{ "Generate a personalized greeting message." }]] std::string greet(const std::string &name)
#else
  [[= doc<"Generate a personalized greeting message.">{}]] std::string greet(const std::string &name)
#endif
  {
    return "Hello, " + name + "!";
  }

  // function without parameters to test edge case
#ifdef GCC
  [[= doc{ "Print 'Hello, World!' to the console." }]] void say_hello()
#else
  [[= doc<"Print 'Hello, World!' to the console.">{}]] void say_hello()
#endif
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

// Verify annotations
static_assert(std::meta::annotations_of(^^py_export::add).size() == 1, "Expected one doc annotation on add function");

#ifdef GCC
static_assert(std::meta::annotations_of_with_type(^^py_export::multiply, ^^doc).size() == 1,
              "Expected exactly one doc annotation on multiply");
static_assert(std::string_view(std::meta::extract<doc>(std::meta::annotations_of(^^py_export::multiply)[0]).value)
                == "Multiply two floating-point numbers.",
              "Expected multiply doc annotation text to be 'value'");
#else
static_assert(std::meta::annotations_of(^^py_export::multiply).size() == 1,
              "Expected 1 annotation on multiply function");
using multiply_doc_annotation_t = decltype(std::meta::extract<doc<"Multiply two floating-point numbers.">>(
  std::meta::annotations_of(^^py_export::multiply)[0]));
static_assert(multiply_doc_annotation_t::text.sv() == "Multiply two floating-point numbers.",
              "Expected multiply doc annotation text to match");
#endif

template<auto FInfo>
consteval const char *docstring_from_annotation()
{
#ifdef GCC
  constexpr auto docs = std::meta::annotations_of_with_type(FInfo, ^^doc);
  static_assert(
    docs.size() == 1,
    "Missing or ambiguous doc annotation: each exported function must have exactly one [[=doc{...}]] annotation");
  constexpr auto d = std::meta::extract<doc>(docs[0]);
  static_assert(d.value != nullptr && d.value[0] != '\0', "Doc annotation text must not be empty");
  return d.value;
#else
  constexpr auto docs = std::define_static_array(std::meta::annotations_of(FInfo));
  static_assert(
    docs.size() == 1,
    "Missing or ambiguous doc annotation: each exported function must have exactly one [[=doc<...>{}]] annotation");
  using doc_type = [:std::meta::type_of(docs[0]):];
  static_assert(!doc_type::text.sv().empty(), "Doc annotation text must not be empty");
  return std::define_static_string(doc_type::text.sv());
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
  constexpr auto doc = docstring_from_annotation<FInfo>();
  m.def(name, &[:FInfo:], doc, py::arg(param_name_at<FInfo, I>())...);
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
