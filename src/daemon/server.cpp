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

#include "directoryeventlistener.h"
#include "directoryview.h"
#include "filesystemfactory.h"
#include "fileview.h"
#include "filewatch.grpc.pb.h"

#include <grpc++/security/server_credentials.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <spdlog/spdlog.h>

#include <csignal>
#include <filesystem>
#include <thread>

namespace fw::dm
{
  namespace
  {
    class FWService : public filewatch::FileWatch::Service
    {
    public:
      ::grpc::Status GetStatus(::grpc::ServerContext* /*context*/,
                               const ::filewatch::Void* /*request*/,
                               ::filewatch::FileWatchStatus* response) override
      {
        response->set_status(filewatch::FileWatchStatus::OK);
        response->set_msg("up-and-running");
        return grpc::Status::OK;
      }

      ::grpc::Status GetVersion(::grpc::ServerContext* /*context*/,
                                const ::filewatch::Void* /*request*/,
                                ::filewatch::Version* response) override
      {
        response->set_major(1);
        response->set_minor(0);
        response->set_revision(0);
        return grpc::Status::OK;
      }
    };


    class MyDirEventListener : public fw::dm::DirectoryEventListener
    {
    public:
      explicit MyDirEventListener(
        ::grpc::ServerWriter<::filewatch::DirectoryEvent>* writer_) :
        writer(writer_)
      {
      }

      void notify(filewatch::DirectoryEvent::Event event,
                  std::string_view containing_dir,
                  std::string_view dir_name,
                  uint64_t mtime) override
      {
        filewatch::DirectoryEvent direvt;
        direvt.set_event(event);
        direvt.set_name(std::string(containing_dir));
        direvt.mutable_modification_time()->set_epoch(mtime);
        direvt.mutable_dirname()->set_name(std::string(dir_name));
        direvt.mutable_dirname()->mutable_modification_time()->set_epoch(mtime);
        writer->Write(direvt);
      }

    private:
      ::grpc::ServerWriter<::filewatch::DirectoryEvent>* writer;
    };

    class DirService : public filewatch::Directory::Service
    {
    public:
      explicit DirService(FileSystemFactory& factory_) : factory(factory_) {}

      ::grpc::Status ListFiles(::grpc::ServerContext* /*context*/,
                               const ::filewatch::Directoryname* request,
                               ::filewatch::FileList* response) override
      {
        auto dirview = factory.create_directory(request->name());
        return dirview->fill_file_list(*response);
      }

      ::grpc::Status ListDirectories(::grpc::ServerContext* /*context*/,
                                     const ::filewatch::Directoryname* request,
                                     ::filewatch::DirList* response) override
      {
        auto dirview = factory.create_directory(request->name());
        return dirview->fill_dir_list(*response);
      }

      ::grpc::Status ListenForEvents(
        ::grpc::ServerContext* context,
        const ::filewatch::Directoryname* request,
        ::grpc::ServerWriter<::filewatch::DirectoryEvent>* writer) override
      {
        auto dirview = factory.create_directory(request->name());
        MyDirEventListener listener(writer);
        dirview->register_event_listener(listener);
        while (!context->IsCancelled())
        {
          std::this_thread::yield();
        }
        dirview->unregister_event_listener(listener);
        return grpc::Status::OK;
      }

    private:
      FileSystemFactory& factory;
    };

    class FileService : public filewatch::File::Service
    {
    public:
      explicit FileService(FileSystemFactory& factory_) :
        factory(factory_)
      {}

      ::grpc::Status
      GetContents(::grpc::ServerContext* /*context*/,
                  const ::filewatch::Filename* request,
                  ::filewatch::FileContent* response) override
      {
        auto fileview = factory.create_file(request->dirname().name(),
                                            request->name());
        return fileview->fill_contents(*response);
      }

    private:
      FileSystemFactory& factory;
    };

  }  // anonymous namespace


  class Services
  {
  public:
    explicit Services(std::unique_ptr<FileSystemFactory> factory_) :
      factory(std::move(factory_)), Directory(*factory), File(*factory)
    {
    }

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
    if (SIGTERM == sig || SIGINT == sig)
    {
      if (SIGTERM == sig)
      {
        spdlog::warn("SIGTERM received");
      }
      else
      {
        spdlog::warn("SIGINT received");
      }

      if (g_server != nullptr)
      {
        spdlog::info("Stopping server.");
        g_server->stop();
      }

      std::signal(sig, signal_handler);
    }
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
    spdlog::info("server listening at localhost:45678");
    spdlog::info("server can be terminated with Ctrl-C");
    server->Wait();
    return 0;
  }

  void Server::stop()
  {
    spdlog::info("filewatch server daemon terminating");
    server->Shutdown();
  }

}  // namespace fw::dm
