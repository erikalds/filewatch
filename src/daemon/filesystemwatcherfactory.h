#ifndef FILESYSTEMWATCHERFACTORY_H
#define FILESYSTEMWATCHERFACTORY_H

#include "daemon/filesystemfactory.h"

namespace fw
{
  namespace dm
  {

    class FileSystem;


    class FileSystemWatcherFactory : public FileSystemFactory
    {
    public:
      explicit FileSystemWatcherFactory(std::unique_ptr<FileSystem> fs);
      ~FileSystemWatcherFactory();

      std::unique_ptr<DirectoryView>
      create_directory(std::string_view dirname) override;

    private:
      std::unique_ptr<FileSystem> fs;
    };

  }  // namespace dm
}  // namespace fw


#endif /* FILESYSTEMWATCHERFACTORY_H */
