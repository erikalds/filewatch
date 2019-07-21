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

#include "directoryview.h"
#include "filesystemfactory.h"

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


      class DirService : public filewatch::Directory::Service
      {
      public:
        explicit DirService(FileSystemFactory& factory) :
          factory(factory) {}

        ::grpc::Status ListFiles(::grpc::ServerContext* /*context*/,
                                 const ::filewatch::Directoryname* request,
                                 ::filewatch::FileList* response) override
        {
          auto dirview = factory.create_directory(request->name());
          return translate_status_code(dirview->fill_file_list(*response),
                                       request->name());
        }

        ::grpc::Status
        ListDirectories(::grpc::ServerContext* /*context*/,
                        const ::filewatch::Directoryname* request,
                        ::filewatch::DirList* response) override
        {
          auto dirview = factory.create_directory(request->name());
          return translate_status_code(dirview->fill_dir_list(*response),
                                       request->name());
        }

      private:
        ::grpc::Status translate_status_code(status_code sc,
                                             std::string_view entryname) const
        {
          if (sc == status_code::OK)
            return grpc::Status::OK;
          else if (sc == status_code::NOT_FOUND)
            return grpc::Status(grpc::NOT_FOUND,
                                "'" + std::string(entryname) + "' does not exist");
          else
          {
            assert(sc == status_code::NOT_A_DIR);
            return grpc::Status(grpc::NOT_FOUND,
                                "'" + std::string(entryname) + "' is not a directory");
          }
        }

        FileSystemFactory& factory;
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
      explicit Services(std::unique_ptr<FileSystemFactory> factory_) :
        factory(std::move(factory_)),
        Directory(*factory)
      {}

      void register_services(grpc::ServerBuilder& builder)
      {
        builder.RegisterService(&File);
        builder.RegisterService(&FileWatch);
        builder.RegisterService(&Directory);
      }

    private:
      std::unique_ptr<FileSystemFactory> factory;
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


    Server::Server(std::unique_ptr<FileSystemFactory> factory) :
      services(std::make_unique<Services>(std::move(factory)))
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
