#ifndef DIRECTORYWATCHER_H
#define DIRECTORYWATCHER_H

#include "daemon/directoryview.h"

#include <functional>
#include <string>
#include <string_view>

namespace fw
{
  namespace dm
  {
    namespace fs
    {
      struct DirectoryEntry;
    }  // namespace fs


    class FileSystem;

    class DirectoryWatcher : public DirectoryView
    {
    public:
      DirectoryWatcher(std::string_view dirname, const FileSystem& fs);

      grpc::Status fill_dir_list(filewatch::DirList& response) const override;
      grpc::Status fill_file_list(filewatch::FileList& response) const override;

    private:
      typedef std::function<bool(const fs::DirectoryEntry&)> FilterFunction;

      template<typename ResponseListT, typename AddEntryFunctionT>
      grpc::Status fill_entry_list(ResponseListT& response,
                                   FilterFunction filter,
                                   AddEntryFunctionT add_entry) const;

      std::string dirname;
      const FileSystem& fs;
    };

  }  // namespace dm
}  // namespace fw


#endif /* DIRECTORYWATCHER_H */
