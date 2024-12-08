cmake_minimum_required(VERSION 3.5.0)
project(ESPER-Utau VERSION 0.1.0 LANGUAGES C CXX)

add_executable(ESPER-Utau
    src/main.cpp
    src/argparse.cpp
    src/fileio.cpp
    src/create-esper-structs.cpp
    src/esper-utils.cpp
    ESPER/esper.h
    AudioFile/AudioFile.h)

include_directories(${CMAKE_SOURCE_DIR}/ESPER)
include_directories(${CMAKE_SOURCE_DIR}/AudioFile)
if (WIN32)
    target_link_libraries(ESPER-Utau ${CMAKE_SOURCE_DIR}/ESPER/esper.lib)
else()
    target_link_libraries(ESPER-Utau PRIVATE m)
    target_link_libraries(ESPER-Utau PRIVATE ${CMAKE_SOURCE_DIR}/ESPER/libesper.so)
endif(WIN32)

include(CTest)
enable_testing()

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

# Ensure the standard is applied to the target
target_compile_features(ESPER-Utau PUBLIC cxx_std_17)
