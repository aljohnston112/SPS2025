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
    TwoDimensionalArrayElement* stockData;
    size_t data_size;
} StockDataResult;

StockDataResult* getAllStockData(int* resultCount);

#endif //FILE_UTIL_H
