#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <cassert>
#include <deque>
#include <filesystem>
#include <memory>
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
        uint64_t size;  // size of file/directory in bytes
      };

    }  // namespace fs


    class DirectoryEventListener;


    class FileSystem
    {
    public:
      virtual ~FileSystem() = 0;

      virtual std::deque<fs::DirectoryEntry>
      ls(std::string_view dirname) const = 0;

      virtual std::optional<fs::DirectoryEntry>
      get_direntry(std::string_view entryname) const = 0;

      virtual bool exists(std::string_view filename) const = 0;
      virtual bool isdir(std::string_view dirname) const = 0;

      virtual std::string read(std::string_view filepath) const = 0;

      virtual std::string join(std::string_view containing_dir,
                               std::string_view filename) const final
      {
        return no_slash_at_end(containing_dir) + "/"
               + no_slash_at_beginning(filename);
      }

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

      virtual void watch(std::string_view dirname,
                         DirectoryEventListener& listener) = 0;
      virtual void stop_watching(std::string_view dirname,
                                 DirectoryEventListener& listener) = 0;

    protected:
      std::string no_slash_at_end(std::string_view entry) const
      {
        return std::string(
          entry.substr(0,
                       entry.back() == '/' ? entry.size() - 1 : entry.size()));
      }
      std::string no_slash_at_beginning(std::string_view entry) const
      {
        return std::string(entry.substr(entry.front() == '/' ? 1 : 0));
      }

      explicit FileSystem(const std::filesystem::path& rootdir_) :
        rootdir(rootdir_)
      {
      }

      std::filesystem::path rootdir;
    };

    std::unique_ptr<FileSystem> create_filesystem(std::string_view rootdir);

  }  // namespace dm
}  // namespace fw

#endif /* FILESYSTEM_H */
