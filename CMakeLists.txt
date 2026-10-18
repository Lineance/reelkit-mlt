cmake_minimum_required(VERSION 3.20)
project(editor_core VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MLTXX REQUIRED IMPORTED_TARGET mlt++-7)

add_library(editor_core STATIC
    src/types.cpp
    src/runtime.cpp
    src/profile.cpp
    src/track.cpp
    src/timeline.cpp
    src/render.cpp
    src/mlt_document.cpp
)
add_library(editor_core::editor_core ALIAS editor_core)
target_include_directories(editor_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(editor_core PUBLIC PkgConfig::MLTXX)
target_compile_options(editor_core PRIVATE -Wall -Wextra -Wpedantic)

install(TARGETS editor_core
    EXPORT EditorCoreTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/editor_core DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT EditorCoreTargets
    FILE EditorCoreTargets.cmake
    NAMESPACE editor_core::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/EditorCore
)
configure_package_config_file(
    cmake/EditorCoreConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/EditorCoreConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/EditorCore
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/EditorCoreConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/EditorCoreConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/EditorCoreConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/EditorCore
)

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

    add_executable(test_native_interop tests/test_native_interop.cpp)
    target_link_libraries(test_native_interop PRIVATE editor_core::editor_core)
    add_test(NAME test_native_interop
             COMMAND test_native_interop "${CMAKE_CURRENT_BINARY_DIR}/test-artifacts")
    set_tests_properties(test_native_interop PROPERTIES TIMEOUT 120 RUN_SERIAL TRUE)

    add_test(NAME test_install_consumer
        COMMAND ${CMAKE_COMMAND}
            -DEDITOR_CORE_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
            -DEDITOR_CORE_BUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -DTEST_ROOT=${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/package-consumer
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_package_consumer.cmake
    )
    set_tests_properties(test_install_consumer PROPERTIES TIMEOUT 120 RUN_SERIAL TRUE)
endif()
