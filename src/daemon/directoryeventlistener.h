#ifndef DIRECTORYEVENTLISTENER_H
#define DIRECTORYEVENTLISTENER_H

#include "filewatch.pb.h"

namespace fw
{
  namespace dm
  {
    class DirectoryEventListener
    {
    public:
      virtual ~DirectoryEventListener() = 0;

      virtual void notify(filewatch::DirectoryEvent::Event event,
                          std::string_view containing_dir,
                          std::string_view dir_name,
                          uint64_t mtime) = 0;
    };

  }  // namespace dm
}  // namespace fw


#endif /* DIRECTORYEVENTLISTENER_H */
