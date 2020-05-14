#ifndef FILEWATCHER_H
#define FILEWATCHER_H

#include "fileview.h"
#include <string>

namespace fw {
  namespace dm {

    class FileSystem;

    class FileWatcher : public FileView
    {
    public:
      FileWatcher(std::string_view dirpath_, std::string_view filename_,
                  FileSystem& fs_);

      grpc::Status
      fill_contents(filewatch::FileContent& response) const override;

    public:
      std::string dirpath;
      std::string filename;
      FileSystem& fs;
    };

  }  // namespace dm
}  // namespace fw

#endif /* FILEWATCHER_H */
