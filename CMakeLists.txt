cmake_minimum_required(VERSION 3.17)
project(roc-lang)
find_package(LLVM REQUIRED CONFIG)

SET(CMAKE_C_USE_RESPONSE_FILE_FOR_OBJECTS 1)
SET(CMAKE_CXX_USE_RESPONSE_FILE_FOR_OBJECTS 1)

SET(CMAKE_C_RESPONSE_FILE_LINK_FLAG "@")
SET(CMAKE_CXX_RESPONSE_FILE_LINK_FLAG "@")

set(CMAKE_CXX_STANDARD 14)

file(GLOB Sources
        parser/*.cpp
        parser/*.h
        compiler/*.h
        compiler/*.cpp
        mir/*.h
        mir/*.cpp
        linking/*.h
        linking/*.cpp
)

message("List of Parser files: ${ParserSources}")
include_directories(
    ${PROJECT_SOURCE_DIR}/libs/
)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_executable(roc-lang
        main.cpp
        ${Sources})

llvm_map_components_to_libnames(llvm_libs
       X86AsmParser
       AArch64AsmParser
       Target
       Core
       ExecutionEngine
       Interpreter
       MC
       MCJIT
       Support
       nativecodegen)

target_link_libraries(roc-lang
        ${llvm_libs}
        )

