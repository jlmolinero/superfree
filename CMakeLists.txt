# SPDX-License-Identifier: GPL-3.0-only

cmake_minimum_required(VERSION 3.10)

project(superfree
    VERSION 0.1.0
    DESCRIPTION "A nicer presentation for Linux memory usage"
    LANGUAGES CXX
)

include(GNUInstallDirs)

add_executable(superfree main.cpp ConsoleTable.cpp ConsoleTable.h)
target_compile_features(superfree PRIVATE cxx_std_11)

install(TARGETS superfree RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
