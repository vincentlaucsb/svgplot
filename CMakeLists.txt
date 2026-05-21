cmake_minimum_required(VERSION 3.25)

project(svgplot VERSION 0.2.0 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(FetchModules)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SVGPLOT_SIBLING_SVG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../svg" CACHE PATH
    "Path to a sibling svg checkout used instead of FetchContent when available")
if(EXISTS "${SVGPLOT_SIBLING_SVG_DIR}/src/svg.hpp")
    message(STATUS "Using sibling svg checkout: ${SVGPLOT_SIBLING_SVG_DIR}")
    set(svg_SOURCE_DIR "${SVGPLOT_SIBLING_SVG_DIR}")
else()
    fetch_module(
        svg
        GIT_REPOSITORY https://github.com/vincentlaucsb/svg.git
        GIT_TAG master
        SOURCE_SUBDIR cmake-do-not-build-upstream-targets
    )
endif()

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

add_executable(training_heatmap_svg examples/training_heatmap.cpp)
target_link_libraries(training_heatmap_svg PRIVATE svgplot)

add_executable(multi_series_line_svg examples/multi_series_line.cpp)
target_link_libraries(multi_series_line_svg PRIVATE svgplot)

include(CTest)
if(BUILD_TESTING)
    add_executable(svgplot_tests
        tests/chart_tests.cpp
    )
    target_link_libraries(svgplot_tests PRIVATE svgplot Catch2::Catch2WithMain)

    include(Catch)
    catch_discover_tests(svgplot_tests)
endif()
