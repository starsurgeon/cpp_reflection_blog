#if __has_include(<fuzztest/fuzztest.h>)
  #include "json.h"
  #include <fuzztest/fuzztest.h>
  #include <string>
  #define CRB_HAS_FUZZTEST 1
#else
  #define CRB_HAS_FUZZTEST 0
#endif

namespace
{

#if CRB_HAS_FUZZTEST

void FuzzTryFromJsonNeverThrows(const std::string &input)
{
  [[maybe_unused]] auto parsed_int = json::try_from_json<int>(input);
  [[maybe_unused]] auto parsed_string = json::try_from_json<std::string>(input);
}

void FuzzRoundTripIntDoesNotThrow(int value)
{
  auto text = json::to_json(value);
  [[maybe_unused]] auto parsed_int = json::try_from_json<int>(text);
}

FUZZ_TEST(JsonFuzzSuite, FuzzTryFromJsonNeverThrows);
FUZZ_TEST(JsonFuzzSuite, FuzzRoundTripIntDoesNotThrow);

#endif

} // namespace
