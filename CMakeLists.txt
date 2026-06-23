cmake_minimum_required(VERSION 3.15)
project(rv_sim CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(rv-sim
    src/main.cpp
    src/cpu.cpp
    src/disasm.cpp
)

if (MSVC)
    target_compile_options(rv-sim PRIVATE /W4)
else()
    target_compile_options(rv-sim PRIVATE -Wall -Wextra -O2)
endif()
