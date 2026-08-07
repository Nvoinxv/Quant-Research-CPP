cmake_minimum_required(VERSION 3.14)
project(QuantResearch LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include headers
include_directories(${CMAKE_SOURCE_DIR}/headers)

# Gather all .cpp files automatically
file(GLOB_RECURSE SOURCES 
    ${CMAKE_SOURCE_DIR}/main.cpp
    ${CMAKE_SOURCE_DIR}/src/*.cpp
)

# Executable
add_executable(quant_research ${SOURCES})

# Optional: Enable strict warnings for GCC/Clang
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(quant_research PRIVATE -Wall -Wextra -Wpedantic -O2)
endif()