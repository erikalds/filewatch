#ifndef SERVER_H_
#define SERVER_H_

/* Header created: 2019-03-24

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

#include <memory>

namespace grpc { class Server; }

namespace fw
{
  namespace dm
  {

    class FileSystemFactory;
    class Services;

    class Server
    {
    public:
      explicit Server(std::unique_ptr<FileSystemFactory> factory);
      ~Server();

      int run();
      void stop();

    private:
      std::unique_ptr<Services> services;
      std::unique_ptr<grpc::Server> server;
    };

  }  // dm
} // namespace fw

#endif // SERVER_H_
