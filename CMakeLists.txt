cmake_minimum_required(VERSION 3.16)
project(TicketSystem)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_BUILD_TYPE Release)

include_directories("src/include")

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-generate=./pgo_profiles")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-use=../pgo_profiles -fprofile-correction")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast")

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
# set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address -fno-omit-frame-pointer")

add_executable(code src/main.cpp
        src/common/locks.cpp
        src/include/common/locks.h
        src/include/storage/disk/disk_manager.h
        src/include/common/config.h
        src/include/buffer/replacer.h
        src/include/buffer/buffer_pool_manager.h
        src/include/buffer/buffer_pool_proxy.h
        src/include/common/stl/pointers.hpp
        src/include/common/stl/pair.hpp
        src/include/common/stl/map.hpp
        src/include/common/stl/list.hpp
        src/include/common/stl/vector.hpp
        src/buffer/buffer_pool_proxy.cpp
        src/storage/disk/disk_manager.cpp
        src/storage/page/page_guard.cpp
        src/buffer/buffer_pool_manager.cpp
        src/include/storage/page/b_plus_tree_header_page.h
        src/include/storage/page/b_plus_tree_page.h
        src/include/storage/page/b_plus_tree_internal_page.h
        src/include/storage/page/b_plus_tree_leaf_page.h
        src/storage/page/b_plus_tree_internal_page.cpp
        src/storage/page/b_plus_tree_leaf_page.cpp
        src/include/storage/index/b_plus_tree.h
        src/include/storage/index/index_iterator.h
        src/storage/index/b_plus_tree.cpp
        src/buffer/replacer.cpp
        src/storage/index/index_iterator.cpp
        src/storage/page/b_plus_tree_page.cpp
        src/include/common/utils.h
        src/include/storage/page/tuple_page.h
        src/include/common/rid.h
        src/storage/page/tuple_page.cpp
        src/include/user/user_system.h
        src/user/user_system.cpp
        src/include/executor/executor.h
        src/executor/executor.cpp
        src/include/ticket/train_system.h
        src/include/common/time.h
        src/common/time.cpp
        src/ticket/train_system.cpp
        src/include/ticket/waitlist.h
        src/ticket/waitlist.cpp
        src/include/ticket/order_list.h
        src/ticket/order_list.cpp
        src/include/ticket/ticket_system.h
        src/ticket/ticket_system.cpp)