#include "defaultfilesystem.h"


fw::dm::DefaultFileSystem::DefaultFileSystem(std::string_view rootdir_) :
  rootdir(std::filesystem::absolute(std::filesystem::path(rootdir_)))
{
}

std::deque<fw::dm::fs::DirectoryEntry>
fw::dm::DefaultFileSystem::ls(std::string_view dirname) const
{
  auto dir = join(rootdir.string(), dirname);
  std::deque<fw::dm::fs::DirectoryEntry> entries;
  for (auto& p : std::filesystem::directory_iterator(dir))
    entries.push_back(create_direntry(p.path()));

  return entries;
}

std::optional<fw::dm::fs::DirectoryEntry>
fw::dm::DefaultFileSystem::get_direntry(std::string_view entryname) const
{
  auto name = join(rootdir.string(), entryname);
  if (!std::filesystem::exists(name))
    return std::optional<fw::dm::fs::DirectoryEntry>{};

  return create_direntry(name);
}

bool fw::dm::DefaultFileSystem::exists(std::string_view filename) const
{
  return std::filesystem::exists(join(rootdir.string(), filename));
}

bool fw::dm::DefaultFileSystem::isdir(std::string_view dirname) const
{
  return std::filesystem::is_directory(join(rootdir.string(), dirname));
}

fw::dm::fs::DirectoryEntry
fw::dm::DefaultFileSystem::create_direntry(const std::filesystem::path& p) const
{
  fw::dm::fs::DirectoryEntry dirent;
  dirent.name = p.filename().string();
  dirent.is_dir = std::filesystem::is_directory(p);
  auto t = std::filesystem::last_write_time(p).time_since_epoch();
  dirent.mtime = static_cast<uint64_t>(
    std::chrono::duration_cast<std::chrono::milliseconds>(t).count());
  return dirent;
}
