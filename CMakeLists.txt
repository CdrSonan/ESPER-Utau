cmake_minimum_required(VERSION 3.12)
project(ESPER_Utau)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add the source code directory
add_subdirectory(src)

# Add the external library directory
add_subdirectory(libESPER)

# Create the executable
add_executable(ESPER_Utau main.cpp)

# Link the executable with the library
target_link_libraries(ESPER_Utau PRIVATE ESPER)