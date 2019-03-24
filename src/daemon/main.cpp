#include "server.h"

#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include <vector>

int main(int argc, const char* argv[])
{
  auto run_unit_tests = false;
  std::vector<const char*> args(argv, argv + argc);
  for (auto iter = args.begin(); iter != args.end();)
  {
    if (std::string_view(*iter) == "--run-unit-tests")
    {
      iter = args.erase(iter);
      run_unit_tests = true;
    }
    else
    {
      ++iter;
    }
  }

  if (run_unit_tests)
  {
    Catch::Session session;
    auto return_code = session.applyCommandLine(args.size(), &args[0]);
    if (return_code != 0)
      return return_code;

    return session.run();
  }

  fw::dm::Server server;
  return server.run();
}
