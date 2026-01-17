cmake_minimum_required (VERSION 2.8.11)
project (cllama)
set(CMAKE_C_STANDARD 23)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_C_FLAGS "-Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "-g -ggdb -fsanitize=address,undefined")
set(CMAKE_C_FLAGS_RELEASE "-O3")

add_library(cgguf SHARED cgguf.c)


add_executable(cllama cllama.c)
target_link_libraries(cllama cgguf)
