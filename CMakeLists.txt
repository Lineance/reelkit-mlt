cmake_minimum_required(VERSION 3.20)
project(editor_core_mlt_poc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(MLTXX REQUIRED IMPORTED_TARGET mlt++-7)

add_executable(mlt_render_poc src/render_poc.cpp)
target_link_libraries(mlt_render_poc PRIVATE PkgConfig::MLTXX)

include(CTest)
if(BUILD_TESTING)
    function(add_mlt_test target source)
        add_executable(${target} ${source})
        target_link_libraries(${target} PRIVATE PkgConfig::MLTXX)
        add_test(NAME ${target}
                 COMMAND ${target} "${CMAKE_CURRENT_BINARY_DIR}/test-artifacts")
        set_tests_properties(${target} PROPERTIES TIMEOUT 120 RUN_SERIAL TRUE)
    endfunction()

    add_mlt_test(test_timeline tests/test_timeline.cpp)
    add_mlt_test(test_audio tests/test_audio.cpp)
    add_mlt_test(test_text_keyframes tests/test_text_keyframes.cpp)
    add_mlt_test(test_alpha_overlay tests/test_alpha_overlay.cpp)
endif()
