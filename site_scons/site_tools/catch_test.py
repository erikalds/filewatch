import os
from SCons.Script import Builder
from SCons.Errors import StopError


def exists(env):
    paths = set()
    for p in ('CPATH', 'CPPPATH', 'CXXPATH'):
        if p in env:
            paths.update(env[p])

    for p in paths:
        if env.Glob(os.path.join(p, 'catch.hpp')):
            return True

    print("Error: Catch header file not found: catch.hpp")
    return False


def generate_catch_test_run(target, source, env, for_signature):
    return "{ $SOURCE --run-unit-tests --use-colour no 2>&1 ; echo $$? > /tmp/${SOURCE.name}.status ; } | tee $TARGET && exit $$(cat /tmp/${SOURCE.name}.status)"


def generate(env):
    env.AppendUnique(CXXPATH=["/usr/include/catch"])
    if not exists(env):
        raise StopError("Catch header must be found in $CPPPATH/$CXXPATH/$CPATH")

    catch_test = Builder(generator=generate_catch_test_run)
    env.AppendUnique(BUILDERS=dict(CatchTest=catch_test))
