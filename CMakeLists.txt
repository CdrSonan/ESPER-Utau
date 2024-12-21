cmake_minimum_required(VERSION 3.5.0)
project(ESPER-Utau VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
enable_testing()

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/bundled")

find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
# Update submodules as needed
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if(GIT_SUBMODULE)
        message(STATUS "Submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init --recursive failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()

add_executable(ESPER-Utau
    src/main.cpp
    src/argparse.cpp
    src/fileio.cpp
    src/create-esper-structs.cpp
    src/esper-utils.cpp)

add_subdirectory(libESPER)

include_directories(${PROJECT_SOURCE_DIR}/libESPER)
include_directories(${PROJECT_SOURCE_DIR}/AudioFile)
target_link_libraries(ESPER-Utau PRIVATE esper)
if (WIN32)
    set_target_properties(ESPER-Utau PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=${CMAKE_BINARY_DIR}/libESPER/Debug;${CMAKE_BINARY_DIR}/libESPER/Release;${PROJECT_SOURCE_DIR}/libESPER/fftw_win;${PROJECT_SOURCE_DIR}/libESPER/nfft_win;%PATH%")
    add_custom_command(TARGET ESPER-Utau POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:ESPER-Utau> "${CMAKE_BINARY_DIR}/bundled/ESPER-Utau.exe"
    )
else()
    target_link_libraries(ESPER-Utau PRIVATE m)
    #target_link_libraries(ESPER-Utau PRIVATE ${PROJECT_SOURCE_DIR}/build/libesper.so)
    set_target_properties(ESPER-Utau PROPERTIES
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "$ORIGIN/")
endif(WIN32)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

# Ensure the standard is applied to the target
target_compile_features(ESPER-Utau PUBLIC cxx_std_17)
