#include "filesystem.h"

#include "common/loop_thread.h"
#include "linuxfilesystem.h"
#include "windowsfilesystem.h"

fw::dm::FileSystem::~FileSystem() = default;

std::unique_ptr<fw::dm::FileSystem>
fw::dm::create_filesystem(std::string_view rootdir)
{
  return std::make_unique<fw::dm::OSFileSystem<fw::dm::DefaultFileSystem>>(
    rootdir);
}
