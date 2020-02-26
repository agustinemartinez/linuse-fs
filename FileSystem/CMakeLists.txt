cmake_minimum_required(VERSION 3.5)
project(FileSystem C)

set(CMAKE_C_STANDARD 99)

link_libraries(commons)
#link_libraries(explain)
link_libraries(pthread)

add_executable(FileSystem sac-fs/src/sac.c sac-fs/src/sac.h sac-fs/src/disk.c sac-fs/src/disk.h sac-fs/src/block.c sac-fs/src/block.h sac-fs/src/utils.c sac-fs/src/utils.h sac-fs/src/bitmap.c sac-fs/src/bitmap.h sac-fs/src/sacio.c sac-fs/src/sacio.h sac-fs/src/directory.c sac-fs/src/directory.h sac-fs/src/node.c sac-fs/src/node.h mtserver/src/server.c mtserver/src/server.h mtserver/src/serialization.c mtserver/src/serialization.h mtserver/src/fs_local.c mtserver/src/fs_local.h mtserver/src/log.c mtserver/src/log.h sac-fs/src/file.h sac-fs/src/file.c mtserver/src/fs_sac.c mtserver/src/fs_sac.h)

