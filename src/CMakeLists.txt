cmake_minimum_required(VERSION 2.8)

project(tria CXX)
set(CMAKE_AUTOMOC ON)

find_package(Qt5Core REQUIRED)

Find_Package(LLVM REQUIRED)
link_directories(${LLVM_LIB_DIR})

Find_Package(Clang REQUIRED)
include_directories(${CLANG_INCLUDE_DIRS})

qt5_add_resources(qrc_headers.cpp ../headers.qrc)

add_executable(tria main.cpp triaastconsumer.cpp defs.cpp definitions.cpp nuriagenerator.cpp jsongenerator.cpp qrc_headers.cpp)
target_link_libraries(tria ${CLANG_LIBS} ${LLVM_LIBS_CORE} ${LLVM_LDFLAGS})
qt5_use_modules(tria Core)

ADD_DEFINITIONS("-std=c++11 -Wall ")
ADD_DEFINITIONS("-D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS ")
