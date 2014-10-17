#win32 platform

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_C_COMPILER /usr/bin/i586-mingw32msvc-gcc)
set(CMAKE_CXX_COMPILER /usr/bin/i586-mingw32msvc-g++)
set(CMAKE_RC_COMPILER i586-mingw32msvc-windres)
set(CMAKE_RANLIB i586-mingw32msvc-ranlib)
set(CMAKE_LD i586-mingw32msvc-ld)
#set(CMAKE_AR i586-mingw32msvc-ar)
set(CMAKE_SYSTEM_PROCESSOR intel)
set(CMAKE_FIND_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)

