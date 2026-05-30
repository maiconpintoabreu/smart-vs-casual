# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Set the minimum required version of CMake
cmake_minimum_required(VERSION 3.30)

# Set standards for C and C++
set(CMAKE_C_STANDARD 99)

# Set the project name based on the name given on the gradle.properties
project("${APP_LIB_NAME}")

# Include raylib and raymob as a subdirectories
# set(RAYLIB_VERSION 5.5) # Change this to the version you want to use
set(RAYLIB_VERSION "Add-RL_POINTS")
include(FetchContent)
# check if RAYLIB_VERSION is a number
if(RAYLIB_VERSION MATCHES "^[0-9.]+$")
    find_package(raylib ${RAYLIB_VERSION} QUIET) # QUIET or REQUIRED

    if (NOT raylib_FOUND) # If there's none, fetch and build raylib
        FetchContent_Declare(
            raylib
            DOWNLOAD_EXTRACT_TIMESTAMP OFF
            URL https://github.com/maiconpintoabreu/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
        )
        FetchContent_GetProperties(raylib)
        if (NOT raylib_POPULATED) # Have we downloaded raylib yet?
            set(FETCHCONTENT_QUIET NO)
            FetchContent_MakeAvailable(raylib)
            set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
        endif()

    endif()
else()
    # download the latest version of raylib
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/maiconpintoabreu/raylib.git
        GIT_TAG ${RAYLIB_VERSION} 
    )
    FetchContent_GetProperties(raylib)
    set(FETCHCONTENT_QUIET NO)
    FetchContent_MakeAvailable(raylib)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
endif()    
add_subdirectory(${CMAKE_SOURCE_DIR}/deps/raymob)

# Fetch all source files for your project (recursively), excluding 'deps' source files
file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/src/*.c")

add_definitions(-DIS_ANDROID)
list(FILTER SOURCES EXCLUDE REGEX "${CMAKE_SOURCE_DIR}/deps/.*")

# Add headers directory for android_native_app_glue.c
include_directories(${ANDROID_NDK}/sources/android/native_app_glue/)

# Add android_native_app_glue.c to the source files
list(APPEND SOURCES ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

# Create a shared library with game source files
add_library(${APP_LIB_NAME} SHARED ${SOURCES})

# Define compiler macros for the library
target_compile_definitions(${APP_LIB_NAME} PRIVATE PLATFORM_ANDROID)

# Apply flags depending on the build type
if(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_definitions(${APP_LIB_NAME} PRIVATE _DEBUG DEBUG)
endif()

# Include raylib and raymob header files
target_include_directories(${APP_LIB_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/deps/raymob")

# Link required libraries to the native application
target_link_libraries(${APP_LIB_NAME} raylib raymoblib)
