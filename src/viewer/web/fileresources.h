#ifndef FILERESOURCES_H
#define FILERESOURCES_H

#include "crow_fwd.h"

#include <memory>

namespace crow
{
  namespace json
  {
    class wvalue;
  }
}  // namespace crow
namespace grpc
{
  class Channel;
}

namespace fw
{
  namespace web
  {
    class FileResources
    {
    public:
      explicit FileResources(crow::SimpleApp& app,
                             std::shared_ptr<grpc::Channel>
                               channel_);
      ~FileResources();

    private:
      crow::json::wvalue build_tree(const std::string& path,
                                    int index = 0) const;
      std::shared_ptr<grpc::Channel> channel;
    };

  }  // namespace web
}  // namespace fw


#endif /* FILERESOURCES_H */
