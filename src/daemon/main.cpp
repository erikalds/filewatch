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
#include "defaultfilesystem.h"
#include "filesystemwatcherfactory.h"
#include "common/tee_output.h"

#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include <vector>

int main(int argc, const char* argv[])
{
  auto run_unit_tests = false;
  std::vector<const char*> args(argv, argv + argc);
  std::string tee_output_file;
  for (auto iter = args.begin(); iter != args.end();)
  {
    if (std::string_view(*iter) == "--run-unit-tests")
    {
      iter = args.erase(iter);
      run_unit_tests = true;
    }
    else if (std::string_view(*iter) == "--tee-output")
    {
      iter = args.erase(iter);
      tee_output_file = *iter;
      iter = args.erase(iter);
    }
    else
    {
      ++iter;
    }
  }

  if (run_unit_tests)
  {
    std::unique_ptr<logging::TeeOutput> tee;
    if (!tee_output_file.empty())
      tee = std::make_unique<logging::TeeOutput>(tee_output_file);

    Catch::Session session;
    auto return_code = session.applyCommandLine(args.size(), &args[0]);
    if (return_code != 0)
      return return_code;

    return session.run();
  }

  if (args.size() < 2)
  {
    std::cerr << "usage: " << args[0] << " <root-dir-to-watch>\n";
    return -1;
  }

  auto fs = std::make_unique<fw::dm::DefaultFileSystem>(args[1]);
  auto factory = std::make_unique<fw::dm::FileSystemWatcherFactory>(std::move(fs));

  fw::dm::Server server(std::move(factory));
  return server.run();
}
