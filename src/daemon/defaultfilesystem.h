#ifndef DEFAULTFILESYSTEM_H
#define DEFAULTFILESYSTEM_H

#include "daemon/filesystem.h"

#include <filesystem>

namespace fw
{
  namespace dm
  {
    class DefaultFileSystem : public FileSystem
    {
    public:
      explicit DefaultFileSystem(std::string_view rootdir);

      std::deque<fs::DirectoryEntry>
      ls(std::string_view dirname) const override;

      std::optional<fs::DirectoryEntry>
      get_direntry(std::string_view entryname) const override;

      bool exists(std::string_view filename) const override;
      bool isdir(std::string_view dirname) const override;

    private:
      fs::DirectoryEntry create_direntry(const std::filesystem::path& p) const;

      std::filesystem::path rootdir;
    };

  }  // namespace dm

}  // namespace fw


#endif /* DEFAULTFILESYSTEM_H */
