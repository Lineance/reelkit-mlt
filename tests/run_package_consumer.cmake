if(NOT DEFINED REELKIT_MLT_SOURCE_DIR OR NOT DEFINED REELKIT_MLT_BUILD_DIR OR NOT DEFINED TEST_ROOT)
    message(FATAL_ERROR "Missing package consumer test paths")
endif()

set(prefix "${TEST_ROOT}/prefix")
set(consumer_build "${TEST_ROOT}/consumer-build")

execute_process(
    COMMAND "${CMAKE_COMMAND}" --install "${REELKIT_MLT_BUILD_DIR}" --prefix "${prefix}"
    RESULT_VARIABLE install_result
)
if(NOT install_result EQUAL 0)
    message(FATAL_ERROR "Installing reelkit_mlt failed: ${install_result}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -S "${REELKIT_MLT_SOURCE_DIR}/tests/package_consumer"
            -B "${consumer_build}" -DCMAKE_PREFIX_PATH=${prefix}
    RESULT_VARIABLE configure_result
)
if(NOT configure_result EQUAL 0)
    message(FATAL_ERROR "Configuring downstream consumer failed: ${configure_result}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${consumer_build}"
    RESULT_VARIABLE build_result
)
if(NOT build_result EQUAL 0)
    message(FATAL_ERROR "Building downstream consumer failed: ${build_result}")
endif()

execute_process(
    COMMAND "${consumer_build}/reelkit_mlt_consumer"
    RESULT_VARIABLE run_result
)
if(NOT run_result EQUAL 0)
    message(FATAL_ERROR "Running downstream consumer failed: ${run_result}")
endif()
