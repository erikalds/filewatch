#include "staticpages.h"

#include <crow.h>
#include <spdlog/spdlog.h>

namespace {

  std::string load_binary(const std::string& path)
  {
    auto basedir = crow::mustache::detail::get_template_base_directory_ref();
    auto fullpath = fmt::format("{}{}", basedir, path);
    std::ifstream in(fullpath, std::ios_base::in | std::ios_base::binary);
    if (!in)
      return {};

    std::string contents;
    std::copy(std::istreambuf_iterator<char>{in},
              std::istreambuf_iterator<char>{},
              std::back_inserter<std::string>(contents));
    return contents;
  }

  crow::response static_respond(std::string_view path,
                                std::string_view page,
                                const std::string& content_type,
                                std::function<std::string(std::string)> loader=crow::mustache::load_text)
  {
    spdlog::debug("loading page: {}{}{}",
                  crow::mustache::detail::get_template_base_directory_ref(),
                  path, page);
    auto webpath = fmt::format("{}{}", path, page);
    crow::response res = loader(webpath);
    if (res.body.empty())
    {
      spdlog::warn("Page not found: {}", webpath);
      return crow::response{404, fmt::format("Page not found: {}", webpath)};
    }
    res.set_header("Content-Type", content_type);
    return res;
  }

}  // anonymous namespace

fw::web::StaticPages::StaticPages(crow::SimpleApp& app)
{
  crow::mustache::set_base("./pages");

  CROW_ROUTE(app, "/")
    ([]
     {
       return static_respond("", "index.html", "text/html");
     });

  CROW_ROUTE(app, "/logo192.png")
    ([]
     {
       return static_respond("", "logo192.png", "image/png", load_binary);
     });

  CROW_ROUTE(app, "/favicon.ico")
    ([]
     {
       return static_respond("", "favicon.ico", "image/x-icon", load_binary);
     });

  CROW_ROUTE(app, "/static/css/<string>")
    ([](const std::string& page)
     {
       return static_respond("static/css/", page, "text/css");
     });

  CROW_ROUTE(app, "/static/js/<string>")
    ([](const std::string& page)
     {
       return static_respond("static/js/", page, "text/javascript");
     });
}

fw::web::StaticPages::~StaticPages() = default;
