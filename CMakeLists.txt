# CMake file for NuriaTria
# Source: https://github.com/NuriaProject/Tria
cmake_minimum_required(VERSION 2.8.8)

# Base configuration
PROJECT(Tria)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

find_package(Qt5Core REQUIRED)
Find_Package(LLVM REQUIRED)
Find_Package(Clang REQUIRED)
set(CMAKE_AUTOMOC ON)

set(RESOURCE "${CMAKE_BINARY_DIR}/tria_headers.qrc")

# Write the headers.qrc file. A Qt resources file for RCC which takes all
# built-in headers of Clang and stores them as application resources for
# later use in Tria.
# Inspired by Moc-NGs CMakeLists.txt
file(WRITE ${RESOURCE} "<!DOCTYPE RCC>\n<RCC version=\"1.0\">\n")
file(APPEND ${RESOURCE} "<qresource prefix=\"/headers\">\n")
exec_program(${LLVM_CONFIG_EXECUTABLE} ARGS --version OUTPUT_VARIABLE LLVM_VERSION )
string(REPLACE "svn" "" LLVM_VERSION "${LLVM_VERSION}")
file(GLOB_RECURSE BUILTINS_HEADERS "${LLVM_BIN_DIR}/../lib/clang/${LLVM_VERSION}/include/*.h")
foreach(BUILTIN_HEADER ${BUILTINS_HEADERS})
    string(REPLACE "${LLVM_BIN_DIR}/../lib/clang/${LLVM_VERSION}/include/" "" ALIAS "${BUILTIN_HEADER}"  )
    file(APPEND ${RESOURCE} "<file alias=\"${ALIAS}\">${BUILTIN_HEADER}</file>\n")
endforeach()
file(APPEND ${RESOURCE} "</qresource>\n</RCC>")

# Paths ..
include_directories(${CLANG_INCLUDE_DIRS})
link_directories(${LLVM_LIB_DIR})

# Include the qrc pointing to the internal Clang headers
qt5_add_resources(RESOURCE_SOURCES ${RESOURCE})

# Sources
SET(Tria_SRC
    src/definitions.cpp
    src/definitions.hpp
    src/defs.cpp
    src/defs.hpp
    src/jsongenerator.cpp
    src/jsongenerator.hpp
    src/main.cpp
    src/nuriagenerator.cpp
    src/nuriagenerator.hpp
    src/triaastconsumer.cpp
    src/triaastconsumer.hpp
)

# Build target
add_executable(tria ${Tria_SRC} ${RESOURCE_SOURCES})
target_link_libraries(tria ${CLANG_LIBS} ${LLVM_LIBS_CORE} ${LLVM_LDFLAGS})
qt5_use_modules(tria Core)

LIST(APPEND CMAKE_CXX_FLAGS "-std=c++11 -Wall")
ADD_DEFINITIONS(-D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS)

# Install target
INSTALL(TARGETS tria DESTINATION bin/)
