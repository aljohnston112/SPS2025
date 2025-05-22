#ifndef FILE_UTIL_H
#define FILE_UTIL_H

#include "csv_util.h"

typedef struct {
    char** file_paths;
    size_t file_count;
} FileData;

typedef struct {
    RowArray* row_arrays;
    size_t number_of_raw_stock_data_arrays;
} AllStockDataArrays;


bool load_stock_data_from_disk(
    AllStockDataArrays** raw_stock_data_array,
    const u_int16_t* start_year,
    const u_int16_t* end_year
);

void freeAllStockData(
    AllStockDataArrays* raw_stock_data_array
);

#endif // FILE_UTIL_H
