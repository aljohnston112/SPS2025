#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csv_util.h"
#include "file_util.h"

#include <sys/stat.h>

#include "config.h"

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
        } else {
            symbol = strdup(lastSlashPosition);
        }
    }
    return symbol;
}

void getStockDataForSingle(const char* fileName, StockDataResult* result) {
    result->symbol = extract_symbol(fileName);
    size_t data_size;
    read_stock_csv(fileName, &data_size, result->stockData);
    // printData(result->stockData, data_size);
    result->data_size = data_size;
}

char** getAllFilesPaths(const char* folder, int* file_count) {
    DIR* dir = opendir(folder);
    if (dir == NULL) {
        perror("opendir failed");
        *file_count = 0;
        return nullptr;
    }

    size_t capacity = 11356;
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

void printStockDataResults(const StockDataResult* stockDataResults, const size_t fileCount) {
    for (size_t i = 0; i < fileCount; i++) {
        const StockDataResult* result = &stockDataResults[i];

        printf("Stock Symbol: %s\n", result->symbol);
        printf("Data Size: %zu\n", result->data_size);

        for (int j = 0; j < 7; j++) {
            const TwoDimensionalArrayElement* element = (&result->stockData)[j];
            printf("Stock Data Element %d:\n", j);

            if (j < 2) {
                printf("  ints: ");
                for (size_t k = 0; k < result->data_size; k++) {
                    printf("%d ", element->ints[k]);
                }
                printf("\n");
            } else {
                printf("  doubles: ");
                for (size_t k = 0; k < result->data_size; k++) {
                    printf("%.2f ", element->doubles[k]);
                }
                printf("\n");
            }
        }
    }
}

/**
 * @return An array of stock data, where the stock data is an array of series
 *         where the indices map to the data being represented as follows.
 *         0 -> month
 *         1 -> day
 *         2 -> open
 *         3 -> high
 *         4 -> low
 *         5 -> close
 *         6 -> volume
 */
StockDataResult* getAllStockData(int* resultCount) {
    int fileCount = 0;

    // filePaths must be freed
    char** filePaths = getAllFilesPaths(INTERMEDIATE_DATA_FOLDER, &fileCount);
    if (filePaths == NULL) {
        fprintf(stderr, "Error: Could not retrieve file paths.\n");
        *resultCount = 0;
        return nullptr;
    }

    StockDataResult* stockDataResults = malloc(fileCount * sizeof(StockDataResult));
#pragma omp parallel for default(none) shared(filePaths, fileCount, stockDataResults) num_threads(180) proc_bind(close) if(IS_PARALLEL)
    for (int i = 0; i < fileCount; i++) {
        stockDataResults[i].stockData = malloc(7 * sizeof(TwoDimensionalArrayElement));
        for (int j = 0; j < 2; j++) {
            // TwoDimensionalArrayElement* e = malloc(sizeof(TwoDimensionalArrayElement));
            stockDataResults[i].stockData[j].ints = malloc(15844 * sizeof(char));
            // stockDataResults[i].stockData[j] = *e;
        }
        for (int j = 2; j < 7; j++) {
            TwoDimensionalArrayElement* e = malloc(sizeof(TwoDimensionalArrayElement));
            // e->doubles = malloc(15844 * sizeof(double));
            stockDataResults[i].stockData[j].doubles = malloc(15844 * sizeof(double));
            // stockDataResults[i].stockData[j] = *e;
        }

        getStockDataForSingle(filePaths[i], &stockDataResults[i]);

        // printf("Stock %s:\n", stockDataResults[i].symbol);
        // printData(stockDataResults[i].stockData, stockDataResults->data_size);
    }
    freeAllFilesPaths(filePaths, fileCount);
    // filePaths freed

    *resultCount = fileCount;

    // printStockDataResults(stockDataResults, fileCount);

    return stockDataResults;
}
