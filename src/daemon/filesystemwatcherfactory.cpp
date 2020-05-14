#include "daemon/filesystemwatcherfactory.h"

#include "daemon/directorywatcher.h"
#include "daemon/filewatcher.h"
#include "daemon/filesystem.h"

fw::dm::FileSystemWatcherFactory::FileSystemWatcherFactory(
  std::unique_ptr<FileSystem> fs_) :
  fs(std::move(fs_))
{
}

fw::dm::FileSystemWatcherFactory::~FileSystemWatcherFactory() = default;

std::unique_ptr<fw::dm::DirectoryView>
fw::dm::FileSystemWatcherFactory::create_directory(std::string_view dirname)
{
  return std::make_unique<fw::dm::DirectoryWatcher>(dirname, *fs);
}

std::unique_ptr<fw::dm::FileView>
fw::dm::FileSystemWatcherFactory::create_file(std::string_view dirname,
                                              std::string_view filename)
{
  return std::make_unique<fw::dm::FileWatcher>(dirname, filename, *fs);
}
