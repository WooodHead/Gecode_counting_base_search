cmake_minimum_required(VERSION 2.8)
project(CBS)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
find_package(Gecode REQUIRED)
include_directories(${Gecode_INCLUDE_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build)

set(SOURCE_FILES main.cpp CBSBrancher.cpp AllDiffCBS.cpp CBSPosValChoice.hpp CBSConstraint.hpp)

add_executable(CBS ${SOURCE_FILES})
target_link_libraries(CBS ${Gecode_LIBRARIES})
