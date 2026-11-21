cmake_minimum_required(VERSION 3.25)
project(svgplot_docs_examples LANGUAGES CXX)

add_subdirectory(../.. svgplot EXCLUDE_FROM_ALL)

foreach(example IN ITEMS first-line station-readings monthly-samples sampling-calendar)
    add_executable(${example} ${example}.cpp)
    target_compile_features(${example} PRIVATE cxx_std_23)
    target_link_libraries(${example} PRIVATE svgplot)
    if(MSVC)
        target_compile_options(${example} PRIVATE /utf-8 /bigobj)
    endif()
endforeach()
