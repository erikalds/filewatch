#include "daemon/filewatcher.h"
#include "dummyfilesystem.h"
#include "filewatch.pb.h"

#include <catch2/catch.hpp>
#include <grpcpp/impl/codegen/status.h>

TEST_CASE("file contents", "[FileWatcher]")
{
  DummyFileSystem fs("rootdir", 123456);
  fs.add_file("/", "filename.txt", 12345678);
  fs.write_file("/filename.txt", "some contents");
  fw::dm::FileWatcher filewatcher("/", "filename.txt", fs);
  filewatch::FileContent response;
  CHECK(filewatcher.fill_contents(response).ok());

  SECTION("fills_dirname")
  {
    CHECK(response.dirname().name() == "/");
    CHECK(response.dirname().modification_time().epoch() == 123456);
  }

  SECTION("fills_filename")
  {
    CHECK(response.filename().name() == "filename.txt");
    CHECK(response.filename().modification_time().epoch() == 12345678);
  }

  SECTION("fills_lines")
  {
    REQUIRE(response.lines_size() == 1);
    CHECK(response.lines(0) == "some contents");
  }
}

TEST_CASE("file contents in a subdirectory", "[FileWatcher]")
{
  DummyFileSystem fs("rootdir", 123456);
  fs.add_dir("/", "subdir", 1234567);
  fs.add_file("/subdir", "otherfilename.txt", 87654321);
  fs.write_file("/subdir/otherfilename.txt", "some other contents");
  fw::dm::FileWatcher filewatcher("/subdir", "otherfilename.txt", fs);
  filewatch::FileContent response;
  CHECK(filewatcher.fill_contents(response).ok());

  SECTION("fills_dirname")
  {
    CHECK(response.dirname().name() == "/subdir");
    CHECK(response.dirname().modification_time().epoch() == 1234567);
  }

  SECTION("fills_filename")
  {
    CHECK(response.filename().name() == "otherfilename.txt");
    CHECK(response.filename().modification_time().epoch() == 87654321);
  }

  SECTION("fills_lines")
  {
    REQUIRE(response.lines_size() == 1);
    CHECK(response.lines(0) == "some other contents");
  }
}

TEST_CASE("file multiline contents", "[FileWatcher]")
{
  DummyFileSystem fs("rootdir", 123456);
  fs.add_dir("/", "subdir", 1234567);
  fs.add_file("/subdir", "otherfilename.txt", 87654321);
  fs.write_file("/subdir/otherfilename.txt",
                "line no 1\nline no 2\nline no 3\n");
  fw::dm::FileWatcher filewatcher("/subdir", "otherfilename.txt", fs);
  filewatch::FileContent response;
  CHECK(filewatcher.fill_contents(response).ok());
  CHECK(response.lines_size() == 4);
  for (int i = 0; i < response.lines_size() - 1; ++i)
  {
    CHECK(response.lines(i) == fmt::format("line no {}", i + 1));
  }

  CHECK(response.lines(response.lines_size() - 1).empty());
}

TEST_CASE("contents on non existing file", "[FileWatcher]")
{
  DummyFileSystem fs("rootdir", 123456);

  filewatch::FileContent response;
  SECTION("non existing file")
  {
    fw::dm::FileWatcher filewatcher("/", "filename.txt", fs);
    auto status = filewatcher.fill_contents(response);
    CHECK(!status.ok());
    CHECK(status.error_code() == grpc::NOT_FOUND);
    CHECK(status.error_message() == "'/filename.txt' does not exist");
  }

  SECTION("non existing subdir")
  {
    fw::dm::FileWatcher filewatcher("/subdir", "filename.txt", fs);
    auto status = filewatcher.fill_contents(response);
    CHECK(!status.ok());
    CHECK(status.error_code() == grpc::NOT_FOUND);
    CHECK(status.error_message() == "'/subdir/filename.txt' does not exist");
  }

  SECTION("name is a directory")
  {
    fs.add_dir("/", "dir", 2345678);
    fs.add_dir("/dir", "anotherdir", 2345682);
    fw::dm::FileWatcher filewatcher("/dir", "anotherdir", fs);
    auto status = filewatcher.fill_contents(response);
    CHECK(!status.ok());
    CHECK(status.error_code() == grpc::FAILED_PRECONDITION);
    CHECK(status.error_message() == "'/dir/anotherdir' is not a file");
  }
}
