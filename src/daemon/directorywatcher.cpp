#include "directorywatcher.h"

#include "filesystem.h"
#include "filewatch.pb.h"

fw::dm::DirectoryWatcher::DirectoryWatcher(std::string_view dirname,
                                           const FileSystem& fs) :
  dirname(dirname),
  fs(fs)
{
}

namespace
{

  void set_mtime_of(filewatch::Directoryname& dn,
                    const fw::dm::FileSystem::DirectoryEntry& entry)
  {
    dn.mutable_modification_time()->set_epoch(entry.mtime);
  }

}  // anonymous namespace

fw::dm::status_code
fw::dm::DirectoryWatcher::fill_dir_list(filewatch::DirList& response) const
{
  response.mutable_name()->set_name(dirname);
  if (!fs.exists(dirname))
    return status_code::NOT_FOUND;

  if (!fs.isdir(dirname))
    return status_code::NOT_FOUND;

  set_mtime_of(*response.mutable_name(), *fs.get_direntry(dirname));

  for (auto direntry : fs.ls(dirname))
  {
    if (!direntry.is_dir)
      continue;

    auto dn = response.add_dirnames();
    dn->set_name(direntry.name);
    set_mtime_of(*dn, direntry);
  }
  return status_code::OK;
}
