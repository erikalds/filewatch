#include "daemon/filesystemwatcherfactory.h"

#include "daemon/directorywatcher.h"
#include "daemon/filesystem.h"

fw::dm::FileSystemWatcherFactory::FileSystemWatcherFactory(std::unique_ptr<FileSystem> fs) :
  fs(std::move(fs))
{
}

fw::dm::FileSystemWatcherFactory::~FileSystemWatcherFactory()
{
}

std::unique_ptr<fw::dm::DirectoryView>
fw::dm::FileSystemWatcherFactory::create_directory(std::string_view dirname)
{
  return std::make_unique<fw::dm::DirectoryWatcher>(dirname, *fs);
}
