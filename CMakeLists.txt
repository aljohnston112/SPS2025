cmake_minimum_required(VERSION 3.29)
project(SPS2025 C CXX)

set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 20)
find_package(OpenMP REQUIRED)

set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")

add_compile_options(
        -fopenmp
        -msse4.2
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
        C/probability.c
        C/memory_util.c
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
        C/probability.h
        C/config.h
        C/memory_util.h
)

add_executable(
        SPS2025
        ${C_SOURCE_FILES}
        ${CXX_SOURCE_FILES}
        ${C_HEADER_FILES}
        ${CXX_HEADER_FILES}
        C/main.c
)
if(OpenMP_C_FOUND)
    target_link_libraries(SPS2025 PRIVATE OpenMP::OpenMP_C)
endif()
