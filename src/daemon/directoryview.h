#ifndef DIRECTORYVIEW_H
#define DIRECTORYVIEW_H

namespace filewatch {
  class DirList;
  class FileList;
}  // namespace filewatch

namespace fw {
  namespace dm {

    enum class status_code
    {
      OK,
      NOT_FOUND,
      NOT_A_DIR
    };

    class DirectoryView
    {
    public:
      virtual ~DirectoryView() = 0;

      virtual status_code fill_dir_list(filewatch::DirList& response) const = 0;
      virtual status_code fill_file_list(filewatch::FileList& response) const = 0;
    };

  }  // namespace dm
}  // namespace fw

#endif /* DIRECTORYVIEW_H */
