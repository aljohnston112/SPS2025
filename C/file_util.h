#ifndef FILE_UTIL_H
#define FILE_UTIL_H

#include "csv_util.h"
#include <stdbool.h>

typedef struct {
    char** file_paths;
    size_t file_count;
} FileData;

typedef struct {
    RowArray* row_arrays;
    size_t number_of_raw_stock_data_arrays;
} RawStockDataArray;

char* extract_symbol(const char* path);
bool load_all_stock_data_from_disk(RawStockDataArray** raw_stock_data_array);
bool load_stock_data_to_year_from_disk(
    RawStockDataArray** raw_stock_data_array,
    u_int16_t year
);

bool load_stock_data_from_year_from_disk(
    RawStockDataArray** raw_stock_data_array,
    u_int16_t year
);
void freeAllStockData(RawStockDataArray* raw_stock_data_array);

#endif // FILE_UTIL_H
