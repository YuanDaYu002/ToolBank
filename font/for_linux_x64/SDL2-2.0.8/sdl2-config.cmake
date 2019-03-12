# sdl2 cmake project-config input for ./configure scripts

set(prefix "/home/yuanda.yu/git/font/for_linux_x64/SDL2-2.0.8/out") 
set(exec_prefix "${prefix}")
set(libdir "${exec_prefix}/lib")
set(SDL2_PREFIX "/home/yuanda.yu/git/font/for_linux_x64/SDL2-2.0.8/out")
set(SDL2_EXEC_PREFIX "/home/yuanda.yu/git/font/for_linux_x64/SDL2-2.0.8/out")
set(SDL2_LIBDIR "${exec_prefix}/lib")
set(SDL2_INCLUDE_DIRS "${prefix}/include/SDL2")
set(SDL2_LIBRARIES "-L${SDL2_LIBDIR} -Wl,-rpath,${libdir} -Wl,--enable-new-dtags -lSDL2 ")
string(STRIP "${SDL2_LIBRARIES}" SDL2_LIBRARIES)
