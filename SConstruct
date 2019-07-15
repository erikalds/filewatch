import os

env = Environment()

env.AppendUnique(CXXFLAGS=['-std=c++17', '-Wall', '-pedantic', '-O0', '-ggdb'])
env.AppendUnique(CPPPATH=[os.path.join('#', 'src', 'rpc_defs', 'generated'),
                          os.path.join(os.path.sep, 'usr', 'local', 'include')])
env.AppendUnique(LIBPATH=[os.path.join('#', 'src', 'rpc_defs'),
                          os.path.join(os.path.sep, 'usr', 'local', 'lib')])

common = env.SConscript(os.path.join("src", "common", "SConscript"),
                        exports=['env'])
rpclib = env.SConscript(os.path.join("src", "rpc_defs", "SConscript"),
                        exports=['env'])
daemon = env.SConscript(os.path.join("src", "daemon", "SConscript"),
                        exports=['env', 'common', 'rpclib'])
viewer = env.SConscript(os.path.join("src", "viewer", "SConscript"),
                        exports=['env', 'common', 'rpclib'])

daemon_test_report = env.SConscript(os.path.join("tests", "daemon",
                                                 "SConscript"),
                                    exports=['env', 'daemon'])
