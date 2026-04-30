cmake_minimum_required(VERSION 3.18)
project(citation_network)

# C++
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
#

include_directories(.)

add_executable(citation_network
    main.cpp
    CitationNetwork.cpp
)

target_compile_options(citation_network PRIVATE
  $<$<COMPILE_LANGUAGE:CXX>:-O3 -g -std=c++20>
)
