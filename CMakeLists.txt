# SPDX-License-Identifier: GPL-3.0-only

cmake_minimum_required(VERSION 3.10)

project(superfree
    VERSION 0.1.0
    DESCRIPTION "A nicer presentation for Linux memory usage"
    LANGUAGES CXX
)

include(GNUInstallDirs)
include(CTest)

add_executable(superfree main.cpp ConsoleTable.cpp ConsoleTable.h)
target_compile_features(superfree PRIVATE cxx_std_11)

if(BUILD_TESTING)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_test(
        NAME superfree_cli
        COMMAND ${Python3_EXECUTABLE} -m unittest tests/test_superfree_cli.py -v
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

install(TARGETS superfree RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
