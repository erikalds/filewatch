#ifndef DIRECTORYWATCHER_H
#define DIRECTORYWATCHER_H

#include "daemon/directoryview.h"
#include <string>
#include <string_view>

namespace fw
{
  namespace dm
  {
    class FileSystem;

    class DirectoryWatcher : public DirectoryView
    {
    public:
      DirectoryWatcher(std::string_view dirname, const FileSystem& fs);

      status_code fill_dir_list(filewatch::DirList& response) const override;

    private:
      std::string dirname;
      const FileSystem& fs;
    };

  }  // dm
}  // fw


#endif /* DIRECTORYWATCHER_H */
