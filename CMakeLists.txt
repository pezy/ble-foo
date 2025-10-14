cmake_minimum_required(VERSION 3.10)
project(ble VERSION 1.0.0 LANGUAGES CXX)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Platform detection - Linux ARM64 only
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
        set(TARGET_PLATFORM "linux-arm64")
    else()
        message(FATAL_ERROR "Unsupported processor: ${CMAKE_SYSTEM_PROCESSOR}. Only ARM64 is supported.")
    endif()
else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}. Only Linux is supported.")
endif()

message(STATUS "Building for ${TARGET_PLATFORM}")

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GIO REQUIRED gio-2.0)
pkg_check_modules(DBUS REQUIRED dbus-1)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GIO_CFLAGS_OTHER}")

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/src/include)

# Build shared library
add_library(ble SHARED
    src/lib/bluetooth/device_discovery.cpp
)

target_link_libraries(ble ${GIO_LIBRARIES} ${DBUS_LIBRARIES})

target_include_directories(ble PUBLIC ${GIO_INCLUDE_DIRS} ${DBUS_INCLUDE_DIRS})

# Set library properties
set_target_properties(ble PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "src/include/bluetooth/device_discovery.hpp"
)

# Build CLI executable
add_executable(ble_paired src/cli/ble_paired.cpp)
target_link_libraries(ble_paired ble)

# Installation
install(TARGETS ble DESTINATION lib)
install(TARGETS ble_paired DESTINATION bin)
install(FILES src/include/bluetooth/device_discovery.hpp DESTINATION include/bluetooth)

# Testing (optional)
option(BUILD_TESTING "Build tests" OFF)

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

# CPPLint
find_program(CPPLINT cpplint)

if(CPPLINT)
    set(ALL_CPP_SOURCES
        "${CMAKE_SOURCE_DIR}/src/cli/ble_paired.cpp"
        "${CMAKE_SOURCE_DIR}/src/include/bluetooth/device_discovery.hpp"
        "${CMAKE_SOURCE_DIR}/src/lib/bluetooth/device_discovery.cpp"
    )
    
    add_custom_target(cpplint
        COMMAND ${CPPLINT}
            --recursive
            ${CMAKE_SOURCE_DIR}/src
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running cpplint on source files"
    )
    
    add_custom_target(cpplint-fix
        COMMAND ${CPPLINT}
            --recursive
            --output=sed
            ${CMAKE_SOURCE_DIR}/src
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating cpplint fixes (run with: make cpplint-fix | sh)"
    )
    
    message(STATUS "cpplint found: ${CPPLINT}")
    message(STATUS "Use 'make cpplint' to run code style checks")
    message(STATUS "Use 'make cpplint-fix | sh' to apply automatic fixes")
else()
    message(STATUS "cpplint not found - code style checks disabled")
endif()

# clang-format (optional)
find_program(CLANG_FORMAT clang-format)

if(CLANG_FORMAT)
    set(ALL_SOURCE_FILES
        "${CMAKE_SOURCE_DIR}/src/cli/ble_paired.cpp"
        "${CMAKE_SOURCE_DIR}/src/include/bluetooth/device_discovery.hpp"
        "${CMAKE_SOURCE_DIR}/src/lib/bluetooth/device_discovery.cpp"
    )
    
    add_custom_target(format
        COMMAND ${CLANG_FORMAT} -i ${ALL_SOURCE_FILES}
        COMMENT "Formatting source code with clang-format"
    )
endif()
