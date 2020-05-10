#include "dummyfilesystem.h"

#include <catch2/catch.hpp>

TEST_CASE("join", "[filesystem]")
{
  DummyFileSystem fs("rootdir");
  SECTION("normal join") { CHECK(fs.join("/foo", "bar") == "/foo/bar"); }
  SECTION("containing dir ends with /")
  {
    CHECK(fs.join("/foo/", "bar") == "/foo/bar");
  }
  SECTION("containing dir is /") { CHECK(fs.join("/", "foobar") == "/foobar"); }
  SECTION("containing dir and filename has /")
  {
    CHECK(fs.join("/foo/", "/bar") == "/foo/bar");
  }
}

TEST_CASE("parent", "[filesystem]")
{
  DummyFileSystem fs("rootdir");
  SECTION("sub-sub-sub-directory")
  {
    CHECK(fs.parent("/foo/bar/baz/asdf") == "/foo/bar/baz");
  }
  SECTION("sub-sub-directory")
  {
    CHECK(fs.parent("/foo/bar/baz") == "/foo/bar");
  }
  SECTION("sub-directory") { CHECK(fs.parent("/foo/bar") == "/foo"); }
  SECTION("directory") { CHECK(fs.parent("/foo") == "/"); }
  SECTION("directory with trailing slash") { CHECK(fs.parent("/foo/") == "/"); }
  SECTION("root") { CHECK(fs.parent("/") == "/"); }
}

TEST_CASE("leaf", "[filesystem]")
{
  DummyFileSystem fs("rootdir");
  SECTION("sub-sub-directory") { CHECK(fs.leaf("/foo/bar/baz") == "baz"); }
  SECTION("slash-at-end sub-directory")
  {
    CHECK(fs.leaf("/foo/bar/") == "bar");
  }
  SECTION("directory") { CHECK(fs.leaf("/foo") == "foo"); }
  SECTION("root") { CHECK(fs.leaf("/") == "/"); }
}
