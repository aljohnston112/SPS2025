#ifndef FILE_UTIL_H
#define FILE_UTIL_H

#include <stdint.h>

#include "csv_util.h"

typedef struct {
    char** file_paths;
    size_t file_count;
} FilePathList;

typedef struct {
    StockDataTable* tables;
    size_t table_count;
} StockDataTables;

void load_stock_data_from_files(
    const FilePathList* file_path_list,
    const StockDataTables* stock_data_tables,
    const uint16_t* start_year,
    const uint16_t* end_year
);

char* extract_symbol(const char* path);

void get_all_files_paths_recursive(
    const char* folder,
    FilePathList* file_path_list
);

void freeAllFilesPaths(FilePathList* file_path_list);

bool load_stock_data_from_disk(
    StockDataTables* stock_data_tables,
    const uint16_t* start_year,
    const uint16_t* end_year,
    const char* folder
);

void free_stock_data_tables(
    const StockDataTables* stock_data_tables
);

#endif // FILE_UTIL_H
