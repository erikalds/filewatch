#include "daemon/directoryeventlistener.h"
#include "daemon/directorywatcher.h"
#include "dummyfilesystem.h"
#include "filewatch.pb.h"

#include <catch2/catch.hpp>
#include <grpcpp/impl/codegen/status.h>

TEST_CASE("fill root dir directory list", "[DirectoryWatcher]")
{
  DummyFileSystem fs("rootdir");
  fw::dm::DirectoryWatcher dw("/", fs);
  filewatch::DirList response;

  SECTION("no directories and no files in path")
  {
    CHECK(dw.fill_dir_list(response).ok());
    CHECK(response.dirnames_size() == 0);
  }

  SECTION("one directory in path")
  {
    fs.add_dir("/", "foo", 345645);
    CHECK(dw.fill_dir_list(response).ok());
    REQUIRE(response.dirnames_size() == 1);
    CHECK(response.dirnames(0).name() == "foo");
    CHECK(response.dirnames(0).modification_time().epoch() == 345645);
  }

  SECTION("one directory and one file in path")
  {
    fs.add_file("/", "foo", 576843);
    fs.add_dir("/", "bar", 829832);
    CHECK(dw.fill_dir_list(response).ok());
    REQUIRE(response.dirnames_size() == 1);
    CHECK(response.dirnames(0).name() == "bar");
    CHECK(response.dirnames(0).modification_time().epoch() == 829832);
  }
}

TEST_CASE("fill subdir directory list", "[DirectoryWatcher]")
{
  DummyFileSystem fs("rootdir");
  fw::dm::DirectoryWatcher dw("/subdir", fs);
  filewatch::DirList response;

  SECTION("Directory not found")
  {
    auto status = dw.fill_dir_list(response);
    CHECK(grpc::NOT_FOUND == status.error_code());
    CHECK("'/subdir' does not exist" == status.error_message());
    CHECK(response.dirnames_size() == 0);
  }

  SECTION("Sought directory is a file")
  {
    fs.add_file("/", "subdir", 3456);
    auto status = dw.fill_dir_list(response);
    CHECK(grpc::NOT_FOUND == status.error_code());
    CHECK("'/subdir' is not a directory" == status.error_message());
    CHECK(response.dirnames_size() == 0);
  }

  SECTION("Directory exists")
  {
    fs.add_dir("/", "subdir", 8234822);
    CHECK(dw.fill_dir_list(response).ok());
    CHECK(response.dirnames_size() == 0);
  }

  SECTION("subdir contains two directories")
  {
    fs.add_dir("/", "subdir", 4632);
    fs.add_dir("/subdir", "subsub0", 345678);
    fs.add_dir("/subdir", "subsub1", 983982);
    fs.add_file("/subdir", "subsub2", 38028);
    CHECK(dw.fill_dir_list(response).ok());
    REQUIRE(response.dirnames_size() == 2);
    CHECK(response.dirnames(0).name() == "subsub0");
    CHECK(response.dirnames(0).modification_time().epoch() == 345678);
    CHECK(response.dirnames(1).name() == "subsub1");
    CHECK(response.dirnames(1).modification_time().epoch() == 983982);
  }
}

TEST_CASE("fill directory list sets dirname", "[DirectoryWatcher]")
{
  DummyFileSystem fs("rootdir", 123456);
  fs.add_dir("/", "subdir", 27389272);
  fs.add_file("/subdir", "filename", 17283);

  filewatch::DirList response;

  SECTION("root dir")
  {
    fw::dm::DirectoryWatcher dw("/", fs);
    CHECK(dw.fill_dir_list(response).ok());
    CHECK("/" == response.name().name());
    CHECK(response.name().modification_time().epoch() == 123456);
  }

  SECTION("subdir")
  {
    fw::dm::DirectoryWatcher dw("/subdir", fs);
    CHECK(dw.fill_dir_list(response).ok());
    CHECK("/subdir" == response.name().name());
    CHECK(response.name().modification_time().epoch() == 27389272);
  }

  SECTION("dir not found")
  {
    fw::dm::DirectoryWatcher dw("/subdir/not_existing", fs);
    auto status = dw.fill_dir_list(response);
    CHECK(grpc::NOT_FOUND == status.error_code());
    CHECK("'/subdir/not_existing' does not exist" == status.error_message());
    CHECK("/subdir/not_existing" == response.name().name());
    // CHECK(response.name().modification_time().epoch() == UNDEFINED);
  }
}



TEST_CASE("fill root dir file list", "[DirectoryWatcher]")
{
  DummyFileSystem fs("rootdir", 564321);
  filewatch::FileList response;
  fw::dm::DirectoryWatcher dw("/", fs);

  SECTION("empty root dir")
  {
    CHECK(dw.fill_file_list(response).ok());
    CHECK(response.filenames().empty());
  }

  SECTION("one file in root dir")
  {
    fs.add_file("/", "my file.txt", 24356);
    CHECK(dw.fill_file_list(response).ok());
    REQUIRE(response.filenames().size() == 1);
    CHECK(response.filenames(0).name() == "my file.txt");
    CHECK(response.filenames(0).modification_time().epoch() == 24356);
    CHECK(response.filenames(0).dirname().name() == "/");
    CHECK(response.filenames(0).dirname().modification_time().epoch()
          == 564321);
  }

  SECTION("one file and one directory in root dir")
  {
    fs.add_file("/", "some-file.cpp", 683902893);
    fs.add_dir("/", "some-dir", 684903847);
    CHECK(dw.fill_file_list(response).ok());
    REQUIRE(response.filenames().size() == 1);
    CHECK(response.filenames(0).name() == "some-file.cpp");
    CHECK(response.filenames(0).modification_time().epoch() == 683902893);
    CHECK(response.filenames(0).dirname().name() == "/");
    CHECK(response.filenames(0).dirname().modification_time().epoch()
          == 564321);
  }
}

TEST_CASE("fill subdir file list", "[DirectoryWatcher]")
{
  DummyFileSystem fs("rootdir");
  fw::dm::DirectoryWatcher dw("/subdir", fs);
  filewatch::FileList response;

  SECTION("Directory not found")
  {
    auto status = dw.fill_file_list(response);
    CHECK(grpc::NOT_FOUND == status.error_code());
    CHECK("'/subdir' does not exist" == status.error_message());
    CHECK(response.filenames_size() == 0);
  }

  SECTION("Sought directory is a file")
  {
    fs.add_file("/", "subdir", 3456);
    auto status = dw.fill_file_list(response);
    CHECK(grpc::NOT_FOUND == status.error_code());
    CHECK("'/subdir' is not a directory" == status.error_message());
    CHECK(response.filenames_size() == 0);
  }

  SECTION("Directory exists but is empty")
  {
    fs.add_dir("/", "subdir", 8234822);
    CHECK(dw.fill_file_list(response).ok());
    CHECK(response.filenames_size() == 0);
  }

  SECTION("subdir contains two files")
  {
    fs.add_dir("/", "subdir", 4632);
    fs.add_file("/subdir", "file0.h", 983982);
    fs.add_file("/subdir", "file0.cpp", 345678);
    fs.add_dir("/subdir", "subsub0", 38028);
    CHECK(dw.fill_file_list(response).ok());
    REQUIRE(response.filenames_size() == 2);
    CHECK(response.filenames(0).name() == "file0.cpp");
    CHECK(response.filenames(0).modification_time().epoch() == 345678);
    CHECK(response.filenames(0).dirname().name() == "/subdir");
    CHECK(response.filenames(0).dirname().modification_time().epoch() == 4632);
    CHECK(response.filenames(1).name() == "file0.h");
    CHECK(response.filenames(1).modification_time().epoch() == 983982);
    CHECK(response.filenames(1).dirname().name() == "/subdir");
    CHECK(response.filenames(1).dirname().modification_time().epoch() == 4632);
  }
}

TEST_CASE("fill file list sets dirname", "[DirectoryWatcher]")
{
  DummyFileSystem fs("rootdir", 382932123);
  fs.add_dir("/", "subdir", 27389272);
  fs.add_file("/subdir", "filename", 17283);

  filewatch::FileList response;

  SECTION("root dir")
  {
    fw::dm::DirectoryWatcher dw("/", fs);
    CHECK(dw.fill_file_list(response).ok());
    CHECK("/" == response.name().name());
    CHECK(response.name().modification_time().epoch() == 382932123);
  }

  SECTION("subdir")
  {
    fw::dm::DirectoryWatcher dw("/subdir", fs);
    CHECK(dw.fill_file_list(response).ok());
    CHECK("/subdir" == response.name().name());
    CHECK(response.name().modification_time().epoch() == 27389272);
  }

  SECTION("dir not found")
  {
    fw::dm::DirectoryWatcher dw("/subdir/not_existing", fs);
    auto status = dw.fill_file_list(response);
    CHECK(grpc::NOT_FOUND == status.error_code());
    CHECK("'/subdir/not_existing' does not exist" == status.error_message());
    CHECK("/subdir/not_existing" == response.name().name());
    // CHECK(response.name().modification_time().epoch() == UNDEFINED);
  }
}

TEST_CASE("fill file list sets size", "[DirectoryWatcher]")
{
  DummyFileSystem fs("rootdir", 382932123);
  fs.add_dir("/", "subdir", 27389272);
  fs.add_file("/subdir", "filename", 17283);

  filewatch::FileList response;
  fw::dm::DirectoryWatcher dw("/subdir", fs);

  SECTION("empty file")
  {
    CHECK(dw.fill_file_list(response).ok());
    REQUIRE(response.filenames_size() == 1);
    CHECK(response.filenames(0).size() == 0);
  }

  SECTION("file with content")
  {
    fs.write_file("/subdir/filename", "content");
    CHECK(dw.fill_file_list(response).ok());
    REQUIRE(response.filenames_size() == 1);
    CHECK(response.filenames(0).size() == fs.size("/subdir/filename"));
  }

  SECTION("file with more content")
  {
    fs.write_file("/subdir/filename", "more content");
    CHECK(dw.fill_file_list(response).ok());
    REQUIRE(response.filenames_size() == 1);
    CHECK(response.filenames(0).size() == fs.size("/subdir/filename"));
  }
}

namespace
{
  class DummyDirectoryEventListener : public fw::dm::DirectoryEventListener
  {
  public:
    void notify(filewatch::DirectoryEvent::Event event,
                std::string_view containing_dir,
                std::string_view dir_name,
                uint64_t mtime) override
    {
      events.emplace_back(EventStruct{event, containing_dir, dir_name, mtime});
    }

    struct EventStruct
    {
      EventStruct(filewatch::DirectoryEvent::Event e, std::string_view cd,
                  std::string_view dn, uint64_t m) :
        event(e),
        containing_dir(cd), dir_name(dn), mtime(m)
      {
      }

      filewatch::DirectoryEvent::Event event;
      std::string containing_dir;
      std::string dir_name;
      uint64_t mtime;
    };

    std::vector<EventStruct> events;
  };

}  // anonymous namespace

TEST_CASE("listen for events", "[DirectoryWatcher]")
{
  DummyFileSystem fs("rootdir");
  fs.add_dir("/", "dir", 123454);
  fs.add_dir("/", "otherdir", 123454);
  DummyDirectoryEventListener listener;

  SECTION("adds and removes watch")
  {
    fw::dm::DirectoryWatcher dw("/dir", fs);
    dw.register_event_listener(listener);
    REQUIRE(fs.listeners.size() == 1);
    CHECK(fs.listeners[0].first == &listener);
    CHECK(fs.listeners[0].second == "/dir");

    dw.unregister_event_listener(listener);
    CHECK(fs.listeners.empty());
  }

  SECTION("adds and removes watch on different directory")
  {
    fw::dm::DirectoryWatcher dw("/otherdir", fs);
    dw.register_event_listener(listener);
    REQUIRE(fs.listeners.size() == 1);
    CHECK(fs.listeners[0].first == &listener);
    CHECK(fs.listeners[0].second == "/otherdir");

    dw.unregister_event_listener(listener);
    CHECK(fs.listeners.empty());
  }
}
