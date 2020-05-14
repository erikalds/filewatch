#include "daemon/directoryview.h"
#include "daemon/filesystemwatcherfactory.h"
#include "daemon/fileview.h"
#include "dummyfilesystem.h"

#include <catch2/catch.hpp>

TEST_CASE("create_directory", "[filesystemwatcherfactory]")
{
  auto fs = std::make_unique<DummyFileSystem>("rootdir");
  fw::dm::FileSystemWatcherFactory f(std::move(fs));
  fw::dm::FileSystemFactory& factory = f;
  SECTION("creates valid ptr")
  {
    CHECK(factory.create_directory("/subdir") != nullptr);
  }
}

TEST_CASE("create_file", "[filesystemwatcherfactory]")
{
  auto fs = std::make_unique<DummyFileSystem>("rootdir");
  fw::dm::FileSystemWatcherFactory f(std::move(fs));
  fw::dm::FileSystemFactory& factory = f;
  SECTION("creates valid ptr")
  {
    CHECK(factory.create_file("/subdir", "filename.txt") != nullptr);
  }
}
