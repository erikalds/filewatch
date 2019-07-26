#include "directorywatcher.h"

#include "filesystem.h"
#include "filewatch.pb.h"
#include <grpcpp/impl/codegen/status.h>

fw::dm::DirectoryWatcher::DirectoryWatcher(std::string_view dirname,
                                           const FileSystem& fs) :
  dirname(dirname),
  fs(fs)
{
}

namespace
{

  template<typename ModificationTimeContainerT>
  void set_mtime_of(ModificationTimeContainerT& mtc,
                    const fw::dm::fs::DirectoryEntry& entry)
  {
    mtc.mutable_modification_time()->set_epoch(entry.mtime);
  }

  template<typename NameAndModificationTimeContainerT>
  void set_name_and_mtime_of(NameAndModificationTimeContainerT& namtc,
                             const fw::dm::fs::DirectoryEntry& entry)
  {
    namtc.set_name(entry.name);
    set_mtime_of(namtc, entry);
  }

}  // anonymous namespace

grpc::Status
fw::dm::DirectoryWatcher::fill_dir_list(filewatch::DirList& response) const
{
  return fill_entry_list(response,
                         [](const fs::DirectoryEntry& direntry)
                         {
                           return !direntry.is_dir;
                         },
                         [](filewatch::DirList& response,
                            const fs::DirectoryEntry& direntry)
                         {
                           auto dn = response.add_dirnames();
                           set_name_and_mtime_of(*dn, direntry);
                         });
}

grpc::Status
fw::dm::DirectoryWatcher::fill_file_list(filewatch::FileList& response) const
{
  return fill_entry_list(response,
                         [](const fs::DirectoryEntry& direntry)
                         {
                           return direntry.is_dir;
                         },
                         [&](filewatch::FileList& response,
                             const fs::DirectoryEntry& direntry)
                         {
                           auto fn = response.add_filenames();
                           fn->mutable_dirname()->set_name(this->dirname);
                           set_mtime_of(*fn->mutable_dirname(),
                                        *fs.get_direntry(this->dirname));
                           set_name_and_mtime_of(*fn, direntry);
                         });
}

template<typename ResponseListT, typename AddEntryFunctionT>
grpc::Status
fw::dm::DirectoryWatcher::
fill_entry_list(ResponseListT& response,
                FilterFunction filter,
                AddEntryFunctionT add_entry) const
{
  response.mutable_name()->set_name(dirname);

  if (!fs.exists(dirname))
    return grpc::Status(grpc::NOT_FOUND, "'" + dirname + "' does not exist");

  if (!fs.isdir(dirname))
    return grpc::Status(grpc::NOT_FOUND, "'" + dirname + "' is not a directory");

  set_mtime_of(*response.mutable_name(), *fs.get_direntry(dirname));

  for (auto direntry : fs.ls(dirname))
  {
    if (filter(direntry))
      continue;

    add_entry(response, direntry);
  }
  return grpc::Status::OK;
}
