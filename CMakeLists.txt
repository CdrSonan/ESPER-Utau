cmake_minimum_required(VERSION 3.5.0)
project(ESPER-Utau VERSION 0.1.0 LANGUAGES C CXX)

add_executable(ESPER-Utau
    src/main.cpp
    src/argparse.cpp)

include(CTest)
enable_testing()

# Set the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
