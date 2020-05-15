#ifndef STATICPAGES_H
#define STATICPAGES_H

#include "crow_fwd.h"

namespace fw
{
namespace web
{

  class StaticPages
  {
  public:
    explicit StaticPages(crow::SimpleApp& app);
    ~StaticPages();
  };

}  // namespace web
}  // namespace fw


#endif /* STATICPAGES_H */
