cmake_minimum_required(VERSION 3.29)
project(SPS2025 C)

set(CMAKE_C_STANDARD 23)
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
)

set(
        HEADER_FILES
        C/csv_monkey/CsvCell.h
        C/csv_monkey/CsvCursor.h
        C/csv_monkey/MappedFileCursor.h
        C/csv_monkey/CsvReader.h
        C/csv_monkey/StringSpanner.h
        C/csv_util.h

)

add_executable(
        SPS2025
        ${SOURCE_FILES}
        ${HEADER_FILES}
        C/main.c
)
