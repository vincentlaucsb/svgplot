cmake_minimum_required(VERSION 3.25)

project(svgplot_single_header_smoke LANGUAGES CXX)

set(SVGPLOT_SINGLE_INCLUDE_DIR "" CACHE PATH "Directory containing generated svgplot single_include tree")
if(NOT SVGPLOT_SINGLE_INCLUDE_DIR)
    message(FATAL_ERROR "SVGPLOT_SINGLE_INCLUDE_DIR is required")
endif()

add_library(smoke_one OBJECT smoke_one.cpp)
add_library(smoke_two OBJECT smoke_two.cpp)

target_compile_features(smoke_one PRIVATE cxx_std_23)
target_compile_features(smoke_two PRIVATE cxx_std_23)

target_include_directories(smoke_one PRIVATE "${SVGPLOT_SINGLE_INCLUDE_DIR}")
target_include_directories(smoke_two PRIVATE "${SVGPLOT_SINGLE_INCLUDE_DIR}")
