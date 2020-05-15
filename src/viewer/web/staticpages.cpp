#include "staticpages.h"

#include <crow.h>
#include <spdlog/spdlog.h>

namespace {

  std::string load_binary(const std::string& path)
  {
    std::ifstream in("./pages/" + path,
                     std::ios_base::in | std::ios_base::binary);
    if (!in)
      return {};

    std::string contents;
    std::copy(std::istreambuf_iterator<char>{in},
              std::istreambuf_iterator<char>{},
              std::back_inserter<std::string>(contents));
    return contents;
  }

}  // anonymous namespace

fw::web::StaticPages::StaticPages(crow::SimpleApp& app)
{
  crow::mustache::set_base("./pages");

  CROW_ROUTE(app, "/")
    ([]
     {
       spdlog::info("loading page: {}{}",
                    crow::mustache::detail::get_template_base_directory_ref(),
                    "index.html");
       crow::response res = crow::mustache::load_text("index.html");
       res.set_header("Content-Type", "text/html");
       return res;
     });

  CROW_ROUTE(app, "/logo192.png")
    ([]
     {
       spdlog::info("loading page: {}{}",
                    crow::mustache::detail::get_template_base_directory_ref(),
                    "logo192.png");
       crow::response res = load_binary("logo192.png");
       res.set_header("Content-Type", "image/png");
       return res;
     });

  CROW_ROUTE(app, "/favicon.ico")
    ([]
     {
       spdlog::info("loading page: {}{}",
                    crow::mustache::detail::get_template_base_directory_ref(),
                    "favicon.ico");
       crow::response res = load_binary("favicon.ico");
       res.set_header("Content-Type", "image/x-icon");
       return res;
     });

  CROW_ROUTE(app, "/static/css/<string>")
    ([](const std::string& page)
     {
       spdlog::info("loading page: {}static/css/{}",
                    crow::mustache::detail::get_template_base_directory_ref(),
                    page);
       crow::response res = crow::mustache::load_text(fmt::format("static/css/{}", page));
       res.set_header("Content-Type", "text/css");
       return res;
     });

  CROW_ROUTE(app, "/static/js/<string>")
    ([](const std::string& page)
     {
       spdlog::info("loading page: {}static/js/{}",
                    crow::mustache::detail::get_template_base_directory_ref(),
                    page);
       crow::response res = crow::mustache::load_text(fmt::format("static/js/{}", page));
       res.set_header("Content-Type", "text/javascript");
       return res;
     });
}

fw::web::StaticPages::~StaticPages() = default;
