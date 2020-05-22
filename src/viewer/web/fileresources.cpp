#include "fileresources.h"

#include "filewatch.grpc.pb.h"
#include <crow.h>
#include <grpcpp/channel.h>
#include <spdlog/spdlog.h>
#include <filesystem>

fw::web::FileResources::FileResources(crow::SimpleApp& app,
                                      std::shared_ptr<grpc::Channel> channel_) :
  channel(std::move(channel_))
{
  CROW_ROUTE(app, "/v1.0/files")
    ([&]{
       crow::json::wvalue o;
       o["/"] = build_tree("/");
       return o;
     });
}

fw::web::FileResources::~FileResources() = default;

namespace {
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
    spdlog::warn("status not ok 2");
    return crow::json::wvalue{};
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
  }

  o["nodes"] = std::move(nodes);

  return o;
}
