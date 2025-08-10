cmake_minimum_required(VERSION 3.31)

set(USE_GCC TRUE)
if (USE_GCC)
    set(CMAKE_C_COMPILER "gcc")
    set(CMAKE_CXX_COMPILER "g++")
    set(COMMON_COMPILE_FLAGS
            -msse4.2
            -g
            -fno-omit-frame-pointer
            -Wall
            -Wextra
            -Wpedantic
            -Walloca
            -Warray-bounds=2
            -Wattribute-alias=2
            -Wbidi-chars=any
            -Wcast-qual
            -Wcomments
            -Wconversion
            -Wdisabled-optimization
            -Wduplicated-cond
            -Wflex-array-member-not-at-end
            -Wformat=2
            -Wformat-nonliteral
            -Wformat-overflow=2
            -Wshadow
            -Wformat-security
            -Wpacked
            -Wredundant-decls
            -Wnull-dereference
            -Wmultichar
            -Wstrict-overflow=5
            -Wnormalized=nfkc
            -fanalyzer
            -Wformat-signedness
            -Wlogical-op
            -Wformat-truncation=2
            -Wformat-y2k
            -Wmissing-format-attribute
            -Wimplicit-fallthrough=5
            -Winit-self
            -Wstack-protector
            -Winvalid-utf8
            -Wstringop-overflow=4
            -Wfloat-equal
            -Wsuggest-attribute=cold
            -Wsuggest-attribute=const
            -Wsuggest-attribute=format
            -Wsuggest-attribute=malloc
            -Wsuggest-attribute=noreturn
            -Wsuggest-attribute=pure
            -Wsuggest-attribute=returns_nonnull
            -Wswitch-enum
            -Wswitch-default
            -Wduplicated-branches
            -Wtrivial-auto-var-init
            -ftrivial-auto-var-init=pattern
            -Wunused-macros
            -Wwrite-strings
            -Wzero-as-null-pointer-constant
            -Wuse-after-free=3
            --strict-flex-arrays=3
            -Wanalyzer-symbol-too-complex
            -fstack-clash-protection
            -Wframe-larger-than=8192
            -Wstack-usage=8192
            -fstack-protector
            --param=analyzer-max-svalue-depth=50
    )
else ()
    set(CMAKE_C_COMPILER "clang")
    set(CMAKE_CXX_COMPILER "clang++")
    set(COMMON_COMPILE_FLAGS
            -msse4.2
            -g
            -fno-omit-frame-pointer
            -Wall
            -Wextra
            -Wpedantic
    )
endif ()

set(COMMON_LINK_FLAGS)

set(ENABLE_COVERAGE true)
if (ENABLE_COVERAGE)
    list(APPEND COMMON_COMPILE_FLAGS --coverage)
endif ()

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
        C/sequence_analyzer.c
        C/tree_file_util.c
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
        C/sequence_analyzer.h
        C/tree_file_util.h
)

find_package(Python3 REQUIRED COMPONENTS Interpreter Development NumPy)
if (PARALLEL)
    find_package(OpenMP REQUIRED)
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

if (ENABLE_COVERAGE)
    target_link_libraries(SPS2025 PRIVATE gcov)
endif ()

set(
        C_TEST_HEADER_FILES
        C_tests/file_util_test.h
        C_tests/csv_util_test.h
        C_tests/test_data.h
        C_tests/ranks_test.h
        C_tests/tree_test.h
)

set(
        C_TEST_SOURCE_FILES
        C_tests/main.c
        C_tests/file_util_test.c
        C_tests/csv_util_test.c
        C_tests/ranks_test.c
        C_tests/tree_test.c
)

add_executable(
        SPS2025Test
        ${C_HEADER_FILES}
        ${CXX_HEADER_FILES}
        ${C_TEST_HEADER_FILES}
        ${C_SOURCE_FILES}
        ${CXX_SOURCE_FILES}
        ${C_TEST_SOURCE_FILES}
)
set_target_properties(SPS2025Test PROPERTIES LINKER_LANGUAGE C)

target_include_directories(
        SPS2025Test PRIVATE
        external/matplotlib-cpp
        ${Python3_INCLUDE_DIRS}
        ${Python3_NumPy_INCLUDE_DIRS}
)
target_link_libraries(SPS2025Test PRIVATE ${Python3_LIBRARIES})

if (PARALLEL)
    target_link_libraries(SPS2025Test PRIVATE m OpenMP::OpenMP_C)
endif ()
file(COPY C_tests/test_data DESTINATION ${CMAKE_BINARY_DIR})

if (ENABLE_COVERAGE)
    target_link_libraries(SPS2025Test PRIVATE gcov)

    find_program(LCOV_EXEC lcov)
    find_program(GENHTML_EXEC genhtml)

    add_custom_target(
            coverage_clean
            COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/*.info
            COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/coverage_html
    )

    add_custom_target(
            coverage
            DEPENDS coverage_clean
            DEPENDS SPS2025Test
            COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR} ./SPS2025Test
            COMMAND ${LCOV_EXEC} --directory ${CMAKE_BINARY_DIR} --capture --output-file coverage.info
            COMMAND ${LCOV_EXEC} --remove coverage.info '/usr/*' --output-file coverage_filtered.info --ignore-errors unused
            COMMAND ${GENHTML_EXEC} coverage_filtered.info --output-directory coverage_html
            COMMAND xdg-open coverage_html/index.html
            COMMENT "Generating and opening coverage report..."
    )
endif ()

add_library(
        array_to_python SHARED
        C/array_to_python.c
        ${C_HEADER_FILES}
        ${CXX_HEADER_FILES}
        ${C_SOURCE_FILES}
        ${CXX_SOURCE_FILES}
)

set_target_properties(array_to_python PROPERTIES LINKER_LANGUAGE C)

