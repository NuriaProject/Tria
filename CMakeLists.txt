cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)
add_subdirectory(src)

Find_Package(LLVM REQUIRED)

# Write the headers.qrc file. A Qt resources file for RCC which takes all
# built-in headers of Clang and stores them as application resources for
# later use in Tria.
# Inspired by Moc-NGs CMakeLists.txt
file(WRITE headers.qrc "<!DOCTYPE RCC>\n<RCC version=\"1.0\">\n")
file(APPEND headers.qrc "<qresource prefix=\"/headers\">\n")
exec_program(${LLVM_CONFIG_EXECUTABLE} ARGS --version OUTPUT_VARIABLE LLVM_VERSION )
string(REPLACE "svn" "" LLVM_VERSION "${LLVM_VERSION}")
file(GLOB_RECURSE BUILTINS_HEADERS "${LLVM_BIN_DIR}/../lib/clang/${LLVM_VERSION}/include/*.h")
foreach(BUILTIN_HEADER ${BUILTINS_HEADERS})
    string(REPLACE "${LLVM_BIN_DIR}/../lib/clang/${LLVM_VERSION}/include/" "" ALIAS "${BUILTIN_HEADER}"  )
    file(APPEND headers.qrc "<file alias=\"${ALIAS}\">${BUILTIN_HEADER}</file>\n")
endforeach()
file(APPEND headers.qrc "</qresource>\n</RCC>")

# 
INSTALL(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/src/tria DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/../bin)
