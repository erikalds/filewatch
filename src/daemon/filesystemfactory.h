#ifndef FILESYSTEMFACTORY_H
#define FILESYSTEMFACTORY_H

#include <memory>
#include <string_view>

namespace fw
{
  namespace dm
  {
    class DirectoryView;


    class FileSystemFactory
    {
    public:
      virtual ~FileSystemFactory() = 0;

      virtual std::unique_ptr<DirectoryView>
      create_directory(std::string_view dirname) = 0;
    };

  }  // namespace dm
}  // namespace fw

#endif /* FILESYSTEMFACTORY_H */
