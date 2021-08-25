cmake_minimum_required(VERSION 3.10)
project(RocTests)

find_package(LLVM REQUIRED CONFIG)

file(GLOB Sources
        ../compiler/*.h
        ../compiler/*.cpp
        ../parser/*.h
        ../parser/*.cpp
        ../passes/*.h
        ../passes/*.cpp
        ../mir/*.h
        ../mir/*.cpp
        ../linking/*.h
        ../linking/*.cpp)

file(GLOB TestSources test_*.h test_*.cpp)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

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

add_executable(RocTests
        TestRunner.cpp
        Catch.h
        ${Sources}
        ${TestSources}
)

target_link_libraries(
        RocTests
        ${llvm_libs}
)

get_filename_component(PARENT_DIR ${PROJECT_SOURCE_DIR} DIRECTORY)
message(${PARENT_DIR})

add_definitions(-DSANDBOX_DIR="${PROJECT_SOURCE_DIR}/sandbox")
add_definitions(-DSDK_DIR="${PARENT_DIR}/sdk")
