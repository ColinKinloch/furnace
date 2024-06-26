set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(TARGET_PREFIX aarch64-linux-gnu)

set(CMAKE_C_COMPILER ${TARGET_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${TARGET_PREFIX}-g++)
set(PKG_CONFIG_EXECUTABLE ${TARGET_PREFIX}-pkg-config)

set(CMAKE_FIND_ROOT_PATH /usr/${TARGET_PREFIX} /usr/lib/${TARGET_PREFIX})
set(CMAKE_LIBRARY_ARCHITECTURE ${TARGET_PREFIX})

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
