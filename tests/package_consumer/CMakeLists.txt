cmake_minimum_required(VERSION 3.20)
project(editor_core_downstream_consumer LANGUAGES CXX)

find_package(EditorCore CONFIG REQUIRED)

add_executable(editor_core_consumer main.cpp)
target_link_libraries(editor_core_consumer PRIVATE editor_core::editor_core)
