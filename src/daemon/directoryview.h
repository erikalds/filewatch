#ifndef DIRECTORYVIEW_H
#define DIRECTORYVIEW_H

namespace filewatch
{
  class DirList;
  class FileList;
}  // namespace filewatch

namespace grpc
{
  class Status;
}

namespace fw
{
  namespace dm
  {
    class DirectoryEventListener;

    class DirectoryView
    {
    public:
      virtual ~DirectoryView() = 0;

      virtual grpc::Status
      fill_dir_list(filewatch::DirList& response) const = 0;
      virtual grpc::Status
      fill_file_list(filewatch::FileList& response) const = 0;

      virtual void
      register_event_listener(DirectoryEventListener& listener) = 0;
      virtual void
      unregister_event_listener(DirectoryEventListener& listener) = 0;
    };

  }  // namespace dm
}  // namespace fw

#endif /* DIRECTORYVIEW_H */
