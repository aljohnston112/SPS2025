#include "file_util.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "config.h"
#include "csv_util.h"
#include <omp.h>

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
            if (symbol != NULL) {
                strncpy(symbol, lastSlashPosition, length);
                symbol[length] = '\0';
            }
        }
        else {
            symbol = strdup(lastSlashPosition);
        }
    }
    return symbol;
}

void getStockDataFromSingleFile(const char* fileName, RawStockDataResults* result) {
    result->symbol = extract_symbol(fileName);
    size_t data_size;
    read_stock_csv(fileName, &data_size, result->stockData);
    result->data_size = data_size;
}

char** getAllFilesPaths(const char* folder, int* file_count) {
    DIR* dir = opendir(folder);
    if (dir == NULL) {
        perror("opendir failed");
        *file_count = 0;
        return nullptr;
    }

    size_t capacity = NUMBER_OF_STOCK_EXAMPLES;
    char** file_paths = malloc(capacity * sizeof(char*));
    if (file_paths == NULL) {
        perror("malloc failed");
        closedir(dir);
        *file_count = 0;
        return nullptr;
    }

    struct dirent* entry;
    *file_count = 0;

    while ((entry = readdir(dir)) != NULL) {
        char full_path[4096];
        snprintf(full_path, sizeof(full_path), "%s/%s", folder, entry->d_name);
        struct stat file_stat;
        if (stat(full_path, &file_stat) == 0 && S_ISREG(file_stat.st_mode)) {
            if (*file_count >= capacity) {
                capacity *= 2;
                char** temp = realloc(file_paths, capacity * sizeof(char*));
                if (temp == NULL) {
                    perror("realloc failed");
                    for (int i = 0; i < *file_count; i++) {
                        free(file_paths[i]);
                    }
                    free(file_paths);
                    closedir(dir);
                    *file_count = 0;
                    return nullptr;
                }
                file_paths = temp;
            }

            file_paths[*file_count] = strdup(full_path);
            if (file_paths[*file_count] == NULL) {
                perror("strdup failed");
                for (int i = 0; i < *file_count; i++) {
                    free(file_paths[i]);
                }
                free(file_paths);
                closedir(dir);
                *file_count = 0;
                return nullptr;
            }

            (*file_count)++;
        }
    }

    closedir(dir);
    return file_paths;
}

void freeAllFilesPaths(char** file_paths, const int file_count) {
    for (int i = 0; i < file_count; i++) {
        free(file_paths[i]);
    }
    free(file_paths);
}

void loadRawStockData(
    char** filePaths,
    const int fileCount,
    RawStockDataResults* rawStockDataResults
) {
    #pragma omp parallel for default(none) shared(filePaths, fileCount, rawStockDataResults) num_threads(180) proc_bind(close) if(IS_PARALLEL)
    for (int i = 0; i < fileCount; i++) {
        constexpr unsigned int LARGEST_STOCK_DATASET_SIZE = 15844;
        rawStockDataResults[i].stockData =
            malloc(LARGEST_STOCK_DATASET_SIZE * sizeof(Row));
        getStockDataFromSingleFile(filePaths[i], &rawStockDataResults[i]);
    }
}

/**
 * The caller is responsible for freeing.
 * @param resultCount The number of rows in the returned raw stock data results.
 * @return The results of loading the stock data from disk.
 */
RawStockDataResults* loadAllStockDataFromDisk(int* resultCount) {
    int fileCount = 0;

    // filePaths must be freed
    // -----------------------------------------------------------------------------------------
    char** filePaths = getAllFilesPaths(INTERMEDIATE_DATA_FOLDER, &fileCount);
    *resultCount = fileCount;

    if (filePaths == NULL) {
        fprintf(stderr, "Error: Could not retrieve file paths.\n");
        *resultCount = 0;
        return nullptr;
    }

    RawStockDataResults* rawStockDataResults = malloc(fileCount * sizeof(RawStockDataResults));
    loadRawStockData(filePaths, fileCount, rawStockDataResults);
    freeAllFilesPaths(filePaths, fileCount);
    // filePaths freed
    // -------------------------------------------------------------------------------------------------

    return rawStockDataResults;
}

void freeAllStockData(RawStockDataResults* results, const int resultsCount) {
    for (int i = 0; i > resultsCount; i++) {
        free(results[i].stockData);
    }
    free(results);
}
