cmake_minimum_required(VERSION 3.5.0)
project(ESPER-Utau VERSION 0.1.0 LANGUAGES C CXX)

add_executable(ESPER-Utau
    src/main.cpp
    src/argparse.cpp
    src/fileio.cpp
    src/create-esper-structs.cpp
    src/esper-utils.cpp)

add_subdirectory(libESPER)

include_directories(${PROJECT_SOURCE_DIR}/libESPER)
include_directories(${PROJECT_SOURCE_DIR}/AudioFile)
if (WIN32)
    target_link_libraries(ESPER-Utau esper)
    set_target_properties(ESPER-Utau PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=${CMAKE_BINARY_DIR}/libESPER/Debug;${CMAKE_BINARY_DIR}/libESPER/Release;${PROJECT_SOURCE_DIR}/libESPER/fftw_win;${PROJECT_SOURCE_DIR}/libESPER/nfft_win;%PATH%")
else()
    target_link_libraries(ESPER-Utau PRIVATE m)
    target_link_libraries(ESPER-Utau PRIVATE ${PROJECT_SOURCE_DIR}/build/libesper.so)
    set_target_properties(ESPER-Utau PROPERTIES
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "$ORIGIN/")
endif(WIN32)

include(CTest)
enable_testing()

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

# Ensure the standard is applied to the target
target_compile_features(ESPER-Utau PUBLIC cxx_std_17)
