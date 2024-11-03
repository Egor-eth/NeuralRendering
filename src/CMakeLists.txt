enable_language(CXX)
enable_language(C)

add_library(nrend_app_compile_options INTERFACE)
target_link_libraries(nrend_app_compile_options INTERFACE nrend_project_compile_options)
target_compile_options(nrend_app_compile_options INTERFACE -m64)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(nrend_app_compile_options INTERFACE -O0 -g)
else()
    target_compile_options(nrend_app_compile_options INTERFACE -O1)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    target_compile_options(nrend_app_compile_options INTERFACE -g)
endif()

include(properties.cmake)

add_executable(${MODULE_NAME}
    ${MODULE_SOURCES}
)

target_compile_options(${MODULE_NAME} PRIVATE -Wno-error=unused-variable)

target_link_libraries(${MODULE_NAME}
    ${MODULE_LIBS} nrend_app_compile_options 
)
