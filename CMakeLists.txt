cmake_minimum_required(VERSION 3.20)
project(editor_core_mlt_poc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(MLTXX REQUIRED IMPORTED_TARGET mlt++-7)

add_executable(mlt_render_poc src/render_poc.cpp)
target_link_libraries(mlt_render_poc PRIVATE PkgConfig::MLTXX)
