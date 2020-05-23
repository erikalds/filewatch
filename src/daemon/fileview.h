#ifndef FILEVIEW_H
#define FILEVIEW_H

namespace filewatch
{
  class FileContent;
}  // namespace filewatch

namespace grpc
{
  class Status;
}  // namespace grpc

namespace fw
{
  namespace dm
  {
    class FileView
    {
    public:
      virtual ~FileView() = 0;

      virtual grpc::Status
      fill_contents(filewatch::FileContent& response) const = 0;
    };

  }  // namespace dm
}  // namespace fw

#endif /* FILEVIEW_H */
