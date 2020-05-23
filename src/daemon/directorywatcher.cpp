#include "directorywatcher.h"

#include "filesystem.h"
#include "filewatch.pb.h"

#include <grpcpp/impl/codegen/status.h>

fw::dm::DirectoryWatcher::DirectoryWatcher(std::string_view dirname_,
                                           FileSystem& fs_) :
  dirname(dirname_),
  fs(fs_)
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

  template<typename EntryContainerT>
  void fill_entry(EntryContainerT& dest,
                  const fw::dm::fs::DirectoryEntry& entry)
  {
    dest.set_name(entry.name);
    set_mtime_of(dest, entry);
  }

}  // anonymous namespace

grpc::Status
fw::dm::DirectoryWatcher::fill_dir_list(filewatch::DirList& response) const
{
  return fill_entry_list(
    response,
    [](const fs::DirectoryEntry& direntry) { return !direntry.is_dir; },
    [](filewatch::DirList& resp, const fs::DirectoryEntry& direntry) {
      auto* dn = resp.add_dirnames();
      fill_entry(*dn, direntry);
    });
}

grpc::Status
fw::dm::DirectoryWatcher::fill_file_list(filewatch::FileList& response) const
{
  return fill_entry_list(
    response,
    [](const fs::DirectoryEntry& direntry) { return direntry.is_dir; },
    [&](filewatch::FileList& resp, const fs::DirectoryEntry& direntry) {
      auto* fn = resp.add_filenames();
      fn->mutable_dirname()->set_name(this->dirname);
      auto containing_dir_entry = fs.get_direntry(this->dirname);
      set_mtime_of(*fn->mutable_dirname(), *containing_dir_entry);
      fill_entry(*fn, direntry);
      fn->set_size(direntry.size);
    });
}

void fw::dm::DirectoryWatcher::register_event_listener(
  DirectoryEventListener& listener)
{
  fs.watch(dirname, listener);
}

void fw::dm::DirectoryWatcher::unregister_event_listener(
  DirectoryEventListener& listener)
{
  fs.stop_watching(dirname, listener);
}

template<typename ResponseListT, typename AddEntryFunctionT>
grpc::Status
fw::dm::DirectoryWatcher::fill_entry_list(ResponseListT& response,
                                          const FilterFunction& filter,
                                          AddEntryFunctionT add_entry) const
{
  response.mutable_name()->set_name(dirname);

  if (!fs.exists(dirname))
  {
    return grpc::Status(grpc::NOT_FOUND, "'" + dirname + "' does not exist");
  }

  if (!fs.isdir(dirname))
  {
    return grpc::Status(grpc::NOT_FOUND,
                        "'" + dirname + "' is not a directory");
  }

  set_mtime_of(*response.mutable_name(), *fs.get_direntry(dirname));

  for (const auto& direntry : fs.ls(dirname))
  {
    if (filter(direntry))
    {
      continue;
    }

    add_entry(response, direntry);
  }
  return grpc::Status::OK;
}
