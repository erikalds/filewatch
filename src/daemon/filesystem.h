#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <cassert>
#include <deque>
#include <optional>
#include <string>

namespace fw
{
  namespace dm
  {
    namespace fs
    {

      struct DirectoryEntry
      {
        std::string name;  // name of entry in contained directory
        bool is_dir;  // true if this is a directory
        uint64_t mtime;  // milliseconds since epoch
      };

    }  // namespace fs


    class FileSystem
    {
    public:
      virtual ~FileSystem() = 0;

      virtual std::deque<fs::DirectoryEntry> ls(std::string_view dirname) const = 0;

      virtual std::optional<fs::DirectoryEntry>
      get_direntry(std::string_view entryname) const = 0;

      virtual bool exists(std::string_view filename) const = 0;
      virtual bool isdir(std::string_view dirname) const = 0;

      virtual std::string join(std::string_view containing_dir,
                               std::string_view filename) const final
      { return no_slash_at_end(containing_dir) + "/" + std::string(filename); }

      virtual std::string parent(std::string_view entryname) const final
      {
        if (entryname == "/")
          return std::string(entryname);

        auto en = no_slash_at_end(entryname);
        auto pos = en.find_last_of('/');
        assert(pos != std::string_view::npos);
        if (pos == 0)
          return "/";

        return en.substr(0, pos);
      }

      virtual std::string leaf(std::string_view entryname) const final
      {
        if (entryname == "/")
          return std::string(entryname);

        auto en = no_slash_at_end(entryname);
        auto pos = en.find_last_of('/');
        assert(pos != std::string_view::npos);
        return en.substr(pos + 1);
      }

    protected:
      std::string no_slash_at_end(std::string_view entry) const
      { return std::string(entry.substr(0, entry.back() == '/' ? entry.size() - 1 : entry.size())); }
    };

  }  // namespace dm
}  // namespace fw

#endif /* FILESYSTEM_H */
