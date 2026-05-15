include(FetchContent)

macro(fetch_module name)
    FetchContent_Declare(${name} ${ARGN})
    FetchContent_MakeAvailable(${name})
endmacro()
