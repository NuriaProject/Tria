# CMake file for NuriaTria
# Source: https://github.com/NuriaProject/Tria
cmake_minimum_required(VERSION 2.8.8)

# Base configuration
PROJECT(Tria)

cmake_policy(SET CMP0020 NEW)

# 
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5Core REQUIRED)
Find_Package(LLVM REQUIRED)
Find_Package(Clang REQUIRED)
set(CMAKE_AUTOMOC ON)

set(Resource "${CMAKE_BINARY_DIR}/tria_headers.qrc")

# Write the headers.qrc file. A Qt resources file for RCC which takes all
# built-in headers of Clang and stores them as application resources for
# later use in Tria.
# Inspired by Moc-NGs CMakeLists.txt

exec_program(${LLVM_CONFIG_EXECUTABLE} ARGS --version OUTPUT_VARIABLE LLVM_VERSION )
string(REPLACE "svn" "" LLVM_VERSION "${LLVM_VERSION}")

# Helper function to add all files to 'Resoucrce' to 'Prefix' from 'Path' by 'Glob'
function(add_files_as_resources Resource Prefix Path Glob)
  message(STATUS "Adding resource files from ${Path} to ${Prefix}")
  file(APPEND ${Resource} "  <qresource prefix=\"${Prefix}\">\n")
  file(GLOB_RECURSE Files "${Path}${Glob}")
  
  foreach(File ${Files})
    string(REPLACE "${Path}" "" Alias "${File}")
    file(APPEND ${Resource} "    <file alias=\"${Alias}\">${File}</file>\n")
  endforeach()
  
  file(APPEND ${Resource} "  </qresource>\n")
endfunction()

# Write Qt resource file
file(WRITE ${Resource} "<!DOCTYPE RCC>\n<RCC version=\"1.0\">\n")
add_files_as_resources(${Resource} "/headers" "${LLVM_BIN_DIR}/../lib/clang/${LLVM_VERSION}/include/" "*.h")
add_files_as_resources(${Resource} "/lua" "${CMAKE_CURRENT_SOURCE_DIR}/lua/" "*.lua")
file(APPEND ${Resource} "</RCC>")

# Paths ..
include_directories(${CLANG_INCLUDE_DIRS})
link_directories(${LLVM_LIB_DIR})

# Include the qrc pointing to the internal Clang headers
qt5_add_resources(RESOURCE_SOURCES ${Resource})

# Sources
SET(Tria_SRC
    src/definitions.cpp
    src/definitions.hpp
    src/defs.cpp
    src/defs.hpp
    src/main.cpp
    src/luagenerator.cpp
    src/luagenerator.hpp
    src/luashell.cpp
    src/luashell.hpp
    src/triaaction.cpp
    src/triaaction.hpp
    src/triaastconsumer.cpp
    src/triaastconsumer.hpp
    src/filemapper.cpp
    src/filemapper.hpp
    src/compiler.cpp
    src/compiler.hpp
)

# Build target
add_executable(tria ${Tria_SRC} ${RESOURCE_SOURCES})
target_link_libraries(tria LuaJit ${CLANG_LIBS} ${LLVM_LIBS_CORE} ${LLVM_LDFLAGS})
target_include_directories(tria PRIVATE ${LUAJIT_INCLUDE_DIR})
qt5_use_modules(tria Core)

# Clang3.5 compatibility. Shouldn't break linking against Clang3.4
if(UNIX)
  target_link_libraries(tria -lpthread -lz -ldl -lcurses)
endif()

ADD_DEFINITIONS(-D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS)

# Install target
INSTALL(TARGETS tria EXPORT triaConfig DESTINATION bin/)
INSTALL(EXPORT triaConfig DESTINATION lib/cmake/tria)

export(TARGETS tria FILE "${NURIA_CMAKE_PREFIX}/triaConfig.cmake")

# Tests are included in NuriaCore as they depend on Core, while tria does not.
