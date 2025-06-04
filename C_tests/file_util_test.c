#include "file_util_test.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <asm-generic/errno-base.h>
#include <sys/stat.h>

#include "../C/file_util.h"

#include <stdint.h>

#include "test_data.h"

#define TEST_FILE_COUNT 2

void load_stock_data_from_disk_loads_correct_data() {
    StockDataTables* tables = malloc(sizeof(StockDataTables));
        tables->tables = nullptr;
        tables->table_count = 0;
    const bool success = load_stock_data_from_disk(
        tables,
        nullptr,
        nullptr,
        "./test_data"
    );
    assert(success);
    assert(tables->tables);
    assert(tables->table_count == 2);

    for (int i = 0; i < TEST_FILE_COUNT; i++) {
        const StockDataTable* table = &tables->tables[i];
        const bool is_first =
            strcmp(table->stock_symbol, "fake_data") == 0;
        const bool is_second =
            strcmp(table->stock_symbol, "fake_data2") == 0;
        assert(is_first || is_second);
        assert(table->row_count == 6);
        if (is_first) {
            check_table(table, expected_rows_for_fake_data, 6);
        } else {
            check_table(table, expected_rows_for_fake_data2, 6);
        }
    }

    free_stock_data_tables(tables);
}

void load_stock_data_from_disk_with_start_date_loads_correct_data() {
    StockDataTables* tables = malloc(sizeof(StockDataTables));
        tables->tables = nullptr;
        tables->table_count = 0;
    constexpr uint16_t start_year = 2001;
    const bool success = load_stock_data_from_disk(
        tables,
        &start_year,
        nullptr,
        "./test_data"
    );
    assert(success);
    assert(tables->tables != NULL);
    assert(tables->table_count == 2);

    for (int i = 0; i < TEST_FILE_COUNT; i++) {
        const StockDataTable* table = &tables->tables[i];
        const bool is_first =
            strcmp(table->stock_symbol, "fake_data") == 0;
        const bool is_second =
            strcmp(table->stock_symbol, "fake_data2") == 0;
        assert(is_first || is_second);
        if (is_first) {
            assert(table->row_count == 2);
            check_table(table, &expected_rows_for_fake_data[4], 2);
        } else {
            assert(table->row_count == 6);
            check_table(table, expected_rows_for_fake_data2, 6);
        }
    }

    free_stock_data_tables(tables);
}

void load_stock_data_from_disk_with_end_date_loads_correct_data() {
    StockDataTables* tables = malloc(sizeof(StockDataTables));
    tables->tables = nullptr;
    tables->table_count = 0;

    constexpr uint16_t end_year = 2001;
    const bool success = load_stock_data_from_disk(
        tables,
        nullptr,
        &end_year,
        "./test_data"
    );
    assert(success);
    assert(tables->tables != NULL);
    assert(tables->table_count == 1);

    const StockDataTable* table = &tables->tables[0];
    assert(strcmp(table->stock_symbol, "fake_data") == 0);
    assert(table->row_count == 4);
    check_table(table, expected_rows_for_fake_data, 4);

    free_stock_data_tables(tables);
}

void load_stock_data_from_disk_with_start_and_end_date_loads_correct_data() {
    StockDataTables* tables = malloc(sizeof(StockDataTables));
    tables->tables = nullptr;
    tables->table_count = 0;

    constexpr uint16_t start_year = 2001;
    constexpr uint16_t end_year = 2002;
    const bool success = load_stock_data_from_disk(
        tables,
        &start_year,
        &end_year,
        "./test_data"
    );
    assert(success);
    assert(tables->tables != NULL);
    assert(tables->table_count == 2);

    for (int i = 0; i < TEST_FILE_COUNT; i++) {
        const StockDataTable* table = &tables->tables[i];
        const bool is_first =
            strcmp(table->stock_symbol, "fake_data") == 0;
        const bool is_second =
            strcmp(table->stock_symbol, "fake_data2") == 0;
        assert(is_first || is_second);
        if (is_first) {
            assert(table->row_count == 2);
            check_table(table, &expected_rows_for_fake_data[4], 2);
        } else {
            assert(table->row_count == 2);
            check_table(table, expected_rows_for_fake_data2, 2);
        }
    }

    free_stock_data_tables(tables);
}

void load_raw_stock_data_loads_correct_data() {
    char* test_file_names[TEST_FILE_COUNT] = {
        FAKE_DATA_FILE_NAME,
        FAKE_DATA_FILE_NAME2
    };
    const FilePathList file_list = {
        .file_paths = test_file_names,
        .file_count = TEST_FILE_COUNT
    };

    StockDataTables* tables = malloc(sizeof(StockDataTables));
    assert(tables);
    tables->tables = nullptr;
    tables->table_count = 0;

    tables->tables = malloc(TEST_FILE_COUNT * sizeof(StockDataTable));
    assert(tables->tables);
    tables->table_count = TEST_FILE_COUNT;

    for (int i = 0; i < TEST_FILE_COUNT; i++) {
        StockDataTable* current_table = &tables->tables[i];
        current_table->rows =
            malloc(ROWS_IN_TEST_FILE * sizeof(StockDataRow));
        assert(current_table->rows);
        current_table->row_count = ROWS_IN_TEST_FILE;
        if (current_table->rows == NULL) {
            perror("Failed to allocate stock data rows");
            exit(1);
        }
    }

    load_stock_data_from_files(&file_list, tables, nullptr, nullptr);
    for (int i = 0; i < TEST_FILE_COUNT; i++) {
        const StockDataTable* table = &tables->tables[i];
        const bool is_first =
            strcmp(table->stock_symbol, "fake_data") == 0;
        const bool is_second =
            strcmp(table->stock_symbol, "fake_data2") == 0;
        assert(is_first || is_second);
        assert(table->row_count == 6);
        if (is_first) {
            check_table(table, expected_rows_for_fake_data, 6);
        } else {
            check_table(table, expected_rows_for_fake_data2, 6);
        }
    }

    free_stock_data_tables(tables);
}

void load_raw_stock_data_with_start_date_loads_correct_data() {
    char* test_file_names[TEST_FILE_COUNT] = {
        FAKE_DATA_FILE_NAME,
        FAKE_DATA_FILE_NAME2
    };
    const FilePathList file_list = {
        .file_paths = test_file_names,
        .file_count = TEST_FILE_COUNT
    };

    StockDataTables* tables = malloc(sizeof(StockDataTables));
    tables->tables = nullptr;
    tables->table_count = 0;

    tables->tables = malloc(TEST_FILE_COUNT * sizeof(StockDataTable));
    assert(tables->tables);
    tables->table_count = TEST_FILE_COUNT;

    for (int i = 0; i < TEST_FILE_COUNT; i++) {
        StockDataTable* current_table = &tables->tables[i];
        current_table->rows =
            malloc(ROWS_IN_TEST_FILE * sizeof(StockDataRow));
        assert(current_table->rows);
        current_table->row_count = ROWS_IN_TEST_FILE;
        if (current_table->rows == NULL) {
            perror("Failed to allocate stock data rows");
            exit(1);
        }
    }

    constexpr uint16_t start_year = 2001;
    load_stock_data_from_files(&file_list, tables, &start_year, nullptr);
    for (int i = 0; i < TEST_FILE_COUNT; i++) {
        const StockDataTable* table = &tables->tables[i];
        const bool is_first =
            strcmp(table->stock_symbol, "fake_data") == 0;
        const bool is_second =
            strcmp(table->stock_symbol, "fake_data2") == 0;
        assert(is_first || is_second);
        if (is_first) {
            assert(table->row_count == 2);
            check_table(table, &expected_rows_for_fake_data[4], 2);
        } else {
            assert(table->row_count == 6);
            check_table(table, expected_rows_for_fake_data2, 6);
        }
    }

    free_stock_data_tables(tables);
}

void load_raw_stock_data_with_end_date_loads_correct_data() {
    char* test_file_names[TEST_FILE_COUNT] = {
        FAKE_DATA_FILE_NAME,
        FAKE_DATA_FILE_NAME2
    };
    const FilePathList file_list = {
        .file_paths = test_file_names,
        .file_count = TEST_FILE_COUNT
    };

    StockDataTables* tables = malloc(sizeof(StockDataTables));
    assert(tables);
    tables->tables = nullptr;
    tables->table_count = 0;
    tables->tables = malloc(TEST_FILE_COUNT * sizeof(StockDataTable));
    assert(tables->tables);
    tables->table_count = TEST_FILE_COUNT;

    for (int i = 0; i < TEST_FILE_COUNT; i++) {
        StockDataTable* current_table = &tables->tables[i];
        current_table->rows =
            malloc(ROWS_IN_TEST_FILE * sizeof(StockDataRow));
        assert(current_table->rows);
        current_table->row_count = ROWS_IN_TEST_FILE;
        if (current_table->rows == NULL) {
            perror("Failed to allocate stock data rows");
            exit(1);
        }
    }

    constexpr uint16_t end_year = 2001;
    load_stock_data_from_files(&file_list, tables, nullptr, &end_year);
    assert(tables->table_count == 1);
    const StockDataTable* table = &tables->tables[0];
    assert(strcmp(table->stock_symbol, "fake_data") == 0);
    assert(table->row_count == 4);
    check_table(table, expected_rows_for_fake_data, 4);

    free_stock_data_tables(tables);
}

void load_raw_stock_data_with_start_and_end_date_loads_correct_data() {
    char* test_file_names[TEST_FILE_COUNT] = {
        FAKE_DATA_FILE_NAME,
        FAKE_DATA_FILE_NAME2
    };
    const FilePathList file_list = {
        .file_paths = test_file_names,
        .file_count = TEST_FILE_COUNT
    };

    StockDataTables* tables = malloc(sizeof(StockDataTables));
    tables->tables = nullptr;
    tables->table_count = 0;
    tables->tables = malloc(TEST_FILE_COUNT * sizeof(StockDataTable));
    assert(tables->tables);
    tables->table_count = TEST_FILE_COUNT;

    for (int i = 0; i < TEST_FILE_COUNT; i++) {
        StockDataTable* current_table = &tables->tables[i];
        current_table->rows =
            malloc(ROWS_IN_TEST_FILE * sizeof(StockDataRow));
        assert(current_table->rows);
        current_table->row_count = ROWS_IN_TEST_FILE;
        if (current_table->rows == NULL) {
            perror("Failed to allocate stock data rows");
            exit(1);
        }
    }
    constexpr uint16_t start_year = 2001;
    constexpr uint16_t end_year = 2002;
    load_stock_data_from_files(&file_list, tables, &start_year, &end_year);
    for (int i = 0; i < TEST_FILE_COUNT; i++) {
        const StockDataTable* table = &tables->tables[i];
        const bool is_first =
            strcmp(table->stock_symbol, "fake_data") == 0;
        const bool is_second =
            strcmp(table->stock_symbol, "fake_data2") == 0;
        assert(is_first || is_second);
        if (is_first) {
            assert(table->row_count == 2);
            check_table(table, &expected_rows_for_fake_data[4], 2);
        } else {
            assert(table->row_count == 2);
            check_table(table, expected_rows_for_fake_data2, 2);
        }
    }

    free_stock_data_tables(tables);
}

void extract_symbol_extracts_symbol() {
    char* symbol = extract_symbol("./a/b/c/d/e.f.g.h.i.txt");
    assert(symbol != NULL);
    assert(strcmp(symbol, "e") == 0);
    free(symbol);
}

void create_folder(const char* folder) {
    assert(mkdir(folder, 999) != -1 || errno == EEXIST);
}

void get_all_files_paths_recursive_on_empty_folder_returns_nothing() {
    const char* folder = "./test_empty";
    create_folder(folder);

    FilePathList list;
    get_all_files_paths_recursive(folder, &list);
    assert(list.file_count == 0);
    rmdir(folder);
    freeAllFilesPaths(&list);
}

void get_all_files_paths_recursive_on_nested_folder_returns_both_files() {
    const char* base_folder_name = "./test_nested";
    create_folder(base_folder_name);
    const char* nested_folder_name = "./test_nested/nested_folder";
    create_folder(nested_folder_name);

    const char* filename1 = "./test_nested/file1.txt";
    FILE* f1 = fopen(filename1, "w");
    assert(f1);
    fclose(f1);

    const char* filename2 = "./test_nested/nested_folder/file2.txt";
    FILE* f2 = fopen(filename2, "w");
    assert(f2);
    fclose(f2);

    const char* nested_folder2_name = "./test_nested/nested_folder/nested2";
    create_folder(nested_folder2_name);

    FilePathList list;
    get_all_files_paths_recursive(base_folder_name, &list);

    assert(list.file_count == 2);
    assert(
        strcmp(
            list.file_paths[0],
            "./test_nested/nested_folder/file2.txt"
        ) == 0
    );
    assert(
        strcmp(
            list.file_paths[1],
            "./test_nested/file1.txt"
        ) == 0
    );
    remove(filename1);
    remove(filename2);
    rmdir(nested_folder2_name);
    rmdir(nested_folder_name);
    rmdir(base_folder_name);
    freeAllFilesPaths(&list);
}

void run_file_util_tests() {
    extract_symbol_extracts_symbol();
    get_all_files_paths_recursive_on_empty_folder_returns_nothing();
    get_all_files_paths_recursive_on_nested_folder_returns_both_files();
    load_raw_stock_data_loads_correct_data();
    load_raw_stock_data_with_start_date_loads_correct_data();
    load_raw_stock_data_with_end_date_loads_correct_data();
    load_raw_stock_data_with_start_and_end_date_loads_correct_data();
    load_stock_data_from_disk_loads_correct_data();
    load_stock_data_from_disk_with_start_date_loads_correct_data();
    load_stock_data_from_disk_with_end_date_loads_correct_data();
    load_stock_data_from_disk_with_start_and_end_date_loads_correct_data();
}
