#ifndef STATICPAGES_H
#define STATICPAGES_H

#include "crow_fwd.h"
#include <filesystem>

namespace fw
{
namespace web
{

  class StaticPages
  {
  public:
    StaticPages(crow::SimpleApp& app, const std::filesystem::path& pagedir);
  };

}  // namespace web
}  // namespace fw


#endif /* STATICPAGES_H */
