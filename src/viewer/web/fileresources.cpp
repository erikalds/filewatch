#include "fileresources.h"

#include "filewatch.grpc.pb.h"
#include <crow.h>

#include <grpcpp/channel.h>
#include <spdlog/spdlog.h>

#include <filesystem>

fw::web::FileResources::FileResources(crow::SimpleApp& app,
                                      std::shared_ptr<grpc::Channel>
                                        channel_) :
  channel(std::move(channel_))
{
  CROW_ROUTE(app, "/v1.0/files")
  ([&] {
    crow::json::wvalue o;
    auto tree = build_tree("/");
    auto rtree = crow::json::load(crow::json::dump(tree));
    if (rtree.has("error"))
    {
      return crow::response{static_cast<int>(rtree["error"]["code"])};
    }
    o["/"] = std::move(tree);
    return crow::response{o};
  });
}

fw::web::FileResources::~FileResources() = default;

namespace
{
  std::string create_dir_label(std::string_view dirpath)
  {
    if (dirpath == "/")
    {
      return "/";
    }

    assert(dirpath.ends_with('/'));
    auto pos = dirpath.find_last_of('/', dirpath.size() - 2);
    assert(pos != std::string_view::npos);
    return std::string{dirpath.substr(pos + 1)};
  }

  namespace http
  {
    static const int not_found{404};
    static const int internal_server_error{500};
  }  // namespace http

}  // anonymous namespace

crow::json::wvalue fw::web::FileResources::build_tree(const std::string& path,
                                                      int index) const
{
  auto dirstub = filewatch::Directory::NewStub(channel);

  grpc::ClientContext ctxt;
  filewatch::Directoryname dirname;
  dirname.set_name(std::string{path});
  filewatch::DirList dirlist;
  auto status = dirstub->ListDirectories(&ctxt, dirname, &dirlist);
  if (!status.ok())
  {
    spdlog::warn("Unable to list directory: {}. Msg: {}", path,
                 status.error_message());
    crow::json::wvalue error{};
    error["error"]["message"] = status.error_message();
    if (status.error_code() == grpc::NOT_FOUND)
    {
      error["error"]["code"] = http::not_found;
    }
    else
    {
      error["error"]["code"] = http::internal_server_error;
    }

    return error;
  }

  crow::json::wvalue o;
  o["index"] = index;
  o["label"] = create_dir_label(dirlist.name().name());
  o["mtime"] = dirlist.name().modification_time().epoch();
  crow::json::wvalue nodes;
  for (int i = 0; i < dirlist.dirnames_size(); ++i)
  {
    auto dname = dirlist.dirnames(i).name();
    nodes[dname] = build_tree(path + dname + "/", i);
  }

  filewatch::FileList filelist;
  grpc::ClientContext ctxt2;
  status = dirstub->ListFiles(&ctxt2, dirname, &filelist);

  if (!status.ok())
  {
    spdlog::warn("status not ok");
    return crow::json::wvalue{};
  }

  for (int i = 0; i < filelist.filenames_size(); ++i)
  {
    auto fname = filelist.filenames(i).name();
    nodes[fname]["index"] = dirlist.dirnames_size() + i;
    nodes[fname]["label"] = fname;
    nodes[fname]["mtime"] = filelist.filenames(i).modification_time().epoch();
    nodes[fname]["size"] = filelist.filenames(i).size();
  }

  o["nodes"] = std::move(nodes);

  return o;
}
