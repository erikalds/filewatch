import os
from SCons.Errors import StopError


def exists(env):
    if env.WhereIs('protoc', path=env['ENV']['PATH']):
        if env.WhereIs('grpc_cpp_plugin', path=env['ENV']['PATH']):
            conf = env.Configure()
            if not conf.CheckCXXHeader('grpc++/server.h'):
                return False

            env = conf.Finish()
            return True
        else:
            print("Error: 'grpc_cpp_plugin' not found")
    else:
        print("Error: 'protoc' not found")


def generate(env):
    if not exists(env):
        raise StopError("Cannot find grpc/protobuf compiler")

    env.SetDefault(PROTOGENDIR="generated")
