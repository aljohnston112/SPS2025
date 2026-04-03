#include "csv_util_test.h"

#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "test_data.h"

void read_stock_csv_loads_correct_data() {
    StockDataTable table = {
        .rows = malloc(ROWS_IN_TEST_FILE * (sizeof(StockDataRow))),
        .row_count = 0,
        .capacity = ROWS_IN_TEST_FILE
    };
    assert(table.rows);
    read_stock_csv(
        FAKE_DATA_FILE_NAME,
        &table,
        nullptr,
        nullptr
    );
    check_table(&table, expected_rows_for_fake_data, 6);
    free(table.rows);
    free((void*)(uintptr_t)table.stock_symbol);
}

void read_stock_csv_with_start_date_loads_correct_data() {
    StockDataTable table = {
        .rows = malloc(ROWS_IN_TEST_FILE * (sizeof(StockDataRow))),
        .row_count = 0,
        .capacity = ROWS_IN_TEST_FILE
    };
    assert(table.rows);
    constexpr uint16_t start = 2000;
    read_stock_csv(FAKE_DATA_FILE_NAME, &table, &start, nullptr);
    check_table(&table, &expected_rows_for_fake_data[2], 4);
    free(table.rows);
    free((void*)(uintptr_t)table.stock_symbol);
}

void read_stock_csv_with_end_date_loads_correct_data() {
    StockDataTable table = {
        .rows = malloc(ROWS_IN_TEST_FILE * (sizeof(StockDataRow))),
        .row_count = 0,
        .capacity = ROWS_IN_TEST_FILE
    };
    assert(table.rows);
    constexpr uint16_t end = 2001;
    read_stock_csv(FAKE_DATA_FILE_NAME, &table, nullptr, &end);
    check_table(&table, expected_rows_for_fake_data, 4);
    free(table.rows);
    free((void*)(uintptr_t)table.stock_symbol);
}


void read_stock_csv_with_start_and_end_date_loads_correct_data() {
    StockDataTable table = {
        .rows = malloc(ROWS_IN_TEST_FILE * (sizeof(StockDataRow))),
        .row_count = 0,
        .capacity = ROWS_IN_TEST_FILE
    };
    assert(table.rows);
    constexpr uint16_t start = 2000;
    constexpr uint16_t end = 2001;
    read_stock_csv(FAKE_DATA_FILE_NAME, &table, &start, &end);
    check_table(&table, &expected_rows_for_fake_data[2], 2);
    free(table.rows);
    free((void*)(uintptr_t)table.stock_symbol);
}

void run_csv_util_tests() {
    read_stock_csv_loads_correct_data();
    read_stock_csv_with_start_date_loads_correct_data();
    read_stock_csv_with_end_date_loads_correct_data();
    read_stock_csv_with_start_and_end_date_loads_correct_data();
}
