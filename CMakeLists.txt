#debug:
#system("chcp 65001");
#setbuf(stdout, 0);


cmake_minimum_required(VERSION 3.20)
project(progC CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXE_LINKER_FLAGS "-static")

# https://nuwen.net/mingw.html#install

set(Boost_INCLUDE_DIR C:/mingw/include/)
set(Boost_LIBRARY_DIR C:/mingw/lib/)
find_package(Boost COMPONENTS serialization json REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

file(GLOB_RECURSE progC_SRC CONFIGURE_DEPENDS "src/*")
add_executable(progC ${progC_SRC})

target_link_libraries(progC ${Boost_LIBRARIES})