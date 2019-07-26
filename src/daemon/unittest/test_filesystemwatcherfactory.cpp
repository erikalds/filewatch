#include <catch.hpp>

#include "dummyfilesystem.h"
#include "daemon/directoryview.h"
#include "daemon/filesystemwatcherfactory.h"

TEST_CASE("create_directory", "[filesystemwatcherfactory]")
{
  auto fs = std::make_unique<DummyFileSystem>();
  fw::dm::FileSystemWatcherFactory f(std::move(fs));
  fw::dm::FileSystemFactory& factory = f;
  SECTION("creates valid ptr")
  {
    CHECK(factory.create_directory("/subdir") != nullptr);
  }
}