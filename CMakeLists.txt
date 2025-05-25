cmake_minimum_required(VERSION 3.31)

set(USE_GCC TRUE)
if (USE_GCC)
    set(CMAKE_C_COMPILER "gcc")
    set(CMAKE_CXX_COMPILER "g++")
else ()
    set(CMAKE_C_COMPILER "clang")
    set(CMAKE_CXX_COMPILER "clang++")
endif ()
set(COMMON_COMPILE_FLAGS -msse4.2 -g -fno-omit-frame-pointer -Wall)
set(COMMON_LINK_FLAGS "")

project(SPS2025 C CXX)

set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 20)

set(USE_ASAN FALSE)
set(PARALLEL TRUE)

set(
        CXX_SOURCE_FILES
        CPP/csv_lion/CsvCell.cpp
        CPP/grapher.cpp
)

set(
        CXX_HEADER_FILES
        CPP/csv_lion/CsvCell.h
        CPP/grapher.h
        CPP/matplotlibcpp.h
)

set(
        C_SOURCE_FILES
        CPP/csv_lion/CsvCursor.c
        CPP/csv_lion/MappedFileCursor.c
        CPP/csv_lion/CsvReader.c
        CPP/csv_lion/StringSpanner.c
        C/csv_util.c
        C/file_util.c
        C/directions.c
        C/tree.c
        C/ranks.c
)

set(
        C_HEADER_FILES
        CPP/csv_lion/CsvCursor.h
        CPP/csv_lion/MappedFileCursor.h
        CPP/csv_lion/CsvReader.h
        CPP/csv_lion/StringSpanner.h
        C/csv_util.h
        C/file_util.h
        C/internal_config.h
        C/directions.h
        C/config.h
        C/tree.h
        C/ranks.h
)

find_package(Python3 REQUIRED COMPONENTS Interpreter Development NumPy)
if (PARALLEL)
    find_package(OpenMP REQUIRED)
    if (NOT USE_GCC)
        list(APPEND COMMON_LINK_FLAGS -fopenmp=libomp)
    endif ()
    list(APPEND COMMON_COMPILE_FLAGS -fopenmp)
endif ()

if (USE_ASAN)
    set(
            ASAN_FLAGS
            -fsanitize=address
            -fsanitize=undefined
            -fsanitize=bounds-strict
            -fsanitize=pointer-overflow
    )
    list(APPEND COMMON_COMPILE_FLAGS ${ASAN_FLAGS})
    list(APPEND COMMON_LINK_FLAGS ${ASAN_FLAGS})
endif ()

add_compile_options(${COMMON_COMPILE_FLAGS})
add_link_options(${COMMON_LINK_FLAGS})
add_executable(
        SPS2025
        ${C_HEADER_FILES}
        ${CXX_HEADER_FILES}
        ${C_SOURCE_FILES}
        ${CXX_SOURCE_FILES}
        C/main.c
)

target_include_directories(
        SPS2025 PRIVATE
        external/matplotlib-cpp
        ${Python3_INCLUDE_DIRS}
        ${Python3_NumPy_INCLUDE_DIRS}
)

if (PARALLEL)
    target_link_libraries(SPS2025 PRIVATE m OpenMP::OpenMP_C)
endif ()
target_link_libraries(SPS2025 PRIVATE ${Python3_LIBRARIES})

add_library(
        array_to_python SHARED
        C/array_to_python.c
        ${C_HEADER_FILES}
        ${CXX_HEADER_FILES}
        ${C_SOURCE_FILES}
        ${CXX_SOURCE_FILES}
)

set_target_properties(array_to_python PROPERTIES LINKER_LANGUAGE C)

