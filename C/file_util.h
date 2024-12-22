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
} RawStockDataArray;

bool loadAllStockDataFromDisk(RawStockDataArray* raw_stock_data_array);
void freeAllStockData(const RawStockDataArray* raw_stock_data_array);

#endif // FILE_UTIL_H
