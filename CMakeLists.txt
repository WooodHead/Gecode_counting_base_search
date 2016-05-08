cmake_minimum_required(VERSION 2.8)
project(Gecode_couting_base_search)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
find_package(Gecode REQUIRED)
include_directories(${Gecode_INCLUDE_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/build/Release)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR}/build/Debug)

# Sources files
set(SOURCE_FILES CBSBrancher.cpp AllDiffCBS.cpp CBSPosValChoice.hpp CBSConstraint.hpp ValToVar.cpp utils.hpp)

# Problems
set(DUMMY_PROBLEM problems/DummyProblem.cpp ${SOURCE_FILES})
add_executable(DummyProblem ${DUMMY_PROBLEM})
target_link_libraries(DummyProblem ${Gecode_LIBRARIES})

set(SUDOKU problems/Sudoku.cpp ${SOURCE_FILES})
add_executable(Sudoku ${SUDOKU})
target_link_libraries(Sudoku ${Gecode_LIBRARIES})