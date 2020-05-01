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
#include <catch2/catch.hpp>

#include <docopt/docopt.h>

#include <vector>

static constexpr auto USAGE =
  R"(filewatcher daemon.

Usage:
    fwdaemon DIR
    fwdaemon --run-unit-tests [--tee-output=FILE] [--use-colour=(auto|yes|no)]
    fwdaemon (-h | --help)
    fwdaemon --version

Options:
    --run-unit-tests            Run the unit tests.
    --tee-output=FILE           Put test output in FILE as well as stderr/stdout.
    --use-colour=(auto|yes|no)  Use colour in output.
    -h --help                   Show this screen.
    --version                   Show version.
)";

std::vector<const char*> filter_args(int argc, const char** argv);

int main(int argc, const char* argv[])
{
  std::map<std::string, docopt::value> args
    = docopt::docopt(USAGE,
                     { std::next(argv), std::next(argv, argc) },
                     true,  // show help if requested
                     "fwdaemon 0.1");  // version string

  if (args["--run-unit-tests"].asBool() == true)
  {
    std::unique_ptr<logging::TeeOutput> tee;
    auto tee_output = args["--tee-output"];
    if (tee_output)
      tee = std::make_unique<logging::TeeOutput>(tee_output.asString());

    Catch::Session session;
    std::vector<const char*> rawargs = filter_args(argc, argv);
    auto return_code = session.applyCommandLine(static_cast<int>(rawargs.size()),
                                                &rawargs[0]);
    if (return_code != 0)
      return return_code;

    return session.run();
  }

  auto fs = std::make_unique<fw::dm::DefaultFileSystem>(args["DIR"].asString());
  auto factory = std::make_unique<fw::dm::FileSystemWatcherFactory>(std::move(fs));

  fw::dm::Server server(std::move(factory));
  return server.run();
}


std::vector<const char*> filter_args(int argc, const char** argv)
{
  std::vector<const char*> rawargs(argv, argv + argc);
  for (auto iter = rawargs.begin(); iter != rawargs.end();)
  {
    if (std::string_view(*iter) == "--run-unit-tests")
    {
      iter = rawargs.erase(iter);
    }
    else if (std::string_view(*iter) == "--tee-output")
    {
      iter = rawargs.erase(iter);
      iter = rawargs.erase(iter);
    }
    else
    {
      ++iter;
    }
  }
  return rawargs;
}
