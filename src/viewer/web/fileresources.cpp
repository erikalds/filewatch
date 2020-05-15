#include "fileresources.h"

#include "filewatch.grpc.pb.h"
#include <crow.h>
#include <grpcpp/channel.h>
#include <spdlog/spdlog.h>

fw::web::FileResources::FileResources(crow::SimpleApp& app,
                                      std::shared_ptr<grpc::Channel> channel_) :
  channel(channel_)
{
  CROW_ROUTE(app, "/v1.0/files")
    ([&]{
       auto dirstub = filewatch::Directory::NewStub(channel);

       grpc::ClientContext ctxt;
       filewatch::Directoryname dirname;
       filewatch::FileList filelist;
       auto status = dirstub->ListFiles(&ctxt, dirname, &filelist);

       if (status.ok())
       {
         crow::json::wvalue o;
         o["/"]["index"] = 0;
         o["/"]["label"] = "/";
         o["/"]["mtime"] = filelist.name().modification_time().epoch();
         spdlog::info("received filelist of size {}", filelist.filenames_size());
         for (int i = 0; i < filelist.filenames_size(); ++i)
         {
           auto fname = filelist.filenames(i).name();
           o["/"]["nodes"][fname]["index"] = i;
           o["/"]["nodes"][fname]["label"] = fname;
           o["/"]["nodes"][fname]["mtime"] = filelist.filenames(i).modification_time().epoch();
         }
         spdlog::info("returning {}", crow::json::dump(o));
         return o;
       }
       spdlog::info("status not ok");
       return crow::json::wvalue{};
     });
}

fw::web::FileResources::~FileResources() = default;
