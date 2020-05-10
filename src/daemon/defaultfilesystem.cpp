#include "defaultfilesystem.h"

#include <spdlog/spdlog.h>

fw::dm::DefaultFileSystem::DefaultFileSystem(std::string_view rootdir_) :
  FileSystem(std::filesystem::absolute(std::filesystem::path(rootdir_)))
{
}

std::deque<fw::dm::fs::DirectoryEntry>
fw::dm::DefaultFileSystem::ls(std::string_view dirname) const
{
  auto dir = join(rootdir.string(), dirname);
  std::deque<fw::dm::fs::DirectoryEntry> entries;
  for (const auto& p : std::filesystem::directory_iterator(dir))
  {
    entries.push_back(create_direntry(p.path()));
  }

  return entries;
}

std::optional<fw::dm::fs::DirectoryEntry>
fw::dm::DefaultFileSystem::get_direntry(std::string_view entryname) const
{
  auto name = join(rootdir.string(), entryname);
  if (!std::filesystem::exists(name))
  {
    return std::optional<fw::dm::fs::DirectoryEntry>{};
  }

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

#include <iostream>

fw::dm::fs::DirectoryEntry
fw::dm::DefaultFileSystem::create_direntry(const std::filesystem::path& p)
{
  fw::dm::fs::DirectoryEntry dirent;
  dirent.name = p.filename().string();
  dirent.is_dir = std::filesystem::is_directory(p);
  std::error_code ec;
  auto tp = std::filesystem::last_write_time(p, ec);
  auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
    tp - decltype(tp)::clock::now() + std::chrono::system_clock::now());
  auto t = sctp.time_since_epoch();
  if (ec)
  {
    spdlog::error("path {} gave error: {}\n", p.string(), ec.message());
  }
  // NOLINTNEXTLINE
  using ms64_t = std::chrono::duration<uint64_t, std::ratio<1, 1000>>;
  auto msecs = std::chrono::duration_cast<ms64_t>(t).count();

  auto tp_t_c = decltype(sctp)::clock::to_time_t(sctp);
  auto* tp_c = std::localtime(&tp_t_c);
  std::ostringstream ost;
  ost << std::put_time(tp_c, "%F %T");
  spdlog::debug("path {} last write time: {} ms since epoch (at {}).\n",
                p.string(), msecs, ost.str());

  dirent.mtime = static_cast<uint64_t>(msecs);
  return dirent;
}
