#include <charconv>
#include <meta>
#include <print>
#include <span>
#include <string>
#include <string_view>
#include <vector>

// helper for static_assert false in templates
template<class>
inline constexpr bool dependent_false_v = false;

template<class T>
T parse_value(std::string_view sv)
{
  if constexpr (std::is_same_v<T, std::string>)
  {
    return std::string(sv);
  }
  else if constexpr (std::is_same_v<T, std::string_view>)
  {
    return sv;
  }
  else if constexpr (std::is_same_v<T, bool>)
  {
    // In the post, bool flags are handled as presence-only.
    // Still useful if you ever allow --flag=true
    if (sv == "1" || sv == "true" || sv == "TRUE" || sv == "on")
    {
      return true;
    }
    if (sv == "0" || sv == "false" || sv == "FALSE" || sv == "off")
    {
      return false;
    }
    throw std::invalid_argument("invalid bool: " + std::string(sv));
  }
  else if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>)
  {
    T out{};
    const auto *first = sv.data();
    const auto *last = sv.data() + sv.size();
    auto [ptr, ec] = std::from_chars(first, last, out);
    if (ec != std::errc{} || ptr != last)
    {
      throw std::invalid_argument("invalid integer: " + std::string(sv));
    }
    return out;
  }
  else if constexpr (std::is_floating_point_v<T>)
  {
    // from_chars for float is C++17/C++20-ish but still spotty across lib/compilers.
    // Keep it simple and portable for the blog post:
    std::string s(sv);
    if constexpr (std::is_same_v<T, float>)
    {
      return std::stof(s);
    }
    if constexpr (std::is_same_v<T, double>)
    {
      return std::stod(s);
    }
    return static_cast<T>(std::stold(s));
  }
  else
  {
    static_assert(dependent_false_v<T>, "parse_value<T>: unsupported type");
  }
}

struct Config
{
  std::string host = "localhost";
  int port = 8080;
  bool verbose = false;
};

template<typename T>
void parse_cli(T &obj, std::vector<std::string_view> args)
{
  constexpr auto type = ^^T;
  constexpr auto ctx = std::meta::access_context::current();
  constexpr auto members = define_static_array(std::meta::nonstatic_data_members_of(type, ctx));

  template for (constexpr auto m : members)
  {
    constexpr auto name = identifier_of(m);

    for (std::size_t i = 0; i < args.size(); ++i)
    {
      if (args[i] == std::string("--") + std::string(name))
      {
        using MemberType = [:type_of(m):];

        if constexpr (std::is_same_v<MemberType, bool>)
        {
          obj.[:m:] = true;
        }
        else
        {
          obj.[:m:] = parse_value<MemberType>(args[i + 1]);
        }
      }
    }
  }
}

int main(int argc, char **argv)
{
  Config cfg;
  std::span<char *> argv_span(argv, static_cast<std::size_t>(argc));
  std::vector<std::string_view> args(argv_span.begin() + 1, argv_span.end());

  parse_cli(cfg, args);
  std::print("{}:{} verbose={}\n", cfg.host, cfg.port, cfg.verbose);
}