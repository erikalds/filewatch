#ifndef FILERESOURCES_H
#define FILERESOURCES_H

#include "crow_fwd.h"
#include <memory>

namespace grpc { class Channel; }

namespace fw
{
  namespace web
  {

    class FileResources
    {
    public:
      explicit FileResources(crow::SimpleApp& app,
                             std::shared_ptr<grpc::Channel> channel_);
      ~FileResources();

    private:
      std::shared_ptr<grpc::Channel> channel;
    };

  }  // namespace web
}  // namespace fw


#endif /* FILERESOURCES_H */
