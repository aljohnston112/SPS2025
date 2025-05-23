cmake_minimum_required(VERSION 3.31)
project(SPS2025 C CXX)

set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 17)

find_package(Python3 REQUIRED COMPONENTS Interpreter Development NumPy)
include_directories(
        external/matplotlib-cpp
        ${Python3_INCLUDE_DIRS}
        ${Python3_NumPy_INCLUDE_DIRS}
)

set(PARALLEL TRUE)

set(USE_ASAN FALSE)
if (USE_ASAN)
    add_link_options(-fsanitize=address -fsanitize=undefined -fsanitize=bounds-strict -fsanitize=pointer-overflow)
    add_compile_options(-fsanitize=address -fsanitize=undefined -fsanitize=bounds-strict -fsanitize=pointer-overflow)
endif ()

set(USE_GCC TRUE)
if (USE_GCC)
    set(CMAKE_C_COMPILER "gcc")
    set(CMAKE_CXX_COMPILER "g++")
else ()
    set(CMAKE_C_COMPILER "clang")
    set(CMAKE_CXX_COMPILER "clang++")
endif ()

if (PARALLEL)
    find_package(OpenMP REQUIRED)
    if (USE_GCC)
    else ()
        add_link_options(-fopenmp=libomp)
    endif ()
    add_compile_options(-fopenmp)
endif ()


add_compile_options(
        -msse4.2
        -g
        -fno-omit-frame-pointer
        -Wall
)

set(
        CXX_SOURCE_FILES
        C/csv_lion/CsvCell.cpp
)

set(
        CXX_HEADER_FILES
        C/csv_lion/CsvCell.h
)

set(
        C_SOURCE_FILES
        C/csv_lion/CsvCursor.c
        C/csv_lion/MappedFileCursor.c
        C/csv_lion/CsvReader.c
        C/csv_lion/StringSpanner.c
        C/csv_util.c
        C/file_util.c
        C/directions.c
        C/tree.c
        C/ranks.c
)

set(
        C_HEADER_FILES
        C/csv_lion/CsvCursor.h
        C/csv_lion/MappedFileCursor.h
        C/csv_lion/CsvReader.h
        C/csv_lion/StringSpanner.h
        C/csv_util.h
        C/file_util.h
        C/internal_config.h
        C/directions.h
        C/config.h
        C/tree.h
        C/ranks.h
)

add_executable(
        SPS2025
        ${C_HEADER_FILES}
        ${CXX_HEADER_FILES}
        ${C_SOURCE_FILES}
        ${CXX_SOURCE_FILES}
        C/main.c
)

add_library(
        array_to_python SHARED
        C/array_to_python.c
        ${C_HEADER_FILES}
        ${CXX_HEADER_FILES}
        ${C_SOURCE_FILES}
        ${CXX_SOURCE_FILES}
)


set_target_properties(array_to_python PROPERTIES LINKER_LANGUAGE C)

if (PARALLEL)
    target_link_libraries(SPS2025 PRIVATE m OpenMP::OpenMP_C)
endif ()

target_link_libraries(SPS2025 PRIVATE m ${Python3_LIBRARIES})

