#include "filewatcher.h"

#include "filesystem.h"
#include "filewatch.pb.h"

#include <fmt/format.h>
#include <grpcpp/impl/codegen/status.h>

fw::dm::FileWatcher::FileWatcher(std::string_view dirpath_,
                                 std::string_view filename_,
                                 FileSystem& fs_) :
  dirpath(dirpath_),
  filename(filename_), fs(fs_)
{
}

grpc::Status
fw::dm::FileWatcher::fill_contents(filewatch::FileContent& response) const
{
  auto direntry = fs.get_direntry(dirpath);
  auto filepath = fs.join(dirpath, filename);
  auto fileentry = fs.get_direntry(filepath);
  if (!fileentry)
  {
    return grpc::Status{grpc::NOT_FOUND,
                        fmt::format("'{}' does not exist", filepath)};
  }
  if (fileentry->is_dir)
  {
    return grpc::Status{grpc::FAILED_PRECONDITION,
                        fmt::format("'{}' is not a file", filepath)};
  }

  assert(direntry);  // if file exists in dirpath, then so does dirpath
  response.mutable_dirname()->set_name(dirpath);
  response.mutable_dirname()->mutable_modification_time()->set_epoch(
    direntry->mtime);
  response.mutable_filename()->set_name(fileentry->name);
  response.mutable_filename()->mutable_modification_time()->set_epoch(
    fileentry->mtime);
  std::string contents{fs.read(filepath)};
  std::size_t pos = 0;
  do
  {
    auto nextpos = contents.find_first_of('\n', pos);
    if (nextpos == std::string::npos)
    {
      response.add_lines(contents.substr(pos));
      pos = nextpos;
    }
    else
    {
      response.add_lines(contents.substr(pos, nextpos - pos));
      pos = nextpos + 1;
    }
  } while (pos != std::string::npos);

  return grpc::Status::OK;
}
