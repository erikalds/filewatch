#include "restserver.h"

#include "staticpages.h"
#include "fileresources.h"
#include <crow.h>
#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <spdlog/spdlog.h>

fw::web::RESTServer::RESTServer(uint16_t port) :
  app(std::make_unique<crow::SimpleApp>()),
  channel(grpc::CreateChannel("localhost:45678",
                              grpc::InsecureChannelCredentials())),
  file_resources(std::make_unique<FileResources>(*app, channel)),
  static_pages(std::make_unique<StaticPages>(*app))
{
  app->port(port).multithreaded();
}

fw::web::RESTServer::~RESTServer()
{
  app->stop();
}

int fw::web::RESTServer::run()
{
  app->run();
  return 0;
}
