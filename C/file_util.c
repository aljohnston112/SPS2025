#include "file_util.h"

#include <assert.h>
#include <dirent.h>
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
    assert(path != NULL);

    char* symbol = NULL;
    const char* lastSlashPosition = strrchr(path, '/');
    if (lastSlashPosition != NULL) {
        lastSlashPosition++;
        const char* dotPosition = strchr(lastSlashPosition, '.');
        assert(dotPosition != NULL);

        const size_t length = dotPosition - lastSlashPosition;
        symbol = malloc(length + 1); // +1 for null-terminator
        if (symbol == NULL) {
            perror("malloc failed");
            return NULL;
        }
        strncpy(symbol, lastSlashPosition, length);
        symbol[length] = '\0';
    }

    assert(symbol != NULL);
    return symbol;
}

void getAllFilesPaths(const char* folder, FileData* file_data) {
    DIR* dir = opendir(folder);
    if (dir == NULL) {
        perror("opendir failed");
        exit(1);
    }

    file_data->file_paths = malloc(NUMBER_OF_STOCK_EXAMPLES * sizeof(char*));
    if (file_data->file_paths == NULL) {
        perror("malloc failed");
        closedir(dir);
        exit(1);
        return;
    }

    struct dirent* entry;
    file_data->file_count = 0;

    while ((entry = readdir(dir)) != NULL &&
        file_data->file_count < NUMBER_OF_STOCK_EXAMPLES
    ) {
        char full_path[4096];
        snprintf(
            full_path,
            sizeof(full_path),
            "%s/%s",
            folder,
            entry->d_name
        );
        struct stat file_stat;
        if (stat(full_path, &file_stat) == 0 && S_ISREG(file_stat.st_mode)) {
            file_data->file_paths[file_data->file_count] = strdup(full_path);
            if (file_data->file_paths[file_data->file_count] == NULL) {
                perror("strdup failed");
                closedir(dir);
                exit(1);
            }
            ++file_data->file_count;
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

void load_raw_stock_data(
    const FileData* file_data,
    const RawStockDataArray* raw_stock_data_results
) {
#if IS_PARALLEL
    #pragma omp parallel \
        for default(none) \
        shared(file_data, raw_stock_data_results) \
        num_threads(180) \
        proc_bind(close)
#endif
    for (int i = 0; i < file_data->file_count; i++) {
        const char* file_name = file_data->file_paths[i];
        RowArray* row_array = &raw_stock_data_results->row_arrays[i];
        row_array->stock_symbol = extract_symbol(file_name);
        read_stock_csv(file_name, row_array);
    }
}

void load_raw_stock_data_from_year(
    const FileData* file_data,
    const RawStockDataArray* raw_stock_data_results,
    const u_int16_t year
) {
#if IS_PARALLEL
#pragma omp parallel \
    for default(none) \
    shared(file_data, raw_stock_data_results, year) \
    num_threads(180) \
    proc_bind(close)
#endif
    for (int i = 0; i < file_data->file_count; i++) {
        const char* file_name = file_data->file_paths[i];
        RowArray* row_array = &raw_stock_data_results->row_arrays[i];
        row_array->stock_symbol = extract_symbol(file_name);
        read_stock_csv_from_year(file_name, row_array, year);
    }
}

void load_raw_stock_data_to_year(
    const FileData* file_data,
    const RawStockDataArray* raw_stock_data_results,
    const u_int16_t year
) {
#if IS_PARALLEL
#pragma omp parallel \
    for default(none) \
    shared(file_data, raw_stock_data_results, year) \
    num_threads(180) \
    proc_bind(close)
#endif
    for (int i = 0; i < file_data->file_count; i++) {
        const char* file_name = file_data->file_paths[i];
        RowArray* row_array = &raw_stock_data_results->row_arrays[i];
        row_array->stock_symbol = extract_symbol(file_name);
        read_stock_csv_to_year(file_name, row_array, year);
    }
}


/**
 * The caller is responsible for freeing the returned stock data via freeAllStockData.
 * @return The results of loading the stock data from disk.
 */
bool load_all_stock_data_from_disk(RawStockDataArray** raw_stock_data_array) {
    // filePaths must be freed
    // -----------------------------------------------------------------------------------------
    FileData file_data;
    getAllFilesPaths(INTERMEDIATE_DATA_FOLDER, &file_data);
    if (file_data.file_paths == NULL) {
        fprintf(stderr, "Error: Could not retrieve file paths.\n");
        exit(1);
    }

    // assert(file_data.file_count == NUMBER_OF_STOCK_EXAMPLES);
    (*raw_stock_data_array) = malloc(sizeof(RawStockDataArray));
    (*raw_stock_data_array)->row_arrays = malloc(
        file_data.file_count * sizeof(RowArray)
    );
    if ((*raw_stock_data_array)->row_arrays == NULL) {
        perror("malloc failed");
        exit(1);
    }
    (*raw_stock_data_array)->number_of_raw_stock_data_arrays =
        file_data.file_count;

    for (int i = 0; i < file_data.file_count; i++) {
        RowArray* current_row_array = &(*raw_stock_data_array)->row_arrays[i];
        current_row_array->rows = malloc(
            LARGEST_STOCK_DATASET_SIZE * sizeof(Row)
        );
        current_row_array->data_size = LARGEST_STOCK_DATASET_SIZE;
        if (current_row_array->rows == NULL) {
            perror("malloc failed");
            exit(1);
        }
    }

    load_raw_stock_data(&file_data, *raw_stock_data_array);
    freeAllFilesPaths(&file_data);
    // filePaths freed
    // -------------------------------------------------------------------------
    return true;
}

bool load_stock_data_to_year_from_disk(
    RawStockDataArray** raw_stock_data_array,
    const u_int16_t year
    ) {
    // filePaths must be freed
    // -------------------------------------------------------------------------
    FileData file_data;
    getAllFilesPaths(INTERMEDIATE_DATA_FOLDER, &file_data);
    if (file_data.file_paths == NULL) {
        fprintf(stderr, "Error: Could not retrieve file paths.\n");
        exit(1);
    }

    // assert(file_data.file_count == NUMBER_OF_STOCK_EXAMPLES);
    (*raw_stock_data_array) = malloc(sizeof(RawStockDataArray));
    (*raw_stock_data_array)->row_arrays = malloc(
        file_data.file_count * sizeof(RowArray)
    );
    if ((*raw_stock_data_array)->row_arrays == NULL) {
        perror("malloc failed");
        exit(1);
    }
    (*raw_stock_data_array)->number_of_raw_stock_data_arrays =
        file_data.file_count;

    for (int i = 0; i < file_data.file_count; i++) {
        RowArray* current_row_array = &(*raw_stock_data_array)->row_arrays[i];
        current_row_array->rows = malloc(
            LARGEST_STOCK_DATASET_SIZE * sizeof(Row)
        );
        current_row_array->data_size = LARGEST_STOCK_DATASET_SIZE;
        if (current_row_array->rows == NULL) {
            perror("malloc failed");
            exit(1);
        }
    }

    load_raw_stock_data_to_year(&file_data, *raw_stock_data_array, year);
    freeAllFilesPaths(&file_data);
    // filePaths freed
    // -------------------------------------------------------------------------
    return true;
}

bool load_stock_data_from_year_from_disk(
    RawStockDataArray** raw_stock_data_array,
    const u_int16_t year
    ) {
    // filePaths must be freed
    // -------------------------------------------------------------------------
    FileData file_data;
    getAllFilesPaths(INTERMEDIATE_DATA_FOLDER, &file_data);
    if (file_data.file_paths == NULL) {
        fprintf(stderr, "Error: Could not retrieve file paths.\n");
        exit(1);
    }

    // assert(file_data.file_count == NUMBER_OF_STOCK_EXAMPLES);
    (*raw_stock_data_array) = malloc(sizeof(RawStockDataArray));
    (*raw_stock_data_array)->row_arrays = malloc(
        file_data.file_count * sizeof(RowArray)
    );
    if ((*raw_stock_data_array)->row_arrays == NULL) {
        perror("malloc failed");
        exit(1);
    }
    (*raw_stock_data_array)->number_of_raw_stock_data_arrays =
        file_data.file_count;

    for (int i = 0; i < file_data.file_count; i++) {
        RowArray* current_row_array = &(*raw_stock_data_array)->row_arrays[i];
        current_row_array->rows = malloc(
            LARGEST_STOCK_DATASET_SIZE * sizeof(Row)
        );
        current_row_array->data_size = LARGEST_STOCK_DATASET_SIZE;
        if (current_row_array->rows == NULL) {
            perror("malloc failed");
            exit(1);
        }
    }

    load_raw_stock_data_from_year(&file_data, *raw_stock_data_array, year);
    freeAllFilesPaths(&file_data);
    // filePaths freed
    // -------------------------------------------------------------------------
    return true;
}

void freeAllStockData(RawStockDataArray* raw_stock_data_array) {
    for (
        int i = 0;
        i < raw_stock_data_array->number_of_raw_stock_data_arrays;
        i++
    ) {
        free(raw_stock_data_array->row_arrays[i].rows);
        free(raw_stock_data_array->row_arrays[i].stock_symbol);
    }
    free(raw_stock_data_array->row_arrays);
    free(raw_stock_data_array);
}
