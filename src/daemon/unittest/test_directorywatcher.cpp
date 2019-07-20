#include <catch.hpp>

#include "dummyfilesystem.h"
#include "daemon/directorywatcher.h"
#include "filewatch.pb.h"

TEST_CASE("fill root dir directory list", "[DirectoryWatcher]")
{
  DummyFileSystem fs;
  fw::dm::DirectoryWatcher dw("/", fs);
  filewatch::DirList response;

  SECTION("no directories and no files in path")
  {
    CHECK(fw::dm::status_code::OK == dw.fill_dir_list(response));
    CHECK(response.dirnames_size() == 0);
  }

  SECTION("one directory in path")
  {
    fs.add_dir("/", "foo", 345645);
    CHECK(fw::dm::status_code::OK == dw.fill_dir_list(response));
    REQUIRE(response.dirnames_size() == 1);
    CHECK(response.dirnames(0).name() == "foo");
    CHECK(response.dirnames(0).modification_time().epoch() == 345645);
  }

  SECTION("one directory and one file in path")
  {
    fs.add_file("/", "foo", 576843);
    fs.add_dir("/", "bar", 829832);
    CHECK(fw::dm::status_code::OK == dw.fill_dir_list(response));
    REQUIRE(response.dirnames_size() == 1);
    CHECK(response.dirnames(0).name() == "bar");
    CHECK(response.dirnames(0).modification_time().epoch() == 829832);
  }
}

TEST_CASE("fill subdir directory list", "[DirectoryWatcher]")
{
  DummyFileSystem fs;
  fw::dm::DirectoryWatcher dw("/subdir", fs);
  filewatch::DirList response;

  SECTION("Directory not found")
  {
    CHECK(fw::dm::status_code::NOT_FOUND == dw.fill_dir_list(response));
    CHECK(response.dirnames_size() == 0);
  }

  SECTION("Sought directory is a file")
  {
    fs.add_file("/", "subdir", 3456);
    CHECK(fw::dm::status_code::NOT_FOUND == dw.fill_dir_list(response));
    CHECK(response.dirnames_size() == 0);
  }

  SECTION("Directory exists")
  {
    fs.add_dir("/", "subdir", 8234822);
    CHECK(fw::dm::status_code::OK == dw.fill_dir_list(response));
    CHECK(response.dirnames_size() == 0);
  }

  SECTION("subdir contains two directories")
  {
    fs.add_dir("/", "subdir", 4632);
    fs.add_dir("/subdir", "subsub0", 345678);
    fs.add_dir("/subdir", "subsub1", 983982);
    fs.add_file("/subdir", "subsub2", 38028);
    CHECK(fw::dm::status_code::OK == dw.fill_dir_list(response));
    REQUIRE(response.dirnames_size() == 2);
    CHECK(response.dirnames(0).name() == "subsub0");
    CHECK(response.dirnames(0).modification_time().epoch() == 345678);
    CHECK(response.dirnames(1).name() == "subsub1");
    CHECK(response.dirnames(1).modification_time().epoch() == 983982);
  }
}

TEST_CASE("fill directory list sets dirname", "[DirectoryWatcher]")
{
  DummyFileSystem fs(123456);
  fs.add_dir("/", "subdir", 27389272);
  fs.add_file("/subdir", "filename", 17283);

  filewatch::DirList response;

  SECTION("root dir")
  {
    fw::dm::DirectoryWatcher dw("/", fs);
    CHECK(fw::dm::status_code::OK == dw.fill_dir_list(response));
    CHECK("/" == response.name().name());
    CHECK(response.name().modification_time().epoch() == 123456);
  }

  SECTION("subdir")
  {
    fw::dm::DirectoryWatcher dw("/subdir", fs);
    CHECK(fw::dm::status_code::OK == dw.fill_dir_list(response));
    CHECK("/subdir" == response.name().name());
    CHECK(response.name().modification_time().epoch() == 27389272);
  }

  SECTION("dir not found")
  {
    fw::dm::DirectoryWatcher dw("/subdir/not_existing", fs);
    CHECK(fw::dm::status_code::NOT_FOUND == dw.fill_dir_list(response));
    CHECK("/subdir/not_existing" == response.name().name());
    // CHECK(response.name().modification_time().epoch() == UNDEFINED);
  }
}



TEST_CASE("fill root dir file list", "[DirectoryWatcher]")
{
  DummyFileSystem fs;
  filewatch::FileList response;
  fw::dm::DirectoryWatcher dw("/", fs);

  SECTION("empty root dir")
  {
    CHECK(fw::dm::status_code::OK == dw.fill_file_list(response));
    CHECK(response.filenames().size() == 0);
  }

  SECTION("one file in root dir")
  {
    fs.add_file("/", "my file.txt", 24356);
    CHECK(fw::dm::status_code::OK == dw.fill_file_list(response));
    REQUIRE(response.filenames().size() == 1);
    CHECK(response.filenames(0).name() == "my file.txt");
    CHECK(response.filenames(0).modification_time().epoch() == 24356);
  }

  SECTION("one file and one directory in root dir")
  {
    fs.add_file("/", "some-file.cpp", 683902893);
    fs.add_dir("/", "some-dir", 684903847);
    CHECK(fw::dm::status_code::OK == dw.fill_file_list(response));
    REQUIRE(response.filenames().size() == 1);
    CHECK(response.filenames(0).name() == "some-file.cpp");
    CHECK(response.filenames(0).modification_time().epoch() == 683902893);
  }
}

TEST_CASE("fill subdir file list", "[DirectoryWatcher]")
{
  DummyFileSystem fs;
  fw::dm::DirectoryWatcher dw("/subdir", fs);
  filewatch::FileList response;

  SECTION("Directory not found")
  {
    CHECK(fw::dm::status_code::NOT_FOUND == dw.fill_file_list(response));
    CHECK(response.filenames_size() == 0);
  }

  SECTION("Sought directory is a file")
  {
    fs.add_file("/", "subdir", 3456);
    CHECK(fw::dm::status_code::NOT_FOUND == dw.fill_file_list(response));
    CHECK(response.filenames_size() == 0);
  }

  SECTION("Directory exists but is empty")
  {
    fs.add_dir("/", "subdir", 8234822);
    CHECK(fw::dm::status_code::OK == dw.fill_file_list(response));
    CHECK(response.filenames_size() == 0);
  }

  SECTION("subdir contains two files")
  {
    fs.add_dir("/", "subdir", 4632);
    fs.add_file("/subdir", "file0.h", 983982);
    fs.add_file("/subdir", "file0.cpp", 345678);
    fs.add_dir("/subdir", "subsub0", 38028);
    CHECK(fw::dm::status_code::OK == dw.fill_file_list(response));
    REQUIRE(response.filenames_size() == 2);
    CHECK(response.filenames(0).name() == "file0.cpp");
    CHECK(response.filenames(0).modification_time().epoch() == 345678);
    CHECK(response.filenames(1).name() == "file0.h");
    CHECK(response.filenames(1).modification_time().epoch() == 983982);
  }
}

TEST_CASE("fill file list sets dirname", "[DirectoryWatcher]")
{
  DummyFileSystem fs(382932123);
  fs.add_dir("/", "subdir", 27389272);
  fs.add_file("/subdir", "filename", 17283);

  filewatch::FileList response;

  SECTION("root dir")
  {
    fw::dm::DirectoryWatcher dw("/", fs);
    CHECK(fw::dm::status_code::OK == dw.fill_file_list(response));
    CHECK("/" == response.name().name());
    CHECK(response.name().modification_time().epoch() == 382932123);
  }

  SECTION("subdir")
  {
    fw::dm::DirectoryWatcher dw("/subdir", fs);
    CHECK(fw::dm::status_code::OK == dw.fill_file_list(response));
    CHECK("/subdir" == response.name().name());
    CHECK(response.name().modification_time().epoch() == 27389272);
  }

  SECTION("dir not found")
  {
    fw::dm::DirectoryWatcher dw("/subdir/not_existing", fs);
    CHECK(fw::dm::status_code::NOT_FOUND == dw.fill_file_list(response));
    CHECK("/subdir/not_existing" == response.name().name());
    // CHECK(response.name().modification_time().epoch() == UNDEFINED);
  }
}
