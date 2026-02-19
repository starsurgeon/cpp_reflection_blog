#include <array>
#include <bitset>
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <span>
#include <stack>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "json.h"

// disable clang-tidy warnings for magic numbers in test cases
// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)

static_assert(json::range_like<std::vector<int>>);
static_assert(json::deserializable_range_like<std::vector<int>>);

static_assert(json::range_like<std::list<int>>);
static_assert(json::deserializable_range_like<std::list<int>>);

static_assert(json::range_like<std::deque<int>>);
static_assert(json::deserializable_range_like<std::deque<int>>);

static_assert(json::range_like<std::set<int>>);
static_assert(json::deserializable_range_like<std::set<int>>);

static_assert(json::range_like<std::unordered_set<int>>);
static_assert(json::deserializable_range_like<std::unordered_set<int>>);

static_assert(json::range_like<std::vector<bool>>);
static_assert(json::deserializable_range_like<std::vector<bool>>);

static_assert(json::bitset_like<std::bitset<8>>);

static_assert(json::tuple_like<std::array<int, 4>>);
static_assert(!json::deserializable_range_like<std::array<int, 4>>);

static_assert(json::range_like<std::forward_list<int>>);
static_assert(json::deserializable_range_like<std::forward_list<int>>);

static_assert(json::range_like<std::span<int>>);
static_assert(!json::deserializable_range_like<std::span<int>>);

static_assert(json::string_key_map_like<std::map<std::string, int>>);
static_assert(json::string_key_map_like<std::unordered_map<std::string, int>>);

static_assert(json::non_string_map_like<std::map<int, int>>);
static_assert(json::non_string_map_like<std::unordered_map<int, int>>);

static_assert(json::tuple_like<std::tuple<int, std::string>>);
static_assert(json::tuple_like<std::pair<int, std::string>>);
static_assert(json::tuple_like<std::tuple<>>);
static_assert(json::tuple_like<std::array<int, 0>>);

static_assert(!json::range_like<std::stack<int>>);
static_assert(!json::range_like<std::queue<int>>);
static_assert(json::stack_like<std::stack<int>>);
static_assert(json::queue_like<std::queue<int>>);
static_assert(json::priority_queue_like<std::priority_queue<int>>);

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)

int main()
{
  return 0;
}
