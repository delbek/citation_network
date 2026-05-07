cmake_minimum_required(VERSION 3.18)
project(citation_network)

# C++
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
#

# OpenMP
find_package(OpenMP REQUIRED)
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -Xcompiler ${OpenMP_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
#

include_directories(.)

add_executable(citation_network
    main.cpp
    CitationNetwork.cpp
)

target_link_libraries(citation_network PRIVATE
    OpenMP::OpenMP_CXX
)

target_compile_options(citation_network PRIVATE
  $<$<COMPILE_LANGUAGE:CXX>:-O3 -g -std=c++20>
)
