Import('env')


env.ParseConfig('pkg-config --libs --cflags opencv')
env.ParseConfig('pkg-config --libs --cflags sdl')
env.ParseConfig('pkg-config --libs --cflags libfreenect')
env.ParseConfig('pkg-config --libs --cflags gl')

cflags = ["-O0", "-fPIC", "-g", "-Wall"]
                                    
env.Append( CPPFLAGS=cflags )

src_files = env.Glob("src/*.cpp")


objs = env.Object(src_files)


prog = env.Program("Kinecap", objs)
env.Install("../bin", prog)
