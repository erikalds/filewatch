#include "restserver.h"

#include "staticpages.h"
#include <crow.h>

fw::web::RESTServer::RESTServer(uint16_t port) :
  app(std::make_unique<crow::SimpleApp>()),
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
