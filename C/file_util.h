#ifndef FILE_UTIL_H
#define FILE_UTIL_H

#include <stdint.h>

#include "csv_util.h"

typedef struct {
    char** file_paths;
    size_t file_count;
    size_t capacity;
} FilePathList;

typedef struct {
    StockDataTable* tables;
    size_t table_count;
    size_t capacity;
} StockDataTables;

void get_all_files_paths_recursive(
    const char* folder,
    FilePathList* file_path_list
);

bool load_stock_data_from_disk(
    StockDataTables* stock_data_tables,
    const uint16_t* start_year,
    const uint16_t* end_year,
    const char* folder
);

char* extract_symbol(const char* path);

void load_stock_data_from_files(
    const FilePathList* file_path_list,
    StockDataTables* stock_data_tables,
    const uint16_t* start_year,
    const uint16_t* end_year
);

void free_all_files_paths(FilePathList* file_path_list);

void free_stock_data_tables(StockDataTables* stock_data_tables);

#endif // FILE_UTIL_H
