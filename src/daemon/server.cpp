/* Source file created: 2019-03-24

   filewatch - File watcher utility
   Copyright (C) 2019 Erik Åldstedt Sund

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   To contact the author, e-mail at erikalds@gmail.com or through
   regular mail:
   Erik Åldstedt Sund
   Darres veg 14
   NO-7540 KLÆBU
   NORWAY
*/

#include "server.h"

#include "filewatch.grpc.pb.h"

#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/security/server_credentials.h>
#include <csignal>
#include <filesystem>
#include <iostream>

namespace fw
{
  namespace dm
  {

    namespace
    {

      class FWService : public filewatch::FileWatch::Service
      {
      public:
        ::grpc::Status GetStatus(::grpc::ServerContext* context,
                                 const ::filewatch::Void* request,
                                 ::filewatch::FileWatchStatus* response) override
        {
          response->set_status(filewatch::FileWatchStatus::OK);
          response->set_msg("up-and-running");
          return grpc::Status::OK;
        }

        ::grpc::Status GetVersion(::grpc::ServerContext* context,
                                  const ::filewatch::Void* request,
                                  ::filewatch::Version* response) override
        {
          response->set_major(1);
          response->set_minor(0);
          response->set_revision(0);
          return grpc::Status::OK;
        }

      };

      int64_t mtime(const std::filesystem::path& p)
      {
        auto t = std::filesystem::last_write_time(p).time_since_epoch();
        return std::chrono::duration_cast<std::chrono::milliseconds>(t).count();
      }

      template<typename T>
      void set_mtime_of(T& var, const std::filesystem::path& p)
      {
        var.mutable_modification_time()->set_epoch(mtime(p));
      }

      class DirService : public filewatch::Directory::Service
      {
      public:
        explicit DirService(const std::string& rootdir) : rootdir(rootdir) {}
        ::grpc::Status ListFiles(::grpc::ServerContext* context,
                                 const ::filewatch::Directoryname* request,
                                 ::filewatch::FileList* response) override
        {
          response->mutable_name()->set_name(request->name());
          auto dir = rootdir / std::filesystem::path(request->name());
          set_mtime_of(*response->mutable_name(), dir);
          for (auto& p : std::filesystem::directory_iterator(dir))
          {
            if (p.is_directory())
              continue;

            filewatch::Filename* fn = response->add_filenames();
            fn->mutable_dirname()->set_name(request->name());
            fn->set_name(p.path().filename().string());
            set_mtime_of(*fn, p.path());
          }
          return grpc::Status::OK;
        }

      private:
        std::filesystem::path rootdir;
      };

      class FileService : public filewatch::File::Service
      {
      public:
        ::grpc::Status GetContents(::grpc::ServerContext* context,
                                   const ::filewatch::Filename* request,
                                   ::filewatch::FileContent* response) override
        {
          return grpc::Status::OK;
        }
      };

    }  // anonymous namespace


    class Services
    {
    public:
      explicit Services(const std::string& rootdir) :
        Directory(rootdir)
      {}

      void register_services(grpc::ServerBuilder& builder)
      {
        builder.RegisterService(&File);
        builder.RegisterService(&FileWatch);
        builder.RegisterService(&Directory);
      }

    private:
      FWService FileWatch;
      DirService Directory;
      FileService File;
    };


    static Server* g_server = nullptr;

    void signal_handler(int sig)
    {
      if (g_server == nullptr)
        return;

      if (SIGTERM == sig || SIGINT == sig)
        g_server->stop();
    }


    Server::Server(const std::string& rootdir) :
      services(std::make_unique<Services>(rootdir))
    {
      g_server = this;
      std::signal(SIGTERM, signal_handler);
      std::signal(SIGINT, signal_handler);
    }

    Server::~Server()
    {
      std::signal(SIGTERM, SIG_DFL);
      std::signal(SIGINT, SIG_DFL);
      g_server = nullptr;
    }

    int Server::run()
    {
      grpc::ServerBuilder builder;
      builder.AddListeningPort("localhost:45678",
                               grpc::InsecureServerCredentials());
      services->register_services(builder);
      server = builder.BuildAndStart();
      server->Wait();
      return 0;
    }

    void Server::stop()
    {
      std::cout << "Terminating filewatch daemon" << std::endl;
      server->Shutdown();
    }

  }  // dm
}  // fw
