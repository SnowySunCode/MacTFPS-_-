cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")

project(FPSBooster VERSION 1.0.0)

# ---------- Geode SDK ----------
if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "GEODE_SDK environment variable is not set. Please set it to the path of your Geode SDK.")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# ---------- Sources ----------
file(GLOB_RECURSE SOURCES src/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})

# ---------- macOS: link CGL for V-Sync control ----------
if (APPLE)
    find_library(OPENGL_LIBRARY OpenGL REQUIRED)
    find_library(COREGRAPHICS_LIBRARY CoreGraphics REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${OPENGL_LIBRARY}
        ${COREGRAPHICS_LIBRARY}
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE GEODE_IS_MACOS=1)
endif()

# ---------- Geode setup ----------
setup_geode_mod(${PROJECT_NAME})