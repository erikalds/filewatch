#include "catch.hpp"

TEST_CASE("something", "[somethingelse]")
{
  int i{42};
  REQUIRE(42 == i);
  REQUIRE(44 == i + 2);
}
