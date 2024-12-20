#ifndef FILE_UTIL_H
#define FILE_UTIL_H
#include <stddef.h>

#include "config.h"
#include "csv_util.h"

typedef struct {
  char** file_paths;
  size_t file_count;
  size_t capacity;
} FileData;

typedef struct {
  char* symbol;
  RowArray rows[LARGEST_STOCK_DATASET_SIZE];
} RawStockDataResults;

RawStockDataResults* loadAllStockDataFromDisk(int* resultCount);
void freeAllStockData(RawStockDataResults* results, int resultsCount);

#endif // FILE_UTIL_H
