#include "file_util.h"

#include <dirent.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "config.h"
#include "csv_util.h"
#include "internal_config.h"

/**
 * Extracts the stock name from a file path.
 * @param path The path to the stock file.
 * @return The returned string must be freed by the caller.
 */
char* extract_symbol(const char* path) {
    char* symbol = nullptr;
    const char* lastSlashPosition = strrchr(path, '/');
    if (lastSlashPosition != NULL) {
        lastSlashPosition++;
        const char* dotPosition = strchr(lastSlashPosition, '.');
        if (dotPosition != NULL) {
            const size_t length = dotPosition - lastSlashPosition;
            symbol = malloc(length + 1); // +1 for null-terminator
            if (symbol == NULL) {
                perror("malloc failed");
                return nullptr;
            }
            strncpy(symbol, lastSlashPosition, length);
            symbol[length] = '\0';
        } else {
            symbol = strdup(lastSlashPosition);
        }
    }
    return symbol;
}

void getStockDataFromSingleFile(const char* file_name, RowArray* row_array) {
    row_array->stock_symbol = extract_symbol(file_name);
    read_stock_csv(file_name, row_array);
}

void getAllFilesPaths(const char* folder, FileData* file_data) {
    DIR* dir = opendir(folder);
    if (dir == NULL) {
        perror("opendir failed");
        file_data->file_count = 0;
        return;
    }

    size_t capacity = NUMBER_OF_STOCK_EXAMPLES;
    file_data->file_paths = malloc(capacity * sizeof(char*));
    if (file_data->file_paths == NULL) {
        perror("malloc failed");
        closedir(dir);
        file_data->file_count = 0;
        return;
    }

    struct dirent* entry;
    file_data->file_count = 0;

    while ((entry = readdir(dir)) != NULL) {
        char full_path[4096];
        snprintf(full_path, sizeof(full_path), "%s/%s", folder, entry->d_name);
        struct stat file_stat;
        if (stat(full_path, &file_stat) == 0 && S_ISREG(file_stat.st_mode)) {
            if (file_data->file_count >= capacity) {
                capacity *= 2;
                char** temp = realloc(file_data->file_paths, capacity * sizeof(char*));
                if (temp == NULL) {
                    perror("realloc failed");
                    for (int i = 0; i < file_data->file_count; i++) {
                        free(file_data->file_paths[i]);
                    }
                    free(file_data->file_paths);
                    closedir(dir);
                    file_data->file_count = 0;
                    return;
                }
                file_data->file_paths = temp;
            }

            file_data->file_paths[file_data->file_count] = strdup(full_path);
            if (file_data->file_paths[file_data->file_count] == NULL) {
                perror("strdup failed");
                for (int i = 0; i < file_data->file_count; i++) {
                    free(file_data->file_paths[i]);
                }
                free(file_data->file_paths);
                closedir(dir);
                file_data->file_count = 0;
                return;
            }

            (file_data->file_count)++;
        }
    }
    closedir(dir);
}

void freeAllFilesPaths(const FileData* file_data) {
    for (int i = 0; i < file_data->file_count; i++) {
        free(file_data->file_paths[i]);
    }
    free(file_data->file_paths);
}

void loadRawStockData(
    const FileData* file_data,
    const RawStockDataArray* raw_stock_data_results
) {
#pragma omp parallel for default(none) shared(file_data, raw_stock_data_results) num_threads(180) proc_bind(close) if(IS_PARALLEL)
    for (int i = 0; i < file_data->file_count; i++) {
        getStockDataFromSingleFile(
            file_data->file_paths[i],
            &raw_stock_data_results->row_arrays[i]
        );
    }
}

/**
 * The caller is responsible for freeing the returned stock data via freeAllStockData.
 * @return The results of loading the stock data from disk.
 */
bool loadAllStockDataFromDisk(RawStockDataArray* raw_stock_data_array) {

    // filePaths must be freed
    // -----------------------------------------------------------------------------------------
    FileData file_data;
    getAllFilesPaths(INTERMEDIATE_DATA_FOLDER, &file_data);
    if (file_data.file_paths == NULL) {
        fprintf(stderr, "Error: Could not retrieve file paths.\n");
        return false;
    }

    raw_stock_data_array->row_arrays = malloc(file_data.file_count * sizeof(RowArray));
    if (raw_stock_data_array->row_arrays == NULL) {
        freeAllFilesPaths(&file_data);
        perror("malloc failed");
        return false;
    }
    raw_stock_data_array->number_of_raw_stock_data_arrays = file_data.file_count;

    for (int i = 0; i < file_data.file_count; i++) {
        RowArray* current_row = &raw_stock_data_array->row_arrays[i];
        current_row->rows = malloc(LARGEST_STOCK_DATASET_SIZE * sizeof(Row));
        current_row->data_size = LARGEST_STOCK_DATASET_SIZE;
        if (raw_stock_data_array->row_arrays[i].rows == NULL) {
            for (int j = 0; j < i; j++) {
                free(raw_stock_data_array->row_arrays[j].rows);
            }
            free(raw_stock_data_array->row_arrays);
            freeAllFilesPaths(&file_data);
            perror("malloc failed");
            return false;
        }
    }

    loadRawStockData(&file_data, raw_stock_data_array);
    freeAllFilesPaths(&file_data);
    // filePaths freed
    // -------------------------------------------------------------------------------------------------
    return true;
}

void freeAllStockData(const RawStockDataArray* raw_stock_data_array) {
    for (int i = 0; i > raw_stock_data_array->number_of_raw_stock_data_arrays; i++) {
        free(raw_stock_data_array->row_arrays[i].rows);
    }
    free(raw_stock_data_array->row_arrays);
}
