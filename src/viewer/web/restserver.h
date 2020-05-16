#ifndef RESTSERVER_H
#define RESTSERVER_H

#include "crow_fwd.h"
#include <filesystem>
#include <memory>

namespace grpc { class Channel; }

namespace fw
{
  namespace web
  {
    class FileResources;
    class StaticPages;

    class RESTServer
    {
    public:
      RESTServer(uint16_t port, const std::filesystem::path& pagedir);
      RESTServer(const RESTServer&) = delete;
      RESTServer& operator=(const RESTServer&) = delete;
      RESTServer(RESTServer&&) = delete;
      RESTServer& operator=(RESTServer&&) = delete;
      ~RESTServer();

      int run();

    private:
      std::unique_ptr<crow::SimpleApp> app; // must be created first
      std::shared_ptr<grpc::Channel> channel; // must be created second;
      std::unique_ptr<FileResources> file_resources;
      std::unique_ptr<StaticPages> static_pages;
    };

  }  // namespace web
}  // namespace fw

#endif /* RESTSERVER_H */
