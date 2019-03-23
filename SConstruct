import os

env = Environment()

env.AppendUnique(CXXFLAGS=['-std=c++17', '-Wall', '-pedantic'])

daemon = env.SConscript(os.path.join("src", "daemon", "SConscript"),
                        exports=['env'])
viewer = env.SConscript(os.path.join("src", "viewer", "SConscript"),
                        exports=['env'])
