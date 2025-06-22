cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(GameSiHaiGuy)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找系统安装的libwebsockets库
# 必须显式包含 FindPkgConfig 模块
include(FindPkgConfig)
pkg_check_modules(LIBWEBSOCKETS REQUIRED libwebsockets)

# 添加可执行文件
add_executable(${PROJECT_NAME}
    main.cpp
    GameInstance.cpp
    GameInstance.h
    Player.cpp
    Player.h
    WSServer.cpp
    WSServer.h
)

# 包含头文件目录
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LIBWEBSOCKETS_INCLUDE_DIRS}
)

# 链接库
target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBWEBSOCKETS_LIBRARIES})

# 可选：输出调试信息
message(STATUS "Libwebsockets include dirs: ${LIBWEBSOCKETS_INCLUDE_DIRS}")
message(STATUS "Libwebsockets libraries: ${LIBWEBSOCKETS_LIBRARIES}")
