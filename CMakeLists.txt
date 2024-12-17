cmake_minimum_required(VERSION 3.29)
project(SPS2025 C)

set(CMAKE_C_STANDARD 23)
add_compile_options(-fopenmp)

set(
        SOURCE_FILES
)

set(
        HEADER_FILES
        C/CSVCell.h
        C/CSVCell.c
)

add_executable(
        SPS2025
        ${SOURCE_FILES}
        ${HEADER_FILES}
        C/main.c
)
