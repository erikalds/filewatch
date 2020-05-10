#include "windowsfilesystem.h"

#ifdef _WIN32

void fw::dm::OSFileSystem::watch(std::string_view dirname,
                                 DirectoryEventListener& listener)
{
#  error Not implemented
}

void fw::dm::OSFileSystem::stop_watching(std::string_view dirname,
                                         DirectoryEventListener& listener)
{
}

#endif  // _WIN32
