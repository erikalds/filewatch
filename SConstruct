import os

env = Environment()

env.AppendUnique(CXXFLAGS=['-std=c++17', '-Wall', '-pedantic'])
env.AppendUnique(CPPPATH=[os.path.join('#', 'src', 'rpc_defs', 'generated')])
env.AppendUnique(LIBPATH=[os.path.join('#', 'src', 'rpc_defs')])

rpclib = env.SConscript(os.path.join("src", "rpc_defs", "SConscript"),
                        exports=['env'])
daemon = env.SConscript(os.path.join("src", "daemon", "SConscript"),
                        exports=['env', 'rpclib'])
viewer = env.SConscript(os.path.join("src", "viewer", "SConscript"),
                        exports=['env', 'rpclib'])
