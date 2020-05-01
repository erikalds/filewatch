#include <catch2/catch.hpp>

#include "dummyfilesystem.h"

TEST_CASE("join", "[filesystem]")
{
  DummyFileSystem fs;
  SECTION("normal join")
  {
    CHECK(fs.join("/foo", "bar") == "/foo/bar");
  }
  SECTION("containing dir ends with /")
  {
    CHECK(fs.join("/foo/", "bar") == "/foo/bar");
  }
  SECTION("containing dir is /")
  {
    CHECK(fs.join("/", "foobar") == "/foobar");
  }
}

TEST_CASE("parent", "[filesystem]")
{
  DummyFileSystem fs;
  SECTION("sub-sub-sub-directory")
  {
    CHECK(fs.parent("/foo/bar/baz/asdf") == "/foo/bar/baz");
  }
  SECTION("sub-sub-directory")
  {
    CHECK(fs.parent("/foo/bar/baz") == "/foo/bar");
  }
  SECTION("sub-directory")
  {
    CHECK(fs.parent("/foo/bar") == "/foo");
  }
  SECTION("directory")
  {
    CHECK(fs.parent("/foo") == "/");
  }
  SECTION("directory with trailing slash")
  {
    CHECK(fs.parent("/foo/") == "/");
  }
  SECTION("root")
  {
    CHECK(fs.parent("/") == "/");
  }
}

TEST_CASE("leaf", "[filesystem]")
{
  DummyFileSystem fs;
  SECTION("sub-sub-directory")
  {
    CHECK(fs.leaf("/foo/bar/baz") == "baz");
  }
  SECTION("slash-at-end sub-directory")
  {
    CHECK(fs.leaf("/foo/bar/") == "bar");
  }
  SECTION("directory")
  {
    CHECK(fs.leaf("/foo") == "foo");
  }
  SECTION("root")
  {
    CHECK(fs.leaf("/") == "/");
  }
}
