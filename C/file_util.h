#ifndef FILE_UTIL_H
#define FILE_UTIL_H
#include <stddef.h>

#include "csv_util.h"

typedef struct {
  char** file_paths;
  size_t file_count;
  size_t capacity;
} FileData;

typedef struct {
  char* symbol;
  Row* stockData;
  size_t data_size;
} RawStockDataResults;

RawStockDataResults* loadAllStockDataFromDisk(int* resultCount);
void freeAllStockData(RawStockDataResults* results, int resultsCount);

#endif // FILE_UTIL_H
