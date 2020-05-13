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

#include "common/tee_output.h"
#include "filesystem.h"
#include "filesystemwatcherfactory.h"
#include "server.h"

#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>
#include <docopt/docopt.h>
#include <spdlog/spdlog.h>

#include <vector>

static constexpr auto USAGE =
  R"(filewatch daemon.

Usage:
    fwdaemon [--log-level=LEVEL] DIR
    fwdaemon --run-unit-tests [--tee-output=FILE] [--use-colour=(auto|yes|no)] [--list-tests] [--log-level=LEVEL]
    fwdaemon (-h | --help)
    fwdaemon --version

Options:
    --run-unit-tests            Run the unit tests.
    --list-tests                List available unit tests.
    --tee-output=FILE           Put test output in FILE as well as stderr/stdout.
    --use-colour=(auto|yes|no)  Use colour in output.
    --log-level=LEVEL           Set log level (error|warn|info|debug).
                                Default: info.
    -h --help                   Show this screen.
    --version                   Show version.
)";

std::vector<const char*> filter_args(int argc, const char** argv);

int main(int argc, const char* argv[])
{
  std::map<std::string, docopt::value> args =
    docopt::docopt(USAGE,
                   {std::next(argv), std::next(argv, argc)},
                   true,  // show help if requested
                   "fwdaemon 0.1");  // version string

  auto log_level = args["--log-level"];
  if (log_level)
  {
    if (log_level.asString() == "debug")
    {
      spdlog::set_level(spdlog::level::debug);
    }
    else if (log_level.asString() == "error")
    {
      spdlog::set_level(spdlog::level::err);
    }
    else if (log_level.asString() == "info")
    {
      spdlog::set_level(spdlog::level::info);
    }
    else if (log_level.asString() == "warn")
    {
      spdlog::set_level(spdlog::level::warn);
    }
    else
    {
      spdlog::error("Unrecognized log level: {}", log_level.asString());
      return -2;
    }
  }
  try
  {
    if (args["--run-unit-tests"].asBool())
    {
      std::unique_ptr<logging::TeeOutput> tee;
      auto tee_output = args["--tee-output"];
      if (tee_output)
      {
        tee = std::make_unique<logging::TeeOutput>(tee_output.asString());
      }

      Catch::Session session;
      std::vector<const char*> rawargs = filter_args(argc, argv);
      auto return_code =
        session.applyCommandLine(static_cast<int>(rawargs.size()), &rawargs[0]);
      if (return_code != 0)
      {
        return return_code;
      }

      return session.run();
    }

    spdlog::info("filewatch 0.1 daemon monitoring {}", args["DIR"].asString());
    auto fs = fw::dm::create_filesystem(args["DIR"].asString());
    auto factory =
      std::make_unique<fw::dm::FileSystemWatcherFactory>(std::move(fs));

    fw::dm::Server server(std::move(factory));
    return server.run();
  }
  catch (const std::exception& e)
  {
    spdlog::error("Exception caught in main: {}", e.what());
  }
  catch (...)
  {
    spdlog::error("Unknown exception caught in main.");
  }
  return -1;
}


std::vector<const char*> filter_args(int argc, const char** argv)
{
  const std::string_view tee_output{"--tee-output="};
  const std::string_view log_level{"--log-level="};
  std::vector<const char*> rawargs{argv, std::next(argv, argc)};
  for (auto iter = rawargs.begin(); iter != rawargs.end();)
  {
    std::string_view a{*iter};
    if (a == "--run-unit-tests"
        || a.starts_with(tee_output)
        || a.starts_with(log_level))
    {
      iter = rawargs.erase(iter);
    }
    else if (a == tee_output.substr(0, tee_output.size() - 1)
             || a == log_level.substr(0, log_level.size() - 1))
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
