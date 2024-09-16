cmake_minimum_required(VERSION 3.26)

project(LearnOpenGL CXX)

add_library(nrend_project_compile_options INTERFACE)
target_compile_features(nrend_project_compile_options INTERFACE cxx_std_20)

set(NREND_PROJECT_ROOT ${PROJECT_SOURCE_DIR})
set(NREND_SRC_DIR ${NREND_PROJECT_ROOT}/src)

SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

include(cmake/includes.cmake)



add_subdirectory(3rd_party)

add_subdirectory(${NREND_SRC_DIR})

add_custom_target(nrend_copy_resources ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/resources)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/resources
                   COMMAND ${CMAKE_COMMAND} -E create_symlink ${NREND_PROJECT_ROOT}/resources
                                                    ${CMAKE_CURRENT_BINARY_DIR}/resources
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/resources)
