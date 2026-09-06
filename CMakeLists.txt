# SPDX-License-Identifier: GPL-3.0-only

cmake_minimum_required(VERSION 3.10)

project(sfree
    VERSION 1.0.1
    DESCRIPTION "A nicer presentation for Linux memory usage"
    LANGUAGES CXX
)

include(GNUInstallDirs)
include(CTest)

set(SFREE_USR_BIN_DIR "/usr/bin" CACHE PATH "Directory for the sfree compatibility symlink")

add_executable(sfree main.cpp ConsoleTable.cpp ConsoleTable.h)
target_compile_features(sfree PRIVATE cxx_std_11)

if(BUILD_TESTING)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_test(
        NAME sfree_cli
        COMMAND ${Python3_EXECUTABLE} -m unittest tests/test_sfree_cli.py -v
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

install(TARGETS sfree RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(CODE "
    if(\"\${CMAKE_INSTALL_PREFIX}\" MATCHES \"/usr/local$\")
        file(MAKE_DIRECTORY \"${SFREE_USR_BIN_DIR}\")
        execute_process(
            COMMAND \"${CMAKE_COMMAND}\" -E create_symlink
                    \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/sfree\"
                    \"${SFREE_USR_BIN_DIR}/sfree\"
            RESULT_VARIABLE sfree_symlink_result
            ERROR_VARIABLE sfree_symlink_error
        )
        if(NOT sfree_symlink_result EQUAL 0)
            message(FATAL_ERROR \"Failed to create ${SFREE_USR_BIN_DIR}/sfree symlink: \${sfree_symlink_error}\")
        endif()
        message(STATUS \"Installed ${SFREE_USR_BIN_DIR}/sfree -> \${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/sfree\")
    endif()
")
