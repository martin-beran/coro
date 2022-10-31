# CMake prologue
cmake_minimum_required(VERSION 3.22.1)

project(
    Coro
    LANGUAGES CXX
)

include(CTest)

# Known build types
set(
    BUILD_TYPES
    Debug
    Release
    RelWithDebInfo
    MinSizeRel
)

# Known sanitizers
set(
    SANITIZERS 
    OFF
    address
    leak
    thread
    undefined
)

### BEGIN Build configuration #################################################

set(
    BUILD_TYPE "RelWithDebInfo" CACHE STRING
    "Select a build type (${BUILD_TYPES})"
)

option(TEMPORARY_DEBUG "Enable use of coro::DEBUG class" OFF)

set(
    SANITIZER "OFF" CACHE STRING
    "Select a sanitizer (${SANITIZERS})"
)

### END Build configuration ###################################################

# Checking build configuration
set_property(
    CACHE BUILD_TYPE PROPERTY STRINGS ${BUILD_TYPES}
)
if (NOT BUILD_TYPE IN_LIST BUILD_TYPES)
    message(
        FATAL_ERROR "Unknown value of BUILD_TYPE: ${BUILD_TYPE}"
    )
endif()
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE ${BUILD_TYPE})
endif()

set_property(
    CACHE SANITIZER PROPERTY STRINGS ${SANITIZERS}
)
if (NOT SANITIZER IN_LIST SANITIZERS)
    message(FATAL_ERROR "Unknown value of SANITIZER: ${SANITIZER}")
endif()

# C++ options
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_options(
    -Wall -Wextra -pedantic -Werror
    -Wconversion -Wsign-conversion
    -Wimplicit-fallthrough=5
    -Wswitch-default -Wswitch-enum
)
include_directories(${CMAKE_SOURCE_DIR}/src ${CMAKE_BINARY_DIR}/src)
if (TEMPORARY_DEBUG)
    add_compile_definitions(TEMPORARY_DEBUG)
endif()
if (NOT SANITIZER STREQUAL "OFF")
    add_compile_options(
        -fsanitize=${SANITIZER}
    )
    add_link_options(
        -fsanitize=${SANITIZER}
    )
endif()

# We really want asserts in all builds
add_compile_options(-UNDEBUG)

# Used packages
find_package(Boost 1.74 REQUIRED OPTIONAL_COMPONENTS unit_test_framework)
find_package(Threads REQUIRED)

include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})
if (NOT Boost_unit_test_framework_FOUND)
    set(BUILD_TESTING OFF)
endif()

# Optional Markdown utility for generating the top-level documentation stub
find_program(
    MARKDOWN
    NAMES markdown markdown_py
    DOC "Program for processing Markdown to HTML"
)

# Subdirectories with CMake configurations
if (BUILD_TESTING)
    add_subdirectory(test)
endif()

# Generate documentation, optional (Markdown and Doxygen)
set(DOC "")

if (MARKDOWN)
    add_custom_command(
        OUTPUT README.html
        COMMAND ${MARKDOWN} ${CMAKE_CURRENT_SOURCE_DIR}/README.md > README.html
        MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/README.md
    )
    add_custom_command(
        OUTPUT LICENSE.html
        COMMAND
            ${MARKDOWN} ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md > LICENSE.html
        MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md
    )
    list(APPEND DOC README.html LICENSE.html)
endif()

# Location of Doxygen is not customizable, hence we cannot use
# find_package(Doxygen) to find a newer locally installed version in
# a non-standard directory.
if (DOXYGEN_EXECUTABLE)
    execute_process(
        COMMAND ${DOXYGEN_EXECUTABLE} --version
        RESULT_VARIABLE DOXYGEN_FOUND
        OUTPUT_VARIABLE DOXYGEN_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (DOXYGEN_FOUND EQUAL "0")
        set(DOXYGEN_FOUND "YES")
    else()
        set(DOXYGEN_FOUND "NO")
    endif()
else()
    find_package(Doxygen)
endif()
if (DOXYGEN_FOUND)
    if (DOXYGEN_VERSION VERSION_GREATER_EQUAL 1.9.3)
        add_custom_target(
            doxygen
            COMMAND
                ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Doxyfile > /dev/null
        )
    else()
        set(DOXYGEN_FOUND "NO")
        message(STATUS "Doxygen too old, missing C++20 support")
    endif()
endif()

add_custom_target(
    doc ALL DEPENDS ${DOC}
)

set(CLEAN_TIDY_CMD "")
set(CLANG_TIDY_CMD "")
if (CLANG_TIDY)
    set(CLEAN_TIDY_CMD COMMAND ${CMAKE_COMMAND} --build . -t clean-tidy)
    set(
        CLANG_TIDY_CMD
        COMMAND ${CMAKE_COMMAND} --build . -t clang-tidy -j `nproc`
    )
endif()

add_custom_target(
    full
    COMMAND ${CMAKE_COMMAND} --build . -t clean
    COMMAND ${CMAKE_COMMAND} --build . -t all -j `nproc`
    COMMAND ${CMAKE_COMMAND} --build . -t doxygen
    ${CLEAN_TIDY_CMD}
    ${CLANG_TIDY_CMD}
    COMMAND ${CMAKE_COMMAND} --build . -t test -j `nproc`
)

# Report CMake configuration
message(STATUS "Version:     ${GIT_DESCRIBE}")
message(STATUS "Config file: ${CONFIG_FILE_HPP}")
message(STATUS "Build type:  ${CMAKE_BUILD_TYPE}")
message(STATUS "Libraries:   ${LIBS_TYPE}")
message(STATUS "Doxygen:     ${DOXYGEN_FOUND} ${DOXYGEN_VERSION}")
message(STATUS "Markdown:    ${MARKDOWN}")
message(STATUS "Clang-tidy:  ${CLANG_TIDY}")
message(STATUS "Build tests: ${BUILD_TESTING}")
message(STATUS "DEBUG:       ${TEMPORARY_DEBUG}")
message(STATUS "Sanitizer:   ${SANITIZER}")
