cmake_minimum_required(VERSION 3.20)
project(reelkit_mlt_consumer LANGUAGES CXX)

find_package(ReelKitMlt CONFIG REQUIRED)

add_executable(reelkit_mlt_consumer main.cpp)
target_link_libraries(reelkit_mlt_consumer PRIVATE reelkit::mlt)
