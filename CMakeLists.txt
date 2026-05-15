cmake_minimum_required(VERSION 3.25)

project(svgplot VERSION 0.1.0 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(FetchModules)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

fetch_module(
    svg
    GIT_REPOSITORY https://github.com/vincentlaucsb/svg.git
    GIT_TAG master
    SOURCE_SUBDIR cmake-do-not-build-upstream-targets
)

fetch_module(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1
)

add_library(svgplot INTERFACE)
target_include_directories(svgplot INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${svg_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(svgplot INTERFACE cxx_std_23)

add_executable(exercise_log_svg examples/exercise_log.cpp)
target_link_libraries(exercise_log_svg PRIVATE svgplot)

include(CTest)
if(BUILD_TESTING)
    add_executable(svgplot_tests
        tests/chart_tests.cpp
    )
    target_link_libraries(svgplot_tests PRIVATE svgplot Catch2::Catch2WithMain)

    include(Catch)
    catch_discover_tests(svgplot_tests)
endif()
