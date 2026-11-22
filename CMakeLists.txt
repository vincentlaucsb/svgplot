cmake_minimum_required(VERSION 3.25)

project(svgplot VERSION 1.0.0 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(FetchModules)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SVGPLOT_ENABLE_COVERAGE "Enable coverage instrumentation for svgplot tests" OFF)

set(_svg_default "${CMAKE_CURRENT_SOURCE_DIR}/third_party/svg")
if(DEFINED SVGPLOT_SIBLING_SVG_DIR)
    # Migrate a custom checkout, but do not reactivate an old cache's automatic sibling lookup.
    get_filename_component(_svg_legacy "${SVGPLOT_SIBLING_SVG_DIR}" ABSOLUTE)
    get_filename_component(_svg_old_default "${CMAKE_CURRENT_SOURCE_DIR}/../svg" ABSOLUTE)
    if(NOT _svg_legacy STREQUAL _svg_old_default AND EXISTS "${_svg_legacy}/src/svg.hpp")
        set(_svg_default "${_svg_legacy}/src")
    endif()
    message(DEPRECATION "SVGPLOT_SIBLING_SVG_DIR is obsolete. For an external checkout set SVGPLOT_SVG_INCLUDE_DIR to its src directory.")
endif()
set(SVGPLOT_SVG_INCLUDE_DIR "${_svg_default}" CACHE PATH "Directory containing the svg.hpp backend")
if(NOT EXISTS "${SVGPLOT_SVG_INCLUDE_DIR}/svg.hpp")
    message(FATAL_ERROR "Missing svg.hpp in SVGPLOT_SVG_INCLUDE_DIR=${SVGPLOT_SVG_INCLUDE_DIR}")
endif()
message(STATUS "svgplot backend: ${SVGPLOT_SVG_INCLUDE_DIR}/svg.hpp")

fetch_module(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1
)

add_library(svgplot INTERFACE)
target_include_directories(svgplot INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${SVGPLOT_SVG_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(svgplot INTERFACE cxx_std_23)

add_executable(exercise_log_svg examples/exercise_log.cpp)
target_link_libraries(exercise_log_svg PRIVATE svgplot)

add_executable(training_heatmap_svg examples/training_heatmap.cpp)
target_link_libraries(training_heatmap_svg PRIVATE svgplot)

add_executable(multi_series_line_svg examples/multi_series_line.cpp)
target_link_libraries(multi_series_line_svg PRIVATE svgplot)

add_executable(stacked_bar_svg examples/stacked_bar.cpp)
target_link_libraries(stacked_bar_svg PRIVATE svgplot)

add_executable(monthly_bar_svg examples/monthly_bar.cpp)
target_link_libraries(monthly_bar_svg PRIVATE svgplot)

add_executable(nice_axis_bar_svg examples/nice_axis_bar.cpp)
target_link_libraries(nice_axis_bar_svg PRIVATE svgplot)

include(CTest)
if(BUILD_TESTING)
    add_executable(svgplot_tests
        tests/chart_tests.cpp
    )
    target_link_libraries(svgplot_tests PRIVATE svgplot Catch2::Catch2WithMain)
    if(SVGPLOT_ENABLE_COVERAGE)
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(svgplot_tests PRIVATE --coverage -O0 -g)
            target_link_options(svgplot_tests PRIVATE --coverage)
        else()
            message(WARNING "SVGPLOT_ENABLE_COVERAGE is only configured for GNU-like compilers")
        endif()
    endif()

    include(Catch)
    catch_discover_tests(svgplot_tests)
endif()
