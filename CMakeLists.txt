cmake_minimum_required(VERSION 3.29)
project(TicketSystem)

set(CMAKE_CXX_STANDARD 23)

include_directories("src/include")

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

FetchContent_MakeAvailable(googletest)

add_executable(TicketSystem src/main.cpp
        src/common/locks.cpp
        src/include/common/locks.h
        src/include/storage/disk/disk_manager.h
        src/include/common/config.h
        src/include/buffer/replacer.h
        src/include/buffer/buffer_pool_manager.h
        src/include/buffer/buffer_pool_proxy.h)
