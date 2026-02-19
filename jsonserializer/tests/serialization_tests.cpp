#include <array>
#include <bitset>
#include <cassert>
#include <cctype>
#include <deque>
#include <forward_list>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <span>
#include <stack>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "json.h"

// disable clang-tidy warnings for magic numbers in test cases
// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)

// Test reflection with different structs
struct Person
{
  std::string name;
  int age;
  bool employed;
};

struct Address
{
  std::string street;
  std::string city;
  int zip_code;
};

struct Employee
{
  Person person;
  Address address;
  double salary;
  std::optional<std::string> department;
};

// Test with private members
class Account
{
private:
  std::string id_;
  double balance_;

public:
  Account(std::string id, double balance) : id_(std::move(id)), balance_(balance)
  {
  }

  [[nodiscard]] const std::string &id() const
  {
    return id_;
  }
  [[nodiscard]] double balance() const
  {
    return balance_;
  }
};

struct PublicBase
{
  int public_value = 0;
};

class ProtectedBase
{
protected:
  
  // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes) - this is intentional for testing protected member serialization
  std::string protected_value; 

  explicit ProtectedBase(std::string value) : protected_value(std::move(value))
  {
  }
};

class PrivateBase
{
private:
  double private_value_ = 0.0;

public:
  explicit PrivateBase(double value) : private_value_(value)
  {
  }
};

class DerivedFromAll : public PublicBase, public ProtectedBase, public PrivateBase
{
public:
  DerivedFromAll(int pub, std::string prot, double priv, bool active) :
    PublicBase{ pub }, ProtectedBase(std::move(prot)), PrivateBase(priv), active_(active)
  {
  }

private:
  bool active_ = false;
};

struct BaseProfile
{
  int id = 0;
};

struct BaseAudit
{
  bool active = false;
};

struct UserRecord : public BaseProfile, public BaseAudit
{
  std::string name;
};

static_assert(json::deserializable_range_like<std::vector<int>>);
static_assert(!json::deserializable_range_like<std::span<int>>);

// Helper function to check if JSON contains expected substring
bool contains(const std::string &json, const std::string &substr)
{
  return json.find(substr) != std::string::npos;
}

namespace
{
  class json_syntax_validator
  {
  public:
    explicit json_syntax_validator(std::string_view input) : input_(input)
    {
    }

    [[nodiscard]] bool validate()
    {
      skip_whitespace();
      if (!parse_value())
      {
        return false;
      }
      skip_whitespace();
      return position_ == input_.size();
    }

  private:
    std::string_view input_;
    std::size_t position_ = 0;

    [[nodiscard]] bool at_end() const
    {
      return position_ >= input_.size();
    }

    [[nodiscard]] char current() const
    {
      return input_[position_];
    }

    void skip_whitespace()
    {
      while (!at_end() && std::isspace(static_cast<unsigned char>(current())) != 0)
      {
        ++position_;
      }
    }

    [[nodiscard]] bool parse_value()
    {
      skip_whitespace();
      if (at_end())
      {
        return false;
      }

      switch (current())
      {
      case '{':
        return parse_object();
      case '[':
        return parse_array();
      case '"':
        return parse_string();
      case 't':
        return parse_literal("true");
      case 'f':
        return parse_literal("false");
      case 'n':
        return parse_literal("null");
      default:
        return parse_number();
      }
    }

    [[nodiscard]] bool parse_literal(std::string_view literal)
    {
      if (input_.substr(position_, literal.size()) != literal)
      {
        return false;
      }
      position_ += literal.size();
      return true;
    }

    [[nodiscard]] bool parse_object()
    {
      if (at_end() || current() != '{')
      {
        return false;
      }
      ++position_;
      skip_whitespace();

      if (!at_end() && current() == '}')
      {
        ++position_;
        return true;
      }

      while (true)
      {
        if (!parse_string())
        {
          return false;
        }

        skip_whitespace();
        if (at_end() || current() != ':')
        {
          return false;
        }
        ++position_;

        if (!parse_value())
        {
          return false;
        }

        skip_whitespace();
        if (!at_end() && current() == ',')
        {
          ++position_;
          skip_whitespace();
          continue;
        }
        if (!at_end() && current() == '}')
        {
          ++position_;
          return true;
        }
        return false;
      }
    }

    [[nodiscard]] bool parse_array()
    {
      if (at_end() || current() != '[')
      {
        return false;
      }
      ++position_;
      skip_whitespace();

      if (!at_end() && current() == ']')
      {
        ++position_;
        return true;
      }

      while (true)
      {
        if (!parse_value())
        {
          return false;
        }

        skip_whitespace();
        if (!at_end() && current() == ',')
        {
          ++position_;
          skip_whitespace();
          continue;
        }
        if (!at_end() && current() == ']')
        {
          ++position_;
          return true;
        }
        return false;
      }
    }

    [[nodiscard]] static bool is_hex_digit(char c)
    {
      return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    }

    [[nodiscard]] static bool is_decimal_digit(char c)
    {
      return std::isdigit(static_cast<unsigned char>(c)) != 0;
    }

    [[nodiscard]] bool parse_digits()
    {
      if (at_end() || !is_decimal_digit(current()))
      {
        return false;
      }

      while (!at_end() && is_decimal_digit(current()))
      {
        ++position_;
      }
      return true;
    }

    [[nodiscard]] bool parse_string()
    {
      if (at_end() || current() != '"')
      {
        return false;
      }

      ++position_;
      while (!at_end())
      {
        const char ch = current();
        ++position_;

        if (ch == '"')
        {
          return true;
        }

        if (ch == '\\')
        {
          if (at_end())
          {
            return false;
          }

          const char esc = current();
          ++position_;
          switch (esc)
          {
          case '"':
          case '\\':
          case '/':
          case 'b':
          case 'f':
          case 'n':
          case 'r':
          case 't':
            break;
          case 'u':
            for (int i = 0; i < 4; ++i)
            {
              if (at_end() || !is_hex_digit(current()))
              {
                return false;
              }
              ++position_;
            }
            break;
          default:
            return false;
          }
          continue;
        }

        if (static_cast<unsigned char>(ch) < 0x20)
        {
          return false;
        }
      }

      return false;
    }

    [[nodiscard]] bool parse_number()
    {
      if (at_end())
      {
        return false;
      }

      if (current() == '-')
      {
        ++position_;
        if (at_end())
        {
          return false;
        }
      }

      if (current() == '0')
      {
        ++position_;
        if (!at_end() && is_decimal_digit(current()))
        {
          return false;
        }
      }
      else if (!parse_digits())
      {
        return false;
      }

      if (!at_end() && current() == '.')
      {
        ++position_;
        if (!parse_digits())
        {
          return false;
        }
      }

      if (!at_end() && (current() == 'e' || current() == 'E'))
      {
        ++position_;
        if (!at_end() && (current() == '+' || current() == '-'))
        {
          ++position_;
        }
        if (!parse_digits())
        {
          return false;
        }
      }

      return true;
    }
  };

  [[nodiscard]] bool is_valid_json(std::string_view json_text)
  {
    return json_syntax_validator(json_text).validate();
  }
} // namespace

void test_primitives()
{
  std::cout << "Testing primitives...\n";

  // Test bool
  assert(json::to_json(true) == "true");
  assert(json::to_json(false) == "false");

  // Test integers
  assert(json::to_json(42) == "42");
  assert(json::to_json(-42) == "-42");
  assert(json::to_json(0) == "0");

  // Test floating point
  auto pi = json::to_json(3.14);
  assert(contains(pi, "3.14"));

  // Test that NaN and Inf throw
  bool threw_nan = false;
  try
  {
    json::to_json(std::numeric_limits<double>::quiet_NaN());
  }
  catch (const std::invalid_argument &)
  {
    threw_nan = true;
  }
  assert(threw_nan);

  bool threw_inf = false;
  try
  {
    json::to_json(std::numeric_limits<double>::infinity());
  }
  catch (const std::invalid_argument &)
  {
    threw_inf = true;
  }
  assert(threw_inf);

  std::cout << "  ✓ Primitives passed\n";
}

void test_strings()
{
  std::cout << "Testing strings...\n";

  // Simple string
  assert(json::to_json(std::string("hello")) == R"("hello")");

  // Plain ASCII regression (no escaping needed)
  assert(json::to_json(std::string("Simple ASCII 123")) == R"("Simple ASCII 123")");

  // String with escapes
  auto escaped = json::to_json(std::string("Hello \"World\"\nNext line"));
  assert(contains(escaped, "\\\""));
  assert(contains(escaped, "\\n"));

  // String with backslash
  auto backslash = json::to_json(std::string("path\\to\\file"));
  assert(contains(backslash, "\\\\"));

  // Mixed plain + escaped characters regression
  assert(json::to_json(std::string("A\tB C\\D\"E")) == R"("A\tB C\\D\"E")");

  // Control characters
  auto control = json::to_json(std::string("\x01\x02"));
  assert(contains(control, "\\u00"));

  const char *null_c_string = nullptr;
  assert(json::to_json(null_c_string) == "null");
  assert(json::to_json(nullptr) == "null");

  std::cout << "  ✓ Strings passed\n";
}

void test_containers()
{
  std::cout << "Testing containers...\n";

  // Vector
  std::vector<int> vec{ 1, 2, 3, 4, 5 };
  auto vec_json = json::to_json(vec);
  assert(contains(vec_json, "1"));
  assert(contains(vec_json, "5"));

  // List
  std::list<std::string> lst{ "apple", "banana", "cherry" };
  auto lst_json = json::to_json(lst);
  assert(contains(lst_json, "apple"));
  assert(contains(lst_json, "cherry"));

  // Deque
  std::deque<double> dq{ 1.1, 2.2, 3.3 };
  auto dq_json = json::to_json(dq);
  assert(contains(dq_json, "1.1"));

  // Array
  std::array<int, 3> arr{ 10, 20, 30 };
  auto arr_json = json::to_json(arr);
  assert(contains(arr_json, "10"));
  assert(contains(arr_json, "30"));
  const auto parsed_arr = json::from_json<std::array<int, 3>>(arr_json);
  assert(parsed_arr == arr);

  const auto malformed_arr = json::try_from_json<std::array<int, 3>>("[10,20]");
  assert(!malformed_arr.has_value());

  // Set
  std::set<int> s{ 5, 3, 1, 4, 2 };
  auto set_json = json::to_json(s);
  assert(contains(set_json, "["));

  // Unordered set
  std::unordered_set<std::string> us{ "one", "two", "three" };
  auto us_json = json::to_json(us);
  assert(contains(us_json, "one"));

  std::cout << "  ✓ Containers passed\n";
}

void test_forward_list()
{
  std::cout << "Testing forward_list...\n";

  std::forward_list<int> fl{ 1, 2, 3, 4 };
  const auto fl_json = json::to_json(fl);
  assert(fl_json == "[1,2,3,4]");

  const auto parsed = json::from_json<std::forward_list<int>>(fl_json);
  const std::vector<int> parsed_vec(parsed.begin(), parsed.end());
  assert((parsed_vec == std::vector<int>{ 1, 2, 3, 4 }));

  const auto malformed = json::try_from_json<std::forward_list<int>>("[1,2,]");
  assert(!malformed.has_value());

  std::cout << "  ✓ forward_list passed\n";
}

void test_bool_containers()
{
  std::cout << "Testing bool containers...\n";

  const std::vector<bool> bool_vector{ true, false, true, true, false };
  const auto bool_vector_json = json::to_json(bool_vector);
  assert(bool_vector_json == "[true,false,true,true,false]");

  const auto parsed_bool_vector = json::from_json<std::vector<bool>>("[false,true,false,true]");
  assert(parsed_bool_vector.size() == 4);
  assert(!parsed_bool_vector[0]);
  assert(parsed_bool_vector[1]);
  assert(!parsed_bool_vector[2]);
  assert(parsed_bool_vector[3]);

  std::bitset<8> bits{};
  bits.set(0);
  bits.set(3);
  bits.set(7);
  const auto bitset_json = json::to_json(bits);
  assert(bitset_json == "[true,false,false,true,false,false,false,true]");

  const auto parsed_bitset = json::from_json<std::bitset<8>>("[true,false,true,false,true,false,true,false]");
  assert(parsed_bitset.test(0));
  assert(!parsed_bitset.test(1));
  assert(parsed_bitset.test(2));
  assert(!parsed_bitset.test(3));
  assert(parsed_bitset.test(4));
  assert(!parsed_bitset.test(5));
  assert(parsed_bitset.test(6));
  assert(!parsed_bitset.test(7));

  const auto too_short = json::try_from_json<std::bitset<4>>("[true,false]");
  assert(!too_short.has_value());

  const auto too_long = json::try_from_json<std::bitset<4>>("[true,false,true,false,true]");
  assert(!too_long.has_value());

  std::cout << "  ✓ Bool containers passed\n";
}

void test_maps()
{
  std::cout << "Testing maps...\n";

  // Map
  std::map<std::string, int> m{ { "a", 1 }, { "b", 2 }, { "c", 3 } };
  auto m_json = json::to_json(m);
  assert(contains(m_json, "\"a\""));
  assert(contains(m_json, ":"));

  // Unordered map
  std::unordered_map<std::string, std::string> um{ { "key1", "value1" }, { "key2", "value2" } };
  auto um_json = json::to_json(um);
  assert(contains(um_json, "key1"));
  assert(contains(um_json, "value1"));

  std::map<int, std::string> int_map{ { 2, "two" }, { 1, "one" } };
  const auto int_map_json = json::to_json(int_map);
  assert(int_map_json == "[[1,\"one\"],[2,\"two\"]]");
  const auto parsed_int_map = json::from_json<std::map<int, std::string>>(int_map_json);
  assert(parsed_int_map == int_map);

  const auto malformed_int_map = json::try_from_json<std::map<int, std::string>>("[[1],[2,\"two\"]]");
  assert(!malformed_int_map.has_value());

  std::cout << "  ✓ Maps passed\n";
}

void test_container_adapters()
{
  std::cout << "Testing container adapters...\n";

  std::stack<int> st;
  st.push(1);
  st.push(2);
  const auto st_json = json::to_json(st);
  assert(st_json == "[2,1]");
  auto parsed_stack = json::from_json<std::stack<int>>(st_json);
  assert(!parsed_stack.empty() && parsed_stack.top() == 2);
  parsed_stack.pop();
  assert(!parsed_stack.empty() && parsed_stack.top() == 1);

  std::queue<std::string> q;
  q.push("a");
  q.push("b");
  const auto q_json = json::to_json(q);
  assert(q_json == R"(["a","b"])");
  auto parsed_queue = json::from_json<std::queue<std::string>>(q_json);
  assert(parsed_queue.front() == "a");
  parsed_queue.pop();
  assert(parsed_queue.front() == "b");

  std::priority_queue<int> pq;
  pq.push(3);
  pq.push(7);
  pq.push(1);
  const auto pq_json = json::to_json(pq);
  assert(pq_json == "[7,3,1]");
  auto parsed_pq = json::from_json<std::priority_queue<int>>(pq_json);
  assert(parsed_pq.top() == 7);
  parsed_pq.pop();
  assert(parsed_pq.top() == 3);
  parsed_pq.pop();
  assert(parsed_pq.top() == 1);

  std::cout << "  ✓ Container adapters passed\n";
}

void test_optional()
{
  std::cout << "Testing optional...\n";

  // Optional with value
  std::optional<int> opt1 = 42;
  assert(json::to_json(opt1) == "42");

  // Empty optional
  std::optional<int> opt2;
  assert(json::to_json(opt2) == "null");

  // Optional string
  std::optional<std::string> opt3 = "test";
  assert(json::to_json(opt3) == R"("test")");

  std::optional<std::string> opt4;
  assert(json::to_json(opt4) == "null");

  std::cout << "  ✓ Optional passed\n";
}

void test_variant()
{
  std::cout << "Testing variant...\n";

  // Variant with int
  std::variant<int, std::string, double> v1 = 42;
  auto v1_json = json::to_json(v1);
  assert(contains(v1_json, "index"));
  assert(contains(v1_json, "value"));
  assert(contains(v1_json, "42"));

  // Variant with string
  std::variant<int, std::string, double> v2 = std::string("hello");
  auto v2_json = json::to_json(v2);
  assert(contains(v2_json, "hello"));

  // Variant with double
  std::variant<int, std::string, double> v3 = 3.14;
  auto v3_json = json::to_json(v3);
  assert(contains(v3_json, "3.14"));

  std::cout << "  ✓ Variant passed\n";
}

void test_tuple()
{
  std::cout << "Testing tuple...\n";

  // Simple tuple
  std::tuple<int, std::string, bool> t1{ 42, "test", true };
  auto t1_json = json::to_json(t1);
  assert(contains(t1_json, "42"));
  assert(contains(t1_json, "test"));
  assert(contains(t1_json, "true"));

  // Pair
  std::pair<std::string, int> p{ "age", 30 };
  auto p_json = json::to_json(p);
  assert(contains(p_json, "age"));
  assert(contains(p_json, "30"));

  // Empty tuple-like
  std::tuple<> empty_tuple{};
  assert(json::to_json(empty_tuple) == "[]");
  const auto parsed_empty_tuple = json::from_json<std::tuple<>>("[]");
  ( void ) parsed_empty_tuple;

  std::array<int, 0> empty_array{};
  assert(json::to_json(empty_array) == "[]");
  const auto parsed_empty_array = json::from_json<std::array<int, 0>>("[]");
  assert(parsed_empty_array.empty());

  const auto invalid_empty_tuple = json::try_from_json<std::tuple<>>("[1]");
  assert(!invalid_empty_tuple.has_value());

  std::cout << "  ✓ Tuple passed\n";
}

void test_reflection()
{
  std::cout << "Testing reflection objects...\n";

  // Simple struct
  Person person{ .name = "Alice", .age = 30, .employed = true };
  auto person_json = json::to_json(person);
  assert(contains(person_json, "name"));
  assert(contains(person_json, "Alice"));
  assert(contains(person_json, "age"));
  assert(contains(person_json, "30"));
  assert(contains(person_json, "employed"));
  assert(contains(person_json, "true"));

  // Nested struct
  Employee emp{ .person = { .name = "Bob", .age = 35, .employed = true },
                .address = { .street = "123 Main St", .city = "Springfield", .zip_code = 12345 },
                .salary = 75000.50,
                .department = "Engineering" };
  auto emp_json = json::to_json(emp);
  assert(contains(emp_json, "Bob"));
  assert(contains(emp_json, "Springfield"));
  assert(contains(emp_json, "75000"));
  assert(contains(emp_json, "Engineering"));

  // Private members
  Account acc{ "ACC-001", 1234.56 };
  auto acc_json = json::to_json(acc);
  assert(contains(acc_json, "id_"));
  assert(contains(acc_json, "ACC-001"));
  assert(contains(acc_json, "balance_"));
  assert(contains(acc_json, "1234"));

  std::cout << "  ✓ Reflection objects passed\n";
}

void test_inheritance()
{
  std::cout << "Testing class inheritance...\n";

  DerivedFromAll obj{ 7, "protected-data", 3.5, true };
  auto obj_json = json::to_json(obj);
  assert(contains(obj_json, "public_value"));
  assert(contains(obj_json, "7"));
  assert(contains(obj_json, "protected_value"));
  assert(contains(obj_json, "protected-data"));
  assert(contains(obj_json, "private_value_"));
  assert(contains(obj_json, "3.5"));
  assert(contains(obj_json, "active_"));
  assert(contains(obj_json, "true"));

  std::cout << "  obj_json: " << obj_json << "\n";

  std::cout << "  ✓ Class inheritance passed\n";
}

void test_pretty_print()
{
  std::cout << "Testing pretty print...\n";

  json::options pretty_opts;
  pretty_opts.pretty = true;

  std::vector<int> vec{ 1, 2, 3 };
  auto vec_json = json::to_json(vec, pretty_opts);
  assert(contains(vec_json, "\n"));
  assert(vec_json.starts_with("[\n"));

  std::map<std::string, int> m{ { "a", 1 }, { "b", 2 } };
  auto m_json = json::to_json(m, pretty_opts);
  assert(contains(m_json, "\n"));
  assert(m_json.starts_with("{\n"));
  assert(contains(m_json, "\n  \"a\": 1"));

  Person person{ .name = "Charlie", .age = 25, .employed = false };
  auto person_json = json::to_json(person, pretty_opts);
  assert(contains(person_json, "\n"));
  assert(person_json.starts_with("{\n"));

  assert(json::to_json(std::vector<int>{}, pretty_opts) == "[]");

  std::cout << "  ✓ Pretty print passed\n";
}

void test_complex_nested()
{
  std::cout << "Testing complex nested structures...\n";

  // Complex nested structure
  std::map<std::string, std::vector<std::optional<int>>> complex{ { "first", { 1, 2, std::nullopt, 4 } },
                                                                  { "second", { std::nullopt, 6, 7 } } };
  auto complex_json = json::to_json(complex);
  assert(contains(complex_json, "null"));
  assert(contains(complex_json, "first"));
  assert(contains(complex_json, "second"));

  // Vector of structs
  std::vector<Person> people{ { .name = "Alice", .age = 30, .employed = true },
                              { .name = "Bob", .age = 35, .employed = false },
                              { .name = "Charlie", .age = 25, .employed = true } };
  auto people_json = json::to_json(people);
  assert(contains(people_json, "Alice"));
  assert(contains(people_json, "Bob"));
  assert(contains(people_json, "Charlie"));

  std::cout << "  ✓ Complex nested structures passed\n";
}

void test_generated_json_validity()
{
  std::cout << "Testing JSON syntax validity...\n";

  const Person person{ .name = "Dana", .age = 41, .employed = true };
  const Employee employee{ .person = { .name = "Eve", .age = 29, .employed = true },
                           .address = { .street = "42 Reflection Ave", .city = "Meta City", .zip_code = 42424 },
                           .salary = 123456.75,
                           .department = "Research" };
  const json::options pretty_opts{ .pretty = true, .indent_size = 2, .current_indent = 0 };

  assert(is_valid_json(json::to_json(true)));
  assert(is_valid_json(json::to_json(42)));
  assert(is_valid_json(json::to_json(-7)));
  assert(is_valid_json(json::to_json(3.14159)));
  assert(is_valid_json(json::to_json(std::string("plain text"))));
  assert(is_valid_json(json::to_json(std::string("line1\nline2\t\\quoted\""))));
  assert(is_valid_json(json::to_json(std::optional<int>{})));
  assert(is_valid_json(json::to_json(std::optional<int>{ 5 })));
  assert(is_valid_json(json::to_json(std::vector<int>{ 1, 2, 3 })));
  assert(is_valid_json(json::to_json(std::map<std::string, int>{ { "x", 1 }, { "y", 2 } })));
  assert(is_valid_json(json::to_json(std::tuple<int, std::string, bool>{ 1, "two", false })));
  assert(is_valid_json(json::to_json(std::variant<int, std::string>{ std::string("variant") })));
  assert(is_valid_json(json::to_json(person)));
  assert(is_valid_json(json::to_json(employee)));
  assert(is_valid_json(json::to_json(employee, pretty_opts)));

  std::cout << "  ✓ JSON syntax validity passed\n";
}

void test_json_validator_rejects_invalid()
{
  std::cout << "Testing invalid JSON rejection...\n";

  assert(!is_valid_json(""));
  assert(!is_valid_json("{"));
  assert(!is_valid_json("["));
  assert(!is_valid_json("{\"key\" 1}"));
  assert(!is_valid_json("{\"key\":}"));
  assert(!is_valid_json("{\"key\":1,}"));
  assert(!is_valid_json("[1,2,]"));
  assert(!is_valid_json("[1 2]"));
  assert(!is_valid_json("\"unterminated"));
  assert(!is_valid_json("\"bad\\xescape\""));
  assert(!is_valid_json("\"bad\\u12G4\""));
  assert(!is_valid_json("nul"));
  assert(!is_valid_json("tru"));
  assert(!is_valid_json("01"));
  assert(!is_valid_json("1."));
  assert(!is_valid_json("1e"));
  assert(!is_valid_json("1e+"));
  assert(!is_valid_json("{\"a\":1} trailing"));

  std::cout << "  ✓ Invalid JSON rejection passed\n";
}

void test_deserialization_primitives_and_containers()
{
  std::cout << "Testing deserialization primitives and containers...\n";

  assert(json::from_json<bool>("true"));
  assert(!json::from_json<bool>("false"));
  assert(json::from_json<int>("42") == 42);

  const auto pi = json::from_json<double>("3.14159");
  assert(std::abs(pi - 3.14159) < 0.000001);

  assert(json::from_json<std::string>(R"("hello")") == "hello");
  assert(json::from_json<std::string>(R"("\uD83D\uDE00")") == "\xF0\x9F\x98\x80");

  const auto maybe = json::from_json<std::optional<int>>("null");
  assert(!maybe.has_value());

  const auto vec = json::from_json<std::vector<int>>("[1,2,3,4]");
  assert(vec.size() == 4);
  assert(vec[0] == 1);
  assert(vec[3] == 4);

  const auto map = json::from_json<std::map<std::string, int>>(R"({"a":1,"b":2})");
  assert(map.at("a") == 1);
  assert(map.at("b") == 2);

  const auto tuple = json::from_json<std::tuple<int, std::string, bool>>(R"([7,"x",true])");
  assert(std::get<0>(tuple) == 7);
  assert(std::get<1>(tuple) == "x");
  assert(std::get<2>(tuple));

  std::cout << "  ✓ Deserialization primitives and containers passed\n";
}

void test_deserialization_reflection_and_variant()
{
  std::cout << "Testing deserialization reflection and variant...\n";

  const auto person = json::from_json<Person>(R"({"name":"Alice","age":30,"employed":true})");
  assert(person.name == "Alice");
  assert(person.age == 30);
  assert(person.employed);

  const auto person_with_unknown = json::from_json<Person>(R"({"name":"Bob","age":25,"extra":99})");
  assert(person_with_unknown.name == "Bob");
  assert(person_with_unknown.age == 25);
  assert(!person_with_unknown.employed);

  const auto person_missing = json::from_json<Person>(R"({"name":"Cara"})");
  assert(person_missing.name == "Cara");
  assert(person_missing.age == 0);
  assert(!person_missing.employed);

  std::variant<int, std::string, double> variant_value = 42;
  const auto variant_json = json::to_json(variant_value);
  const auto parsed_variant = json::from_json<std::variant<int, std::string, double>>(variant_json);
  assert(std::holds_alternative<int>(parsed_variant));
  assert(std::get<int>(parsed_variant) == 42);

  const auto parsed_variant_by_index = json::from_json<std::variant<int, std::string, double>>(
    R"({"index":1,"value":"hello"})");
  assert(std::holds_alternative<std::string>(parsed_variant_by_index));
  assert(std::get<std::string>(parsed_variant_by_index) == "hello");

  std::cout << "  ✓ Deserialization reflection and variant passed\n";
}

void test_deserialization_errors()
{
  std::cout << "Testing deserialization errors...\n";

  bool threw_invalid = false;
  try
  {
    static_cast<void>(json::from_json<int>("\"not-an-int\""));
  }
  catch (const std::invalid_argument &)
  {
    threw_invalid = true;
  }
  assert(threw_invalid);

  const auto non_throwing_error = json::try_from_json<int>("1.5");
  assert(!non_throwing_error.has_value());

  const auto trailing = json::try_from_json<int>("1 trailing");
  assert(!trailing.has_value());

  const std::string invalid_variant_json = R"({"index":99,"value":1})";
  const auto invalid_variant = json::try_from_json<std::variant<int, std::string>>(invalid_variant_json);
  assert(!invalid_variant.has_value());
  assert(invalid_variant.error().position == invalid_variant_json.find("99"));

  const std::string invalid_legacy_variant_json = R"({"type":"unknown","value":1})";
  const auto invalid_legacy_variant = json::try_from_json<std::variant<int, std::string>>(invalid_legacy_variant_json);
  assert(!invalid_legacy_variant.has_value());
  assert(invalid_legacy_variant.error().position == invalid_legacy_variant_json.find("\"unknown\""));

  const auto missing_low_surrogate = json::try_from_json<std::string>(R"("\uD83D")");
  assert(!missing_low_surrogate.has_value());

  const auto invalid_low_surrogate = json::try_from_json<std::string>(R"("\uD83D\u0041")");
  assert(!invalid_low_surrogate.has_value());

  const auto unexpected_low_surrogate = json::try_from_json<std::string>(R"("\uDE00")");
  assert(!unexpected_low_surrogate.has_value());

  const std::string bad_member_json = R"({"id":7,"active":true,"name":123})";
  const auto bad_member = json::try_from_json<UserRecord>(bad_member_json);
  assert(!bad_member.has_value());
  assert(bad_member.error().position == bad_member_json.find("123"));

  std::cout << "  ✓ Deserialization errors passed\n";
}

void test_deserialization_inheritance()
{
  std::cout << "Testing deserialization inheritance...\n";

  const auto derived = json::try_from_json<DerivedFromAll>(
    R"({"public_value":7,"protected_value":"prot","private_value_":3.5,"active_":true})");
  assert(!derived.has_value());
  assert(contains(derived.error().message, "default-constructible"));

  const DerivedFromAll derived_instance{ 7, "prot", 3.5, true };
  const auto derived_json = json::to_json(derived_instance);
  assert(contains(derived_json, "\"public_value\":7"));
  assert(contains(derived_json, "\"protected_value\":\"prot\""));
  assert(contains(derived_json, "\"private_value_\":3.5"));
  assert(contains(derived_json, "\"active_\":true"));

  const auto full = json::from_json<UserRecord>(R"({"id":7,"active":true,"name":"Alice"})");
  assert(full.id == 7);
  assert(full.active);
  assert(full.name == "Alice");

  // Unknown key is ignored, missing key keeps default value
  const auto partial = json::from_json<UserRecord>(R"({"id":9,"name":"Bob","extra":123})");
  assert(partial.id == 9);
  assert(!partial.active);
  assert(partial.name == "Bob");

  const auto missing_required = json::try_from_json<DerivedFromAll>(
    R"({"public_value":7,"protected_value":"prot","active_":true})");
  assert(!missing_required.has_value());
  assert(contains(missing_required.error().message, "default-constructible"));

  std::cout << "  ✓ Deserialization inheritance passed\n";
}

void test_pointer_serialization()
{
  std::cout << "Testing pointer and smart-pointer serialization...\n";

  int x = 5;
  int *p = &x;
  assert(json::to_json(p) == "5");

  int *nullp = nullptr;
  assert(json::to_json(nullp) == "null");

  std::unique_ptr<int> up = std::make_unique<int>(42);
  assert(json::to_json(up) == "42");
  std::unique_ptr<int> upnull;
  assert(json::to_json(upnull) == "null");

  std::shared_ptr<std::string> sp = std::make_shared<std::string>("hello");
  assert(json::to_json(sp) == R"("hello")");
  std::shared_ptr<int> spnull;
  assert(json::to_json(spnull) == "null");

  // Deserialization: unique_ptr
  auto up_parsed = json::from_json<std::unique_ptr<int>>("42");
  assert(up_parsed && *up_parsed == 42);
  auto up_null_parsed = json::from_json<std::unique_ptr<int>>("null");
  assert(!up_null_parsed);

  // Raw pointer deserialization: returns owning raw pointer (caller responsible)
  auto *raw_p = json::from_json<int *>("7");
  assert(raw_p != nullptr && *raw_p == 7);
  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory) - this intentional for testing purposes
  delete raw_p;

  auto *raw_null = json::from_json<int *>("null");
  assert(raw_null == nullptr);

  std::optional<int *> opt_ptr{ p };
  assert(json::to_json(opt_ptr) == "5");
  std::optional<int *> opt_null{ nullptr };
  assert(json::to_json(opt_null) == "null");

  std::tuple<int *, std::unique_ptr<int>> ptr_tuple{ p, std::make_unique<int>(8) };
  assert(json::to_json(ptr_tuple) == "[5,8]");

  std::vector<std::unique_ptr<int>> ptr_vec;
  ptr_vec.push_back(std::make_unique<int>(1));
  ptr_vec.push_back(nullptr);
  ptr_vec.push_back(std::make_unique<int>(3));
  assert(json::to_json(ptr_vec) == "[1,null,3]");

  const auto parsed_vec = json::from_json<std::vector<std::unique_ptr<int>>>("[4,null,6]");
  assert(parsed_vec.size() == 3);
  assert(parsed_vec[0] && *parsed_vec[0] == 4);
  assert(!parsed_vec[1]);
  assert(parsed_vec[2] && *parsed_vec[2] == 6);

  const auto parsed_tuple = json::from_json<std::tuple<std::unique_ptr<int>, std::shared_ptr<int>>>("[9,10]");
  assert(std::get<0>(parsed_tuple) && *std::get<0>(parsed_tuple) == 9);
  assert(std::get<1>(parsed_tuple) && *std::get<1>(parsed_tuple) == 10);

  std::cout << "  ✓ Pointer and smart-pointer serialization passed\n";
}

int main()
{
  std::cout << "Running JSON serialization tests...\n\n";

  try
  {
    test_primitives();
    test_strings();
    test_containers();
    test_forward_list();
    test_bool_containers();
    test_maps();
    test_container_adapters();
    test_optional();
    test_variant();
    test_tuple();
    test_reflection();
    test_inheritance();
    test_pretty_print();
    test_complex_nested();
    test_generated_json_validity();
    test_json_validator_rejects_invalid();
    test_deserialization_primitives_and_containers();
    test_deserialization_reflection_and_variant();
    test_deserialization_errors();
    test_deserialization_inheritance();
    test_pointer_serialization();

    std::cout << "\n✅ All tests passed!\n";
    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "\n❌ Test failed with exception: " << e.what() << "\n";
    return 1;
  }
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
