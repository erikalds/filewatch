#ifndef WINDOWSFILESYSTEM_H
#define WINDOWSFILESYSTEM_H

#ifdef _WIN32

namespace fw
{
  namespace dm
  {
    class OSFileSystem : public DefaultFileSystem
    {
    public:
      using DefaultFileSystem::DefaultFileSystem;

      void watch(std::string_view dirname,
                 DirectoryEventListener& listener) override;
      void stop_watching(std::string_view dirname,
                         DirectoryEventListener& listener) override;
    };

  }  // namespace dm
}  // namespace fw

#endif  // _WIN32

#endif /* WINDOWSFILESYSTEM_H */
