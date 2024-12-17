cmake_minimum_required(VERSION 3.29)
project(SPS2025 C)

set(CMAKE_C_STANDARD 23)
find_package(OpenMP REQUIRED)

add_compile_options(
        -fopenmp
        -msse4.2
)

set(
        SOURCE_FILES
        C/csv_monkey/CsvCell.c
        C/csv_monkey/CsvCursor.c
        C/csv_monkey/MappedFileCursor.c
        C/csv_monkey/CsvReader.c
        C/csv_monkey/StringSpanner.c
        C/csv_util.c
        C/file_util.c
)

set(
        HEADER_FILES
        C/csv_monkey/CsvCell.h
        C/csv_monkey/CsvCursor.h
        C/csv_monkey/MappedFileCursor.h
        C/csv_monkey/CsvReader.h
        C/csv_monkey/StringSpanner.h
        C/csv_util.h
        C/file_util.h
        C/config.h
)

add_executable(
        SPS2025
        ${SOURCE_FILES}
        ${HEADER_FILES}
        C/main.c
)

if(OpenMP_C_FOUND)
    target_link_libraries(SPS2025 PRIVATE OpenMP::OpenMP_C)
endif()
